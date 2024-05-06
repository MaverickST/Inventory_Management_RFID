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
#include "nfc_rfid.h"

uint8_t gLed = 18;
key_pad_t gKeyPad;
nfc_rfid_t gNFC;

uint8_t irq_tag_pin = 20;
bool gTag_entering = false; // Flag that indicates that a tag is being entered

volatile flags_t gFlags; // Global variable that stores the flags of the interruptions pending

void initGlobalVariables(void)
{
    gFlags.W = 0x00U;
    kp_init(&gKeyPad, 2, 6, 100000, true);
    // nfc_init_as_spi(&gNFC);
    led_init(gLed);
}

void program(void)
{
    if(gFlags.B.key){
        kp_capture(&gKeyPad);
        gFlags.B.key = 0;

        uint8_t key = gKeyPad.KEY.dkey;
        // State machine of the inventory management
        static enum {inNONE, AMOUNT, PURCHASE, SALE} in_state_inv = inNONE;
        static enum {idNONE, ID1, ID2, ID3, ID4, ID5} id_state_inv = idNONE;
        static uint32_t in_value = 0;

        switch (gNFC.userType)
        {
        case ADMIN: // Admin is entering
            /* code */
            break;
            
        case INV: // Inventory management user is entering
            // Select the type of data to enter
            if ((key >= 0x0A && key <= 0x0C) && in_state_inv == inNONE) {
                switch (key)
                {
                case 0x0A:
                    in_state_inv = AMOUNT;
                    break;
                case 0x0B:
                    in_state_inv = PURCHASE;
                    break;
                case 0x0C:
                    in_state_inv = SALE;
                    break;
                default:
                    break;
                }
            }
            // Select the ID of the product
            else if ((key >= 0x00 && key <= 0x09) && in_state_inv != inNONE && id_state_inv == idNONE){
                if (key >= 0x01 && key <=0x05) {
                    gNFC.tag.id = key;
                    id_state_inv = key;
                }else {
                    printf("Invalid ID\n");
                    in_state_inv = inNONE; // Reset the state machine
                }
            }
            else if ((key >= 0x00 && key <= 0x09) && in_state_inv != inNONE && id_state_inv != idNONE){
                id_state_inv = ID1;
                in_value = in_value*10 + key;
            }
            else if (key == 0x0D && in_state_inv != inNONE && id_state_inv != idNONE){
                switch (in_state_inv)
                {
                case AMOUNT:
                    gNFC.tag.amount = in_value; // Actualy, it should update an inventory database
                    break;
                case PURCHASE:
                    gNFC.tag.purchase_v = in_value;
                    break;
                case SALE:
                    gNFC.tag.sale_v = in_value;
                    break;
                default:
                    break;
                }
                in_state_inv = inNONE; // Reset the state machine
                id_state_inv = idNONE;
                in_value = 0;
            }else {
                printf("Invalid key\n");
                in_state_inv = inNONE; // Reset the state machine
                id_state_inv = idNONE
                in_value = 0;
            }
            
            break;

        case USER: // User is entering
            /* code */
            break;

        default:
            break;
        }
    }
    if(gFlags.B.tag){
        gFlags.B.tag = 0;
        gTag_entering = false;
        printf("Tag entering\n");
    }
}

bool check()
{
    if(gFlags.W){
        return true;
    }
    return false;
}

void gpioCallback(uint num, uint32_t mask) 
{
    switch (mask)
    {
    case GPIO_IRQ_EDGE_RISE:
        // Just one tag can be entering at the same time
        if (num == irq_tag_pin && !gTag_entering){
            gFlags.B.tag = 1;
            gTag_entering = true;
        }
        // If a tag is entering, the columns are captured
        else if (gTag_entering && !gKeyPad.KEY.dbnc){
            gKeyPad.cols = gpio_get_all() & (0x0000000F << gKeyPad.KEY.clsb); // Get columns gpio values
            kp_set_irq_rows(&gKeyPad); // Switch interrupt to rows
        }
        else {
            printf("Happend what should not happens on GPIO_IRQ_EDGE_RISE\n");
        }
        break;

    case GPIO_IRQ_LEVEL_HIGH:
        if (!gKeyPad.KEY.dbnc){
            gKeyPad.rows = gpio_get_all() & (0x0000000F << gKeyPad.KEY.rlsb); // Get rows gpio values
            kp_dbnc_set_alarm(&gKeyPad);
        }
        break;
    
    default:
        printf("Happend what should not happens on GPIO CALLBACK\n");
        break;
    }
    
    gpio_acknowledge_irq(num, mask); // gpio IRQ acknowledge
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
        kp_set_irq_cols(&gKeyPad); // Switch interrupt to columns
        gKeyPad.KEY.dbnc = 0;
        gFlags.B.key = 1; // The key is processed once the debouncer is finished
    }
}
