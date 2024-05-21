/**
 * \file
 * \brief
 * \details
 * \author      Ricardo Andres Velasquez Velez
 * \version     0.0.1
 * \date        05/10/2023
 * \copyright   Unlicensed
 */

#ifndef __GPIO_LED_H__
#define __GPIO_LED_H__

#include <stdint.h>
#include "hardware/gpio.h"
// #include "pico/time.h"
// #include "hardware/timer.h"

// #include "functs.h"

typedef struct
{
    uint8_t gpio_num;
    bool state;

    // GPIO pinout for RGB LED
    uint8_t lsb_rgb;    ///< LSB of the RGB LED
    uint8_t color;      ///< Value of the RGB LED
    uint32_t time;      ///< Time (us) for the RGB LED
    uint8_t timer_irq;  ///< Alarm timer IRQ number (TIMER_IRQ_0)

}led_rgb_t;

/**
 * @brief This function initializes the GPIOs for the RGB LED
 * 
 * @param led 
 * @param lsb_rgb 
 */
static inline void led_init(led_rgb_t *led, uint8_t lsb_rgb)
{
    led->lsb_rgb = lsb_rgb;
    led->state = false;
    led->color = 0x00;
    led->time = 500000;
    led->timer_irq = TIMER_IRQ_0;
    gpio_init_mask(0x00000007 << lsb_rgb); // gpios for key rows 2,3,4,5
    gpio_set_dir_masked(0x00000007 << lsb_rgb, 0x00000007 << lsb_rgb); // rows as outputs
    gpio_put_masked(0x00000007 << lsb_rgb, 0x00000000);
}

/**
 * @brief Configure the led Alarm
 * 
 * @param led 
 */
static inline void led_set_alarm(led_rgb_t *led)
{
    // Interrupt acknowledge
    hw_clear_bits(&timer_hw->intr, 1u << led->timer_irq);
    led->state = false;

    // Setting the IRQ handler
    irq_set_exclusive_handler(led->timer_irq, led_timer_handler);
    irq_set_enabled(led->timer_irq, true);
    hw_set_bits(&timer_hw->inte, 1u << led->timer_irq); ///< Enable alarm1 for keypad debouncer
    timer_hw->alarm[led->timer_irq] = (uint32_t)(time_us_64() + led->time); ///< Set alarm1 to trigger in 100ms
}

/**
 * @brief Configure the RGB LED: turn on the LED, set the color and set the alarm
 * 
 * @param led 
 * @param color 
 */
static inline void led_setup(led_rgb_t *led, uint8_t color) 
{
    gpio_put_masked(0x00000007 << led->lsb_rgb, (uint32_t)color << led->lsb_rgb);
    led_set_alarm(led);
}

/**
 * @brief This function turns on the LED depending on the color
 * 
 * @param led 
 * @param color just the three LSBs are used: 0b111 means white, 0b000 means off
 */
static inline void led_on(led_rgb_t *led, uint8_t color)
{
    gpio_put_masked(0x00000007 << led->lsb_rgb, (uint32_t)color << led->lsb_rgb);
}

/**
 * @brief This function turns on the LED depending on the color (struct parameter)
 * 
 * @param led 
 */
static inline void led_turn(led_rgb_t *led) 
{
    gpio_put_masked(0x00000007 << led->lsb_rgb, (uint32_t)led->color << led->lsb_rgb);
}

/**
 * @brief This function turns off the LED. Equivalent to led_on(led, 0b000).
 * 
 * @param led 
 */
static inline void led_off(led_rgb_t *led)
{
    gpio_put_masked(0x00000007 << led->lsb_rgb, 0x00000000);
}

static inline void led_toggle(led_rgb_t *led, uint8_t color)
{
    gpio_xor_mask(0x00000007 << led->lsb_rgb);
}

#endif