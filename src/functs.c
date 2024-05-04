/**
 * \file        functs.c
 * \brief
 * \details
 * 
 * 
 * \author      MST_CDA
 * \version     0.0.1
 * \date        10/04/2024
 * \copyright   Unlicensed
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"

#include "functs.h"
#include "keypad_irq.h"
#include "gpio_led.h"

key_pad_t gKeyPad;
signal_t gSignal;
gpio_button_t gButton;
dac_t gDac;
uint8_t gLed = 18;

uint8_t irq_tag_pin = 20;

volatile flags_t gFlags; // Global variable that stores the flags of the interruptions pending

void initGlobalVariables(void)
{
    gFlags.W = 0x00U;
    kp_init(&gKeyPad,2,6,true);
    signal_gen_init(&gSignal, 10, 1000, 500, true);
    signal_calculate(&gSignal);
    button_init(&gButton, 0);
    dac_init(&gDac, 10, true);
    led_init(gLed);
}

void program(void)
{
}

bool check()
{
    if(gFlags.W){
        return true;
    }
    return false;
}

//  void pwmIRQ(void)
//  {
//     uint32_t cols;
//     bool button;
//     switch (pwm_get_irq_status_mask())
//     {
//     case 0x01UL: // PWM slice 0 ISR used as a PIT to generate row sequence
//         kp_gen_seq(&gKeyPad);
//         pwm_clear_irq(0);         // Acknowledge slice 0 PWM IRQ
//         break;

//     case 0x02UL: // PWM slice 1 ISR used as a PIT to implement the keypad debouncer
//         cols = gpio_get_all() & 0x000003C0; // Get columns gpio values
//         if(kp_is_2nd_zero(&gKeyPad)){
//             if(!cols){
//                 kp_set_irq_enabled(&gKeyPad, true); // Enable the GPIO IRQs
//                 pwm_set_enabled(0, true);    // Enable the row sequence
//                 pwm_set_enabled(1, false);   // Disable the keypad debouncer
//                 gKeyPad.KEY.dbnc = 0;
//             }
//             else
//                 kp_clr_zflag(&gKeyPad);
//         }
//         else{
//             if(!cols)
//                 kp_set_zflag(&gKeyPad);
//         }

//         pwm_clear_irq(1); // Acknowledge slice 1 PWM IRQ
//         break;

//     case 0x04UL: // PWM slice 2 ISR used as a PIT to implement the button debouncer
//         button = gpio_get(gButton.KEY.gpio_num);
//         if(button_is_2nd_zero(&gButton)){
//             if(!button){
//                 signal_set_state(&gSignal, (gSignal.STATE.ss + 1)%4);
//                 button_set_irq_enabled(&gButton, true); // Enable the GPIO IRQs
//                 pwm_set_enabled(2, false);    // Disable the button debouncer
//                 signal_calculate(&gSignal); // Recalculate the signal values
//                 gButton.KEY.dbnc = 0;
//             }
//             else
//                 button_clr_zflag(&gButton);
//         }
//         else{
//             if(!button)
//                 button_set_zflag(&gButton);
//         }
//         pwm_clear_irq(2); // Acknowledge slice 2 PWM IRQ
//         break;

//     default:
//         printf("Happend what should not happens on PWM IRQ\n");
//         break;
//     }
//  }


void gpioCallback(uint num, uint32_t mask) 
{
    if (num == irq_tag_pin){
        buttonCallback(num, mask);
    }
    else{
        keypadCallback(num, mask);
    }
    
    gpio_acknowledge_irq(num, mask); // gpio IRQ acknowledge
}

 void keypadCallback(uint num, uint32_t mask)
 {
    // Capture the key pressed
    uint32_t cols = gpio_get_all() & 0x000003C0; // Get columns gpio values
    kp_capture(&gKeyPad, cols);
    // printf("Key: %02x\n", gKeyPad.KEY.dkey);

    pwm_set_enabled(0, false);  // Disable the row sequence
    pwm_set_enabled(1, true);   // Enable the keypad debouncer
    kp_set_irq_enabled(&gKeyPad, false); // Disable the keypad IRQs

    kp_set_zflag(&gKeyPad); // Set the flag that indicates that a zero was detected on keypad
    gKeyPad.KEY.dbnc = 1;

    // Auxiliar variables
    static uint8_t in_param_state = 0x00; // 0: Nothing, 1: Entering amp (A), 2: Entering offset (B), 3: Entering freq (C)
    static uint32_t param = 0; // This variable will store the value of any parameter that is being entered
    static uint8_t key_cont = 0;


    // Process the key pressed 

    // To accept a number, in_param_state must be different of 0
    if(checkNumber(gKeyPad.KEY.dkey) && in_param_state){
        param = (!key_cont)? gKeyPad.KEY.dkey : param*10 + gKeyPad.KEY.dkey;
        key_cont++;
    }
    // To accept a letter different of 0x0D, in_param_state must be 0
    else if(checkLetter(gKeyPad.KEY.dkey) && !in_param_state){
        // cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        led_on(gLed);
        switch (gKeyPad.KEY.dkey)
        {
        case 0x0A:
            in_param_state = 1;
            break;
        case 0x0B:
            in_param_state = 2;
            break;
        case 0x0C:
            in_param_state = 3;
            break;
        default:
            printf("Invalid letter\n");
            break;
        }
    }
    // To accept a 0x0D, in_param_state must be different of 0
    else if(gKeyPad.KEY.dkey == 0x0D && in_param_state){
        // cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        led_off(gLed);
        switch (in_param_state)
        {
        case 1:
            if(checkAmp(param)){
                signal_set_amp(&gSignal,param);
            }
            break;
        case 2:
            if(checkOffset(param)){
                signal_set_offset(&gSignal,param);
            }
            break;
        case 3:
            if(checkFreq(param)){
                signal_set_freq(&gSignal,param);
            }
            break;
        default:
            printf("Invalid state\n");
            break;
        }
        signal_calculate(&gSignal);
        in_param_state = 0;
        param = 0;
        key_cont = 0;
    }
    gKeyPad.KEY.nkey = 0;

 }

 void dbnc_timer_handler(void)
{
    // Interrupt acknowledge
    hw_clear_bits(&timer_hw->intr, 1u << TIMER_IRQ_1);

    uint32_t rows = gpio_get_all() & (0x0000000f << gKeyPad.KEY.rlsb); // Get rows gpio values

    if (rows){ // If a key was pressed, set the alarm again
        irq_set_exclusive_handler(TIMER_IRQ_1, dbnc_timer_handler);
        irq_set_enabled(TIMER_IRQ_1, true);
        hw_set_bits(&timer_hw->inte, 1u << TIMER_IRQ_1); // Enable alarm1 for keypad debouncer
        timer_hw->alarm[1] = (uint32_t)(time_us_64() + gKeyPad.dbnc_time); // Set alarm1 to trigger in 100ms
    }else {
        kp_set_irq_cols(&gKeyPad);
        gKeyPad.KEY.dbnc = 0;
    }
}
