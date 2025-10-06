#ifndef WS2812_H
#define WS2812_H

#include "stm32f0xx_hal.h"   // or stm32f4xx_hal.h depending on MCU

#ifdef __cplusplus
extern "C" {
#endif

/* ==============================
   Configuration Macros
   ============================== */
/* Number of LEDs in your strip */
#define WS2812_MAX_LEDS        17

/* Enable/disable brightness scaling */
#define USE_BRIGHTNESS         1
#define MAX_BRIGHTNESS         45

/* Reserve LED 0 as sacrificial pixel for 3.3V -> 5V level shifting */
#define SACRIFICIAL_PIXEL_USED

/* ==============================
   API
   ============================== */

void WS2812_Init(TIM_HandleTypeDef *htim, uint32_t channel);
void WS2812_SetLED(uint16_t led, uint8_t r, uint8_t g, uint8_t b);
void WS2812_SetBrightness(uint8_t brightness);
void WS2812_Send(void);
void WS2812_DMA_Callback(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif

#endif /* WS2812_H */
