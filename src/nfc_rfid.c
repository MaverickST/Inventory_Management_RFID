/**
 * \file        nfc_rfid.c
 * \brief
 * \details
 * \author      MST_CDA
 * \version     0.0.1
 * \date        05/10/2023
 * \copyright   Unlicensed
 */

#include <stdint.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "nfc_rfid.h"
#include "functs.h"

void nfc_init_as_i2c(nfc_rfid_t *nfc, i2c_inst_t *_i2c, uint8_t sda, uint8_t scl, uint8_t irq)
{
    nfc->i2c = _i2c;
    nfc->pinout.sda = sda;
    nfc->pinout.scl = scl;
    nfc->pinout.irq = irq;
    nfc->i2c_fifo_stat.tx = 0;
    nfc->nbf = 0;
    nfc->idx_fifo = 0;
    nfc->userType = NONE;

    nfc->i2c = _i2c;
    if (_i2c == i2c0){
        nfc->i2c_irq = I2C0_IRQ;
    } else if (_i2c == i2c1){
        nfc->i2c_irq = I2C1_IRQ;
    }

    // Configuring the DW_apb_i2c as a master:
    gpio_init(sda);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    // pull-ups are already active on slave side, this is just a fail-safe in case the wiring is faulty
    gpio_pull_up(sda);

    gpio_init(scl);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_pull_up(scl);

    i2c_init(_i2c, 400000); // Initialize the I2C bus with a speed of 400 kbps (fast mode)
    _i2c->hw->con &= ~(1 << 4); // Set Master 7Bit addressing mode (0 -> 7bit, 1 -> 10bit)

    // Set interrupt handler
    irq_set_exclusive_handler(nfc->i2c_irq, i2c_handler);

    // Initialize the configuration of the MFRC522
    nfc_config_mfrc522_irq(nfc);

}

static inline void nfc_config_mfrc522_irq(nfc_rfid_t *nfc)
{
    // Enable the DW_apb_i2c to start transmitting
    nfc->i2c->hw->enable = true;
    irq_set_enabled(nfc->i2c_irq, true);
    nfc->i2c_fifo_stat.rw = single_WRITE;
    // Write the slave address to the DW_apb_i2c
    nfc->i2c->hw->data_cmd = ADDRESS_SLAVE_MFRC522;
    nfc->i2c_fifo_stat.tx = dev_ADDRESS;
}

inline void nfc_get_nbf(nfc_rfid_t *nfc)
{
    // Enable the DW_apb_i2c to start transmitting
    nfc->i2c->hw->enable = true;
    irq_set_enabled(nfc->i2c_irq, true);
    nfc->i2c_fifo_stat.rw = single_READ; // Single read: read number of bytes of the FIFO
    // Write the slave address to the DW_apb_i2c
    nfc->i2c->hw->data_cmd = ADDRESS_SLAVE_MFRC522;
    nfc->i2c_fifo_stat.tx = dev_ADDRESS;
}

void nfc_i2c_callback(nfc_rfid_t *nfc)
{
    uint8_t regVal = 0;
    switch (nfc->i2c->hw->raw_intr_stat)
    {
    case I2C_IC_RAW_INTR_STAT_TX_EMPTY_BITS:
        printf("TX_EMPTY\n");
        switch (nfc->i2c_fifo_stat.tx)
        {
        case dev_ADDRESS: // Device address was sent
            // Send the register address
            if (!nfc->i2c_fifo_stat.rw){ // Single write (it refers to the NFC configuration)
                nfc->i2c->hw->data_cmd = ComIEnReg;
            }
            else if (nfc->i2c_fifo_stat.rw == mult_READ){ // Multiple read
                nfc->i2c->hw->data_cmd = FIFODataReg;
            }
            else{ // Single read: read number of bytes
                nfc->i2c->hw->data_cmd = FIFOLevelReg;
            }
            nfc->i2c_fifo_stat.tx = reg_ADDRESS;

            break;

        case reg_ADDRESS: // Register address was sent
            // Send the data
            regVal = 0xA0; // rx irq of ComIEnReg in MFRC522 is enabled
            if (!nfc->i2c_fifo_stat.rw){ // Single write (it refers to the NFC configuration)
                nfc->i2c->hw->data_cmd = I2C_IC_DATA_CMD_STOP_BITS | (uint32_t)regVal;
            }
            else if (nfc->i2c_fifo_stat.rw == mult_READ){ // Multiple read
                nfc->i2c->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS | I2C_IC_DATA_CMD_RESTART_BITS | (uint32_t)ADDRESS_SLAVE_MFRC522;
            }
            else { // Single read: read number of bytes            |       read next byte     |
                nfc->i2c->hw->data_cmd = I2C_IC_DATA_CMD_STOP_BITS | I2C_IC_DATA_CMD_CMD_BITS | 
                                         I2C_IC_DATA_CMD_RESTART_BITS | (uint32_t)ADDRESS_SLAVE_MFRC522;
            }
            nfc->i2c_fifo_stat.tx = data_SENT;
            break;

        case data_SENT: // Data was sent
            if (!nfc->i2c_fifo_stat.rw){ // Single write
                printf("Initial configuration of NFC finished\n");
                irq_set_enabled(nfc->i2c_irq, false); // The initial configuration to NFC is finished
                nfc->i2c->hw->enable = false; // Disable the DW_apb_i2c
            }
            else {
                printf("Single and multiple reading generated an irq\n");
            }
            
            break;
        default:
            break;
        }
        break;
    case I2C_IC_RAW_INTR_STAT_RX_FULL_BITS:
        printf("RX_FULL\n");
        if (nfc->i2c_fifo_stat.rw == 1){ // Multiple read
            if (nfc->nbf - 1 == nfc->idx_fifo){
                irq_set_enabled(nfc->i2c_irq, false); // The reading of the number of bytes is finished
                nfc->i2c->hw->enable = false; // Disable the DW_apb_i2c
                nfc->fifo[nfc->idx_fifo] = (uint8_t)nfc->i2c->hw->data_cmd;
                nfc->idx_fifo = 0;
            }
            else {
                nfc->fifo[nfc->idx_fifo] = (uint8_t)nfc->i2c->hw->data_cmd;
                nfc->idx_fifo++;
            }
        }
        else if (nfc->i2c_fifo_stat.rw == 2){ // Single read: read number of bytes
            irq_set_enabled(nfc->i2c_irq, false); // The reading of the number of bytes is finished
            nfc->i2c->hw->enable = false; // Disable the DW_apb_i2c
            nfc->nbf = (uint8_t)nfc->i2c->hw->data_cmd;
        }
        
        break;

    default:
        printf("Happend what should not happens on I2C_HANDLER\n");
        break;
    }
}
