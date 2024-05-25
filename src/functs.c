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
#include "inventory.h"
#include "liquid_crystal_i2c.h"

// SPI pins
#define PIN_SCK 10
#define PIN_MOSI 11
#define PIN_MISO 12
#define PIN_CS 13
#define PIN_IRQ 0
#define PIN_RST 16

// I2C pins
#define PIN_SDA 14
#define PIN_SCL 15

lcd_t gLcd;
led_rgb_t gLed;
key_pad_t gKeyPad;
nfc_rfid_t gNFC;
inventory_t gInventory;

flags_t gFlags; ///< Global variable that stores the flags of the interruptions pending

void initGlobalVariables(void)
{
    lcd_init(&gLcd, 0x20, i2c1, 16, 2, 100, PIN_SDA, PIN_SCL);
    gFlags.W = 0x00U;
    led_init(&gLed, 18);
    kp_init(&gKeyPad, 2, 6, 100000, true);
    // nfc_init_as_i2c(&gNFC, i2c1, 14, 15, 12, 11);
    nfc_init_as_spi(&gNFC, spi1, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_CS, PIN_IRQ, PIN_RST);
    inventory_init(&gInventory, false);
}

void program(void)
{
    if (gFlags.B.key){
        kp_capture(&gKeyPad);
        gFlags.B.key = 0;

        uint8_t key = gKeyPad.KEY.dkey;
        printf("Key: %d\n", key);
        static uint32_t in_value = 0;
        static uint8_t in_cont = 0;

        // State machine of the admin
        // The admin password is 1234 (4 digits) at the beginning
        static enum {adminNONE, PASS} in_state_admin = adminNONE;

        // State machine of the inventory management
        static enum {inNONE, AMOUNT, PURCHASE, SALE} in_state_inv = inNONE;
        static enum {idNONE, ID1, ID2, ID3, ID4, ID5} id_state_inv = idNONE;

        switch (gNFC.userType)
        {
        case ADMIN: ///< Admin is entering
            if (checkNumber(key) && in_state_admin == adminNONE){
                in_value = in_value*10 + key;
                in_cont++;
                if (in_cont == 4){
                    if (in_value == 1234){
                        in_state_admin = PASS;
                        printf("Correct password\n");
                        // Led control
                        led_setup(&gLed, 0x05); ///< Purple color
                    }else {
                        printf("Incorrect password\n");
                        gNFC.tag.is_present = false;
                        gNFC.check = true; ///< Restart the check tag timer
                        in_state_admin = adminNONE;
                        in_value = 0;
                        in_cont = 0;
                        // Led control
                        led_setup(&gLed, 0x04); ///< Red color
                    }
                }
            }
            ///< Reset the inventory database
            else if (key == 0x0E && in_state_admin == PASS) {
                inventory_reset(&gInventory);
                // Led control
                led_setup(&gLed, 0x03); ///< Blue color
            }
            ///< Finish the process
            else if (key == 0x0D){
                printf("Finished Admin process\n");
                gNFC.tag.is_present = false;
                gNFC.check = true; ///< Restart the check tag timer
                in_state_admin = adminNONE;
                in_value = 0;
                in_cont = 0;
                // Led control
                led_setup(&gLed, 0x06); ///< Yellow color
            }
            else {
                printf("Invalid key - ADMIN\n");
                // Led control
                led_setup(&gLed, 0x04); ///< Red color
            }
            
            break;
            
        case INV: ///< Inventory management user is entering
            ///< Select the type of data to enter
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
            ///< Select the ID of the product
            else if (checkNumber(key) && in_state_inv != inNONE && id_state_inv == idNONE){
                if (key >= 0x01 && key <=0x05) {
                    gNFC.tag.id = key;
                    id_state_inv = key;
                }else {
                    printf("Invalid ID\n");
                    in_state_inv = inNONE; ///< Reset the state machine
                    // Led control
                    led_setup(&gLed, 0x04); ///< Red color
                }
            }
            ///< Enter the value of the data
            else if (checkNumber(key) && in_state_inv != inNONE && id_state_inv != idNONE){
                in_value = in_value*10 + key;
            }
            ///< Update the database
            else if (key == 0x0D && in_state_inv != inNONE && id_state_inv != idNONE){
                printf("Updating database   value: %d    id: %d   type: %d\n", in_value, id_state_inv, in_state_inv);
                switch (in_state_inv)
                {
                case AMOUNT:
                    gInventory.database[id_state_inv - 1][0] = in_value;
                    break;
                case PURCHASE:
                    gInventory.database[id_state_inv - 1][1] = in_value;
                    break;
                case SALE:
                    gInventory.database[id_state_inv - 1][2] = in_value;
                    break;
                default:
                    break;
                }
                inventory_store(&gInventory);
                in_state_inv = inNONE; // Reset the state machine
                id_state_inv = idNONE;
                in_value = 0;
                // Led control
                led_setup(&gLed, 0x02); ///<  Green color
            }
            // Finish the process
            else if (key == 0x0D && in_state_inv == inNONE && id_state_inv == idNONE) {
                gNFC.tag.is_present = false;
                gNFC.check = true; ///< Restart the check tag timer
                printf("Finished Inv User\n");
                // Led control
                led_setup(&gLed, 0x06); ///< Yellow color
            }
            else {
                printf("Invalid key - INV\n");
                in_state_inv = inNONE; ///< Reset the state machine
                id_state_inv = idNONE;
                in_value = 0;
                // Led control
                led_setup(&gLed, 0x04); ///< Red color
            }
            break;

        case USER: ///< User is entering
            ///< Input transaction
            if (key == 0x0A) {
                inventory_in_transaction(&gInventory);
                    
                gNFC.tag.is_present = false;
                gNFC.check = true; ///< Restart the check tag timer
                printf("Finished User\n");
                // Led control
                led_setup(&gLed, 0x06); ///< Yellow color
                gInventory.state = DATA_BASE; ///< Now that user go out, Show the data base on LCD
            }
            ///< Output transaction
            else if (key == 0x0B) {
                if (inventory_out_transaction(&gInventory)) { ///< Transaction success
                    // Led control
                    led_setup(&gLed, 0x06); ///< Yellow color
                }else { ///< Transaction failed
                    // Led control
                    led_setup(&gLed, 0x04); ///< Red color
                }
                gNFC.tag.is_present = false;
                gNFC.check = true; ///< Restart the check tag timer
                printf("Finished User\n");
                gInventory.state = DATA_BASE; ///< Now that user go out, Show the data base on LCD
            }
            else {
                printf("Invalid key - USER\n");
                // Led control
                led_setup(&gLed, 0x04); ///< Red color
            }
            break;

        default:
            break;
        }
    }
    ///< NFC interrupt flags
    if (gFlags.B.nfc_tag) {
        gFlags.B.nfc_tag = 0; ///< Clear the flag
        nfc_read_card_serial(&gNFC); ///< Read the serial number of the card
        // Print the serial number of the card
        printf("\nCard UID: ");
        for (int i = 0; i < gNFC.uid.size; i++) {
            printf("%02X", gNFC.uid.uidByte[i]);
        }
        printf("\n");
        // Check if the card is a Mifare Classic card
        if(nfc_authenticate(&gNFC, PICC_CMD_MF_AUTH_KEY_A, gNFC.blockAddr, &gNFC.keyByte[0], &(gNFC.uid))==0){
            if(nfc_read_card(&gNFC, gNFC.blockAddr, gNFC.bufferRead, &gNFC.sizeRead)==0){
                gNFC.tag.is_present = true;
                printf("Block readed\n\r");
                for (int i = 0; i < 16; i++) {
                    printf("%02x ", gNFC.bufferRead[i]);
                }
                printf("\n");
                nfc_stop_crypto1(&gNFC);
                
                led_setup(&gLed, 0x02); ///<  Green color
                if(nfc_get_data_tag(&gNFC)) {///< From the nfc fifo, get the data tag and chet the ID of the tag
                    // irq_set_enabled(gNFC.timer_irq, false); ///< Disable the check tag timer
                    gNFC.check = false; ///< Stop the check of the tag
                }
                // Congiguring the gInventory to show correctlly the data
                gInventory.tag = gNFC.tag; ///< Copy the tag data to the inventory tag
                if (gNFC.tag.id >= 0x01 && gNFC.tag.id <= 0x05){
                    gInventory.state = IN__OUT_TRANSACTION; ///< Show the transaction
                }
            }else{
                led_setup(&gLed, 0x04); ///< Red color
            }
        }else {
            led_setup(&gLed, 0x04); ///< Red color
        }
    }

    ///< Keypad interrupt flags
    if (gFlags.B.kpad_switch){
        gKeyPad.cols = gpio_get_all() & (0x0000000f << gKeyPad.KEY.clsb); ///< Get columns gpio values
        kp_set_irq_rows(&gKeyPad); ///< Switch interrupt to columns
        gKeyPad.rows = gpio_get_all() & (0x0000000f << gKeyPad.KEY.rlsb); ///< Get rows gpio values
        pwm_set_enabled(gKeyPad.pwm_slice, true); ///< Set the debouncer alarm
        gFlags.B.kpad_switch = 0;
    }
    ///< Inventory show interrupt flags
    if (gFlags.B.inv_show){
        gFlags.B.inv_show = 0; ///< Clear the flag
        show_inventory(); ///< Show the inventory on the LCD
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
        if (gNFC.tag.is_present && !gKeyPad.KEY.dbnc){
            kp_set_irq_enabled(&gKeyPad, true, false); ///< Disable the columns interrupt
            gKeyPad.KEY.dbnc = 1; ///< Activate the debouncer
            gFlags.B.kpad_switch = 1; ///< Activate the flag to switch the interrupt to columns
        }
        else {
            printf("There is no a tag entering - RISE\n");
        }
        break;
    
    default:
        printf("Happend what should not happens on GPIO CALLBACK\n");
        break;
    }
    
    gpio_acknowledge_irq(num, mask); ///< gpio IRQ acknowledge
}

void led_timer_handler(void)
{
    // Set the alarm
    hw_clear_bits(&timer_hw->intr, 1u << gLed.timer_irq);

    if (gLed.state){
        led_set_alarm(&gLed);
    }else {
        led_off(&gLed);
        irq_set_enabled(gLed.timer_irq, false); ///< Disable the led timer
    }
}

void check_tag_timer_handler(void)
{
    // Set the alarm
    hw_clear_bits(&timer_hw->intr, 1u << gNFC.timer_irq);
    // Setting the IRQ handler
    irq_set_exclusive_handler(gNFC.timer_irq, check_tag_timer_handler);
    irq_set_enabled(gNFC.timer_irq, true);
    hw_set_bits(&timer_hw->inte, 1u << gNFC.timer_irq); ///< Enable alarm1 for keypad debouncer
    timer_hw->alarm[gNFC.timer_irq] = (uint32_t)(time_us_64() + gNFC.timeCheck); ///< Set alarm1 to trigger in 1s

    // Check for a tag entering
    if (!gNFC.tag.is_present && nfc_is_new_tag(&gNFC) && gNFC.check){
        gFlags.B.nfc_tag = 1; ///< Activate the flag of the NFC interruption to read the card
    }

    // Show the inventory every 3 seconds
    gInventory.count.it = (gInventory.count.it + 1) % 3;
    if (gInventory.count.it == 2){
        gFlags.B.inv_show = 1; ///< Activate the flag of the inventory show interruption
        gInventory.count.it = 0;
    }
}

void show_inventory(void)
{
    uint8_t str_0[16]; ///< Line 0 of the LCD
    uint8_t str_1[16]; ///< Line 1 of the LCD
    
    lcd_send_str_cursor(&gLcd, "                ", 0, 0);
    lcd_send_str_cursor(&gLcd, "                ", 1, 0);

    switch (gInventory.state)
    {
    case DATA_BASE:
        switch (gInventory.count.id)
        {
        case 5: ///< Show the today transactions
            if (!gInventory.count.frame) {
                sprintf((char *)str_1, "AMNT: %d", gInventory.today.amount);
                lcd_send_str_cursor(&gLcd, "TodayTransactions", 0, 0);
                lcd_send_str_cursor(&gLcd, str_1, 1, 0);
            }else {
                sprintf((char *)str_0, "PRCH: %d", gInventory.today.purchases);
                sprintf((char *)str_1, "SAL: %d", gInventory.today.sales);
                lcd_send_str_cursor(&gLcd, str_0, 0, 0);
                lcd_send_str_cursor(&gLcd, str_1, 1, 0);
            }
            break;
        default: ///< Show the data base
            if (!gInventory.count.frame) {
                sprintf((char *)str_0, "Inventory  ID: %d", gInventory.count.id + 1);
                sprintf((char *)str_1, "AMNT: %d", gInventory.database[gInventory.count.id][0]);
                lcd_send_str_cursor(&gLcd, str_0, 0, 0);
                lcd_send_str_cursor(&gLcd, str_1, 1, 0);
            }else {
                sprintf((char *)str_0, "PRCH: %d", gInventory.database[gInventory.count.id][1]);
                sprintf((char *)str_1, "SAL: %d", gInventory.database[gInventory.count.id][2]);
                lcd_send_str_cursor(&gLcd, str_0, 0, 0);
                lcd_send_str_cursor(&gLcd, str_1, 1, 0);
            }
            break;
        }
        break;
    case IN__OUT_TRANSACTION:
        if (!gInventory.count.frame) { ///< First frame
            sprintf((char *)str_0, "TagData  ID: %d", gInventory.tag.id);
            sprintf((char *)str_1, "AMNT: %d", gInventory.tag.amount);
            lcd_send_str_cursor(&gLcd, str_0, 0, 0);
            lcd_send_str_cursor(&gLcd, str_1, 1, 0);
        }else {
            sprintf((char *)str_0, "PRCH: %d", gInventory.tag.purchase_v);
            sprintf((char *)str_1, "SAL: %d", gInventory.tag.sale_v);
            lcd_send_str_cursor(&gLcd, str_0, 0, 0);
            lcd_send_str_cursor(&gLcd, str_1, 1, 0);
        }
        break;
    default:
        break;
    }
    gInventory.count.frame += 1;
    if (gInventory.count.frame){
        gInventory.count.id = (gInventory.count.id + 1) % 6;
    }
}

void initPWMasPIT(uint8_t slice, uint16_t milis, bool enable)
{
    assert(milis<=262);                  // PWM can manage interrupt periods greater than 262 milis
    float prescaler = (float)SYS_CLK_KHZ/500;
    assert(prescaler<256); // the integer part of the clock divider can be greater than 255 
                 // ||   counter frecuency    ||| Period in seconds taking into account de phase correct mode |||   
    uint32_t wrap = (1000*SYS_CLK_KHZ*milis/(prescaler*2*1000)); // 500000*milis/2000
    assert(wrap<((1UL<<17)-1));
    // Configuring the PWM
    pwm_config cfg =  pwm_get_default_config();
    pwm_config_set_phase_correct(&cfg, true);
    pwm_config_set_clkdiv(&cfg, prescaler);
    pwm_config_set_clkdiv_mode(&cfg, PWM_DIV_FREE_RUNNING);
    pwm_config_set_wrap(&cfg, wrap);
    pwm_set_irq_enabled(slice, true);
    irq_set_enabled(PWM_IRQ_WRAP, true);
    pwm_init(slice, &cfg, enable);
 }

 void pwm_handler(void)
 {
    uint32_t rows;
    bool button;
    switch (pwm_get_irq_status_mask())
    {
    case 0x01UL: // PWM slice 0 is used for the button debouncer
        rows = gpio_get_all() & (0x0000000f << gKeyPad.KEY.rlsb); ///< Get rows gpio values
        if (!rows){ ///< If a key was not pressed, process the key and disable the debouncer
            gKeyPad.KEY.dbnc = 0;
            gFlags.B.key = 1; ///< The key is processed once the debouncer is finished
            kp_set_irq_cols(&gKeyPad); ///< Switch interrupt to columns
            pwm_set_enabled(gKeyPad.pwm_slice, false); ///< Disable the debouncer PIT
        }else {
            printf("Debouncing\n");
        }
        pwm_clear_irq(0);         // Acknowledge slice 0 PWM IRQ
        break; 

    default:
        printf("Happend what should not happens on PWM IRQ\n");
        break;
    }
 }

