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
#include "pico/binary_info.h"

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
    nfc->userType = INV;

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

    i2c_init(_i2c, 400000); ///< Initialize the I2C bus with a speed of 400 kbps (fast mode)
    _i2c->hw->con &= ~(1 << 4); ///< Set Master 7Bit addressing mode (0 -> 7bit, 1 -> 10bit)
    irq_set_exclusive_handler(nfc->i2c_irq, i2c_handler);

    // Set the IRQ pin as input
    gpio_init(irq);
    gpio_set_dir(irq, GPIO_IN);
    gpio_pull_up(irq);
    gpio_set_irq_enabled_with_callback(irq, GPIO_IRQ_EDGE_FALL, true, gpioCallback);
    
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));

    // Initialize the configuration of the MFRC522 (comment or uncomment the desired configuration)
    nfc_config_mfrc522_irq(nfc); // IRQ's configuration
    // nfc_config_blocking(nfc);    // Blocking configuration

}

void nfc_config_blocking(nfc_rfid_t *nfc)
{
    // Commands to activate the reception of the NFC
    uint8_t count = 0;
    uint8_t bufC1[2] = {FIFODataReg, 0x26};
    uint8_t bufC2[2] = {CommandReg, 0x0C};
    uint8_t bufC3[2] = {BitFramingReg, 0x87};
    count = i2c_write_blocking(nfc->i2c, ADDRESS_SLAVE_MFRC522, bufC1, 2, false);
    count = i2c_write_blocking(nfc->i2c, ADDRESS_SLAVE_MFRC522, bufC2, 2, false);
    count = i2c_write_blocking(nfc->i2c, ADDRESS_SLAVE_MFRC522, bufC3, 2, false);

    // Configure the MFRC522 IRQ
    printf("Configuring the MFRC522 IRQ\n");
    uint8_t irqEnRx = 0xA0;
    uint8_t irqEnMFIN = 0x90;
    uint8_t buf1[2] = {DivIEnReg, irqEnMFIN}; // ComIEnReg, irqEnRx
    count = i2c_write_blocking(nfc->i2c, ADDRESS_SLAVE_MFRC522, buf1, 2, false);
    if (count < 0){
        printf("Error on writing the ComIEnReg\n");
    }else {
        printf("ComIEnReg was written\n");
    }
    uint8_t clearInt = 0x7F;
    uint8_t buf2[2] = {ComIrqReg, clearInt};
    i2c_write_blocking(nfc->i2c, ADDRESS_SLAVE_MFRC522, buf2, 2, false);

    // Get the version of the MFRC522
    count = i2c_write_blocking(nfc->i2c, ADDRESS_SLAVE_MFRC522, (uint8_t *)VersionReg, 1, true);
    count = i2c_read_blocking(nfc->i2c, ADDRESS_SLAVE_MFRC522, &nfc->version, 1, false);
    if (count < 0){
        printf("Error on reading the version of the MFRC522\n");
    }else {
        printf("Version of the MFRC522: %d\n", nfc->version);
    }

    // Get the number of bytes in the NFC FIFO
    count = i2c_write_blocking(nfc->i2c, ADDRESS_SLAVE_MFRC522, (uint8_t *)FIFOLevelReg, 1, true);
    count = i2c_read_blocking(nfc->i2c, ADDRESS_SLAVE_MFRC522, &nfc->nbf, 1, false);
    if (count < 0){
        printf("Error on reading the number of bytes in the NFC FIFO\n");
    }else {
        printf("Number of bytes in the NFC FIFO: %d\n", nfc->nbf);
    }
}

void nfc_i2c_callback(nfc_rfid_t *nfc)
{
    uint8_t regVal = 0;
    printf("NFC callback: %08x\n", nfc->i2c->hw->raw_intr_stat);
    switch (nfc->i2c->hw->raw_intr_stat)
    {
    case I2C_IC_RAW_INTR_STAT_TX_EMPTY_BITS:
        printf("TX_EMPTY\n");
        switch (nfc->i2c_fifo_stat.tx)
        {
        case dev_ADDRESS: ///< Device address was sent
            ///< Send the register address
            if (nfc->i2c_fifo_stat.rw == single_WRITE){     ///< Single write (it refers to the NFC configuration)
                nfc->i2c->hw->data_cmd = DivIEnReg;
            }
            else if (nfc->i2c_fifo_stat.rw == mult_READ){   ///< Multiple read
                nfc->i2c->hw->data_cmd = FIFODataReg;
            }
            else{                                           ///< Single read: read number of bytes
                nfc->i2c->hw->data_cmd = FIFOLevelReg;
            }
            nfc->i2c_fifo_stat.tx = reg_ADDRESS;

            break;

        case reg_ADDRESS: ///< Register address was sent
            // Send the data
            regVal = 0x10; ///< rx irq of ComIEnReg in MFRC522 is enabled
            if (nfc->i2c_fifo_stat.rw == single_WRITE){     ///< Single write (it refers to the NFC configuration)
                nfc->i2c->hw->data_cmd = I2C_IC_DATA_CMD_STOP_BITS | (uint32_t)regVal;
            }
            else if (nfc->i2c_fifo_stat.rw == mult_READ){   ///< Multiple read
                nfc->i2c->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS | I2C_IC_DATA_CMD_RESTART_BITS | (uint32_t)ADDRESS_SLAVE_MFRC522;
            }
            else {                                          ///< Single read: read number of bytes
                nfc->i2c->hw->data_cmd = I2C_IC_DATA_CMD_STOP_BITS | I2C_IC_DATA_CMD_CMD_BITS | 
                                         I2C_IC_DATA_CMD_RESTART_BITS | (uint32_t)ADDRESS_SLAVE_MFRC522;
            }
            nfc->i2c_fifo_stat.tx = data_SENT;
            break;

        case data_SENT: ///< Data was sent
            printf("Data was sent  rw: %d \n", nfc->i2c_fifo_stat.rw);
            if (nfc->i2c_fifo_stat.rw == single_WRITE){ ///< Single write
                printf("Initial configuration of NFC finished\n");
                irq_set_enabled(nfc->i2c_irq, false); ///< The initial configuration to NFC is finished
                nfc->i2c->hw->enable = false; ///< Disable the DW_apb_i2c
                // nfc->flags.B.nbf = 1; ///< Activate the flag to get the number of bytes in the NFC FIFO
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
        if (nfc->i2c_fifo_stat.rw == mult_READ){        ///< Multiple read
            if (nfc->nbf - 1 == nfc->idx_fifo){
                nfc->fifo[nfc->idx_fifo] = (uint8_t)nfc->i2c->hw->data_cmd;
                irq_set_enabled(nfc->i2c_irq, false); ///< The reading of the number of bytes is finished
                nfc->i2c->hw->enable = false; ///< Disable the DW_apb_i2c
                nfc->idx_fifo = 0;
                nfc->flags.B.dtag = 1; ///< Activate the flag organize the data from fifo to the tag structure
            }
            else {
                nfc->fifo[nfc->idx_fifo] = (uint8_t)nfc->i2c->hw->data_cmd;
                nfc->idx_fifo++;
            }
        }
        else if (nfc->i2c_fifo_stat.rw == single_READ){ ///< Single read: read number of bytes
            nfc->nbf = (uint8_t)nfc->i2c->hw->data_cmd;
            printf("Number of bytes in the NFC FIFO: %d\n", nfc->nbf);
            irq_set_enabled(nfc->i2c_irq, false); ///< The reading of the number of bytes is finished
            nfc->i2c->hw->enable = false; ///< Disable the DW_apb_i2c
            // nfc->flags.B.dfifo = 1; ///< Activate the flag to get the data from the NFC FIFO
        }else {
            printf("Something went wrong on RX_FULL - NFC_I2C_CALLBACK \n");
        }
        
        break;

    default:
        printf("Happend what should not happens on I2C_HANDLER\n");
        break;
    }
}

void nfc_get_data_tag(nfc_rfid_t *nfc)
{
}

