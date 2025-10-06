#include "ws2812.h"

/* ==============================
   Internal Variables
   ============================== */
static TIM_HandleTypeDef *ws2812_htim;
static uint32_t ws2812_channel;

/* LED buffers */
static uint8_t LED_Data[WS2812_MAX_LEDS][4];
static uint8_t LED_Mod[WS2812_MAX_LEDS][4];

/* PWM data buffer: 24 bits per LED + reset */
#define WS2812_PERIOD_TICKS 60
#define WS2812_PERIOD_NS 1250u
#define WS2812_T0H_NS 400u
#define WS2812_T1H_NS 800u

#define WS2812_0_HIGH ((uint16_t)((WS2812_PERIOD_TICKS * WS2812_T0H_NS + (WS2812_PERIOD_NS / 2)) / WS2812_PERIOD_NS))
#define WS2812_1_HIGH ((uint16_t)((WS2812_PERIOD_TICKS * WS2812_T1H_NS + (WS2812_PERIOD_NS / 2)) / WS2812_PERIOD_NS))

#define WS2812_RESET_US 50u
#define WS2812_RESET_PERIODS (((50000u + (WS2812_PERIOD_NS - 1)) / WS2812_PERIOD_NS) + 8u)

static uint16_t pwmData[(24 * WS2812_MAX_LEDS) + 64 + WS2812_RESET_PERIODS];

static volatile int datasentflag = 0;
static uint8_t current_brightness = MAX_BRIGHTNESS;

/* ==============================
   Implementation
   ============================== */
   
void WS2812_Init(TIM_HandleTypeDef *htim, uint32_t channel)
{
    ws2812_htim = htim;
    ws2812_channel = channel;

    /* Clear LED data */
    for (int i = 0; i < WS2812_MAX_LEDS; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            LED_Data[i][j] = 0;
            LED_Mod[i][j] = 0;
        }
    }
}

void WS2812_SetLED(uint16_t led, uint8_t r, uint8_t g, uint8_t b)
{
#ifdef SACRIFICIAL_PIXEL_USED
    if (led == 0)
        return; // skip LED 0
    if (led >= WS2812_MAX_LEDS)
        return;
#else
    if (led >= WS2812_MAX_LEDS)
        return;
#endif

    LED_Data[led][1] = g;
    LED_Data[led][2] = r;
    LED_Data[led][3] = b;
}

void WS2812_SetBrightness(uint8_t brightness)
{
#if USE_BRIGHTNESS
    if (brightness > MAX_BRIGHTNESS)
        brightness = MAX_BRIGHTNESS;
    current_brightness = brightness;

    for (int i = 0; i < WS2812_MAX_LEDS; i++)
    {
        LED_Mod[i][0] = LED_Data[i][0];
        for (int j = 1; j < 4; j++)
        {
            LED_Mod[i][j] = (uint8_t)((LED_Data[i][j] * (uint32_t)brightness) / (uint32_t)MAX_BRIGHTNESS);
        }
    }
#else
    (void)brightness;
#endif
}

void WS2812_Send(void)
{
    uint32_t indx = 0;
    uint32_t color;

    for (int i = 0; i < WS2812_MAX_LEDS; i++)
    {
#if USE_BRIGHTNESS
        color = ((LED_Mod[i][1] << 16) | (LED_Mod[i][2] << 8) | (LED_Mod[i][3]));
#else
        color = ((LED_Data[i][1] << 16) | (LED_Data[i][2] << 8) | (LED_Data[i][3]));
#endif

        for (int bit = 23; bit >= 0; bit--)
        {
            pwmData[indx++] = (color & (1UL << bit)) ? WS2812_1_HIGH : WS2812_0_HIGH;
        }
    }

    for (uint32_t r = 0; r < WS2812_RESET_PERIODS; r++)
    {
        pwmData[indx++] = 0;
    }

    datasentflag = 0;
    HAL_TIM_PWM_Start_DMA(ws2812_htim, ws2812_channel, (uint32_t *)pwmData, indx);
    while (!datasentflag)
    {
    }
    datasentflag = 0;
}

void WS2812_DMA_Callback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == ws2812_htim->Instance)
    {
        HAL_TIM_PWM_Stop_DMA(ws2812_htim, ws2812_channel);
        datasentflag = 1;
    }
}
