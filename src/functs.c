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

bool gTag_entering = false; ///< Flag that indicates that a tag is being entered

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
                        gTag_entering = false;
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
                gTag_entering = false;
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
                gTag_entering = false;
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
                inventory_in_transaction(&gInventory, 
                    gNFC.tag.id, gNFC.tag.amount, gNFC.tag.purchase_v, gNFC.tag.sale_v);
                ///< It is missing to show the data of the transaction on the screen
            }
            ///< Output transaction
            else if (key == 0x0B) {
                inventory_out_transaction(&gInventory, 
                    gNFC.tag.id, gNFC.tag.amount, gNFC.tag.purchase_v, gNFC.tag.sale_v);
                ///< It is missing to show the data of the transaction on the screen
            }
            ///< Finish the process
            else if (key == 0x0D) {
                gTag_entering = false;
                printf("Finished User\n");
                // Led control
                led_setup(&gLed, 0x06); ///< Yellow color
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
    // if (gNFC.flags.B.nbf){
    //     nfc_get_nbf(&gNFC); ///< Init the process to get the number of bytes in the NFC FIFO
    //     gNFC.flags.B.nbf = 0;
    // }
    // if (gNFC.flags.B.dfifo){
    //     nfc_get_data_fifo(&gNFC); ///< Init the proccess to get the data from the NFC FIFO
    //     gNFC.flags.B.dfifo = 0;
    // }
    // if (gNFC.flags.B.dtag){
    //     nfc_get_data_tag(&gNFC); ///< From the nfc fifo, get the data tag
    //     gNFC.flags.B.dtag = 0;
    // }
    ///< Keypad interrupt flags
    if (gFlags.B.kpad_rows){
        kp_set_irq_rows(&gKeyPad); ///< Switch interrupt to rows
        gFlags.B.kpad_rows = 0;
    }
    if (gFlags.B.kpad_cols){
        gKeyPad.cols = gpio_get_all() & (0x0000000f << gKeyPad.KEY.clsb); ///< Get columns gpio values
        kp_set_irq_rows(&gKeyPad); ///< Switch interrupt to columns
        gKeyPad.rows = gpio_get_all() & (0x0000000f << gKeyPad.KEY.rlsb); ///< Get rows gpio values
        gKeyPad.KEY.dbnc = 1; ///< Activate the debouncer
        kp_dbnc_set_alarm(&gKeyPad); ///< Set the debouncer alarm
        gFlags.B.kpad_cols = 0;
    }
}

bool check()
{
    if(gFlags.W | gNFC.flags.W){
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
        if (num == gNFC.pinout.irq && !gTag_entering){
            // gNFC.flags.B.nbf = 1; ///< Activate the flag to get the number of bytes in the NFC FIFO
            gTag_entering = true;
            printf("Tag entering - RISE\n");
        }
        // If a tag is entering, the columns are captured
        else if (gTag_entering && !gKeyPad.KEY.dbnc){
            kp_set_irq_enabled(&gKeyPad, true, false); ///< Disable the columns interrupt
            gFlags.B.kpad_cols = 1; ///< Activate the flag to switch the interrupt to columns
        }
        else {
            printf("There is no a tag entering - RISE\n");
        }
        break;

    case GPIO_IRQ_EDGE_FALL:
        // Just one tag can be entering at the same time
        if (num == gNFC.pinout.irq && !gTag_entering){
            // gNFC.flags.B.nbf = 1; ///< Activate the flag to get the number of bytes in the NFC FIFO
            gTag_entering = true;
            printf("Tag entering - FALL\n");
        }
        else {
            printf("Edge fall\n");
        }

        break;
    
    default:
        printf("Happend what should not happens on GPIO CALLBACK\n");
        break;
    }
    
    gpio_acknowledge_irq(num, mask); ///< gpio IRQ acknowledge
}

 void dbnc_timer_handler(void)
{
    ///< Interrupt acknowledge
    hw_clear_bits(&timer_hw->intr, 1u << gKeyPad.timer_irq);

    uint32_t rows = gpio_get_all() & (0x0000000f << gKeyPad.KEY.rlsb); ///< Get rows gpio values
    if (rows){ ///< If a key was pressed, set the alarm again
        printf("Debouncer\n");
        kp_dbnc_set_alarm(&gKeyPad);
    }else {
        gKeyPad.KEY.dbnc = 0;
        gFlags.B.key = 1; ///< The key is processed once the debouncer is finished
        kp_set_irq_cols(&gKeyPad); ///< Switch interrupt to columns
        irq_set_enabled(gKeyPad.timer_irq, false); ///< Disable the debouncer timer
    }
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
    lcd_send_str_cursor(&gLcd, "Check tag handler", 0, 0);
    // Set the alarm
    hw_clear_bits(&timer_hw->intr, 1u << gNFC.timer_irq);
    // Setting the IRQ handler
    irq_set_exclusive_handler(gNFC.timer_irq, check_tag_timer_handler);
    irq_set_enabled(gNFC.timer_irq, true);
    hw_set_bits(&timer_hw->inte, 1u << gNFC.timer_irq); ///< Enable alarm1 for keypad debouncer
    timer_hw->alarm[gNFC.timer_irq] = (uint32_t)(time_us_64() + gNFC.timeCheck); ///< Set alarm1 to trigger in 1s

    // Check for a tag entering
    if (!gTag_entering && nfc_is_new_tag(&gNFC)){
        gTag_entering = true;
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
                led_setup(&gLed, 0x02); ///<  Green color
                printf("Block readed\n\r");
                for (int i = 0; i < 16; i++) {
                    printf("%02x ", gNFC.bufferRead[i]);
                }
                printf("\n");
                nfc_stop_crypto1(&gNFC);
                printf( "\n");
            }else{
                led_setup(&gLed, 0x04); ///< Red color
            }
        }else {
            led_setup(&gLed, 0x04); ///< Red color
        }
    }
}


