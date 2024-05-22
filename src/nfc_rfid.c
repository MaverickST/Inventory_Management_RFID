/**
 * \file        nfc_rfid.c
 * \brief
 * \details
 * \author      MST_CDA
 * \version     0.0.1
 * \date        05/10/2023
 * \copyright   Unlicensed
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/binary_info.h"
#include "pico/time.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"

#include "nfc_rfid.h"
#include "functs.h"

void nfc_init_as_i2c(nfc_rfid_t *nfc, i2c_inst_t *_i2c, uint8_t sda, uint8_t scl, uint8_t irq, uint8_t rst)
{
    nfc->i2c = _i2c;
    nfc->pinout.sda = sda;
    nfc->pinout.scl = scl;
    nfc->pinout.irq = irq;
    nfc->pinout.rst = rst;
    nfc->i2c_fifo_stat.tx = 0;
    nfc->nbf = 0;
    nfc->idx_fifo = 0;
    nfc->userType = INV;
    nfc->flags.W = 0;
    nfc->timeCheck = 1000000; ///< 1s = 1000000 us
    nfc->timer_irq = TIMER_IRQ_1;

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

    // Reset configuration
    gpio_init(rst);
    gpio_set_dir(rst, GPIO_IN);
    if (gpio_get(rst) == 0){ ///< The MFRC522 chip is in power down mode.
        gpio_set_dir(rst, GPIO_OUT); ///< Now set the resetPowerDownPin as digital output.
        gpio_put(rst, 0); ///< Make sure we have a clean LOW state.
        sleep_us(2);
        gpio_put(rst, 1); ///< Exit power down mode. This triggers a hard reset
        // Section 8.8.2 in the datasheet says the oscillator start-up time is the start up time 
        // of the crystal + 37,74μs. Let us be generous: 50ms.
        sleep_ms(50);
    }else { ///< Perform a soft reset
        printf("Performing a soft reset\n");
        nfc_reset(nfc);
    }

    // Reset baud rates
    nfc_write_blocking(nfc, TxModeReg, 0x00);
    nfc_write_blocking(nfc, RxModeReg, 0x00);
    // Reset ModWidthReg
    nfc_write_blocking(nfc, ModWidthReg, 0x26);

    // When communicating with a PICC we need a timeout if something goes wrong.
	// f_timer = 13.56 MHz / (2*TPreScaler+1) where TPreScaler = [TPrescaler_Hi:TPrescaler_Lo].
	// TPrescaler_Hi are the four low bits in TModeReg. TPrescaler_Lo is TPrescalerReg.
    nfc_write_blocking(nfc, TModeReg, 0x80); ///< TAuto=1; timer starts automatically at the end of the transmission in all communication modes at all speeds
    nfc_write_blocking(nfc, TPrescalerReg, 0xA9); // TPreScaler = TModeReg[3..0]:TPrescalerReg, ie 0x0A9 = 169 => f_timer=40kHz, ie a timer period of 25μs.
    nfc_write_blocking(nfc, TReloadRegH, 0x03); ///< Reload timer with 0x3E8 = 1000, ie 25ms before timeout.
    nfc_write_blocking(nfc, TReloadRegL, 0xE8);

    nfc_write_blocking(nfc, TxASKReg, 0x40); ///< Default 0x00. Force a 100 % ASK modulation independent of the ModGsPReg register setting
    nfc_write_blocking(nfc, ModeReg, 0x3D); // Default 0x3F. Set the preset value for the CRC coprocessor for the CalcCRC command to 0x6363 (ISO 14443-3 part 6.2.4)
    
    nfc_antenna_on(nfc); ///< Enable the antenna

    // Prepare the key (used both as key A and as key B)
    // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
    for (uint8_t i = 0; i < 6; i++) {
        nfc->keyByte[i] = 0xFF;
    }

    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));

    // Initialize the configuration of the MFRC522 (comment or uncomment the desired configuration)
    // nfc_config_mfrc522_irq(nfc); // IRQ's configuration
    // nfc_config_blocking(nfc);    // Blocking configuration

}

bool nfc_is_new_tag(nfc_rfid_t *nfc)
{
    uint8_t bufferATQA[2];
	uint8_t bufferSize = sizeof(bufferATQA);

    // Reset baud rates
    nfc_write_blocking(nfc, TxModeReg, 0x00);
    nfc_write_blocking(nfc, RxModeReg, 0x00);
    // Reset ModWidthReg
    nfc_write_blocking(nfc, ModWidthReg, 0x26);
    printf("Checking...\n");

    uint8_t result = nfc_requestA(nfc, bufferATQA, &bufferSize);

    printf("Result: %d\n", result);

    if (result == STATUS_OK || result == STATUS_COLLISION) {
        nfc->tagInfo.atqa = (bufferATQA[1] << 8) | bufferATQA[0];
        nfc->tagInfo.ats.size = 0;
        nfc->tagInfo.ats.fsc = 32; // default FSC value for ISO14443A

        // Defaults for TA1
        nfc->tagInfo.ats.ta1.transmitted = false;
		nfc->tagInfo.ats.ta1.sameD = false;
		nfc->tagInfo.ats.ta1.ds = BITRATE_106KBITS;
		nfc->tagInfo.ats.ta1.dr = BITRATE_106KBITS;

		// Defaults for TB1
		nfc->tagInfo.ats.tb1.transmitted = false;
		nfc->tagInfo.ats.tb1.fwi = 0;	// TODO: Don't know the default for this!
		nfc->tagInfo.ats.tb1.sfgi = 0;	// The default value of SFGI is 0 (meaning that the card does not need any particular SFGT)

		// Defaults for TC1
		nfc->tagInfo.ats.tc1.transmitted = false;
		nfc->tagInfo.ats.tc1.supportsCID = true;
		nfc->tagInfo.ats.tc1.supportsNAD = false;

        // Memset() converts the value ch to unsigned char and copies it into each of the first n characters of the object pointed to by str[]
		memset(nfc->tagInfo.ats.data, 0, FIFO_SIZE - 2);

		nfc->tagInfo.blockNumber = false;

        return true;
    }
    return false;
}

uint8_t nfc_communicate(nfc_rfid_t *nfc, uint8_t command, uint8_t waitIRq, uint8_t *sendData, uint8_t sendLen, 
                        uint8_t *backData, uint8_t *backLen, uint8_t *validBits, uint8_t rxAlign, bool checkCRC)
{
    // Prepare values for BitFramingReg
    uint8_t txLastBits = validBits ? *validBits : 0;
    uint8_t bitFraming = (rxAlign << 4) + txLastBits; // RxAlign = BitFramingReg[6..4]. TxLastBits = BitFramingReg[2..0]

    nfc_write_blocking(nfc, CommandReg, PCD_Idle);  // Stop any active command.
    nfc_write_blocking(nfc, ComIrqReg, 0x7F);       // Clear all seven interrupt request bits
    nfc_write_blocking(nfc, FIFOLevelReg, 0x80);    // FlushBuffer = 1, FIFO initialization
    nfc_write_mult_blocking(nfc, FIFODataReg, sendData, sendLen); // Write sendData to the FIFO
    nfc_write_blocking(nfc, BitFramingReg, bitFraming); // Bit adjustments
    nfc_write_blocking(nfc, CommandReg, command); // Execute the command
    if (command == PCD_Transceive) {
        nfc_set_reg_bitmask(nfc, BitFramingReg, 0x80); // StartSend=1, transmission of data starts
    }

    // In PCD_Init() we set the TAuto flag in TModeReg. This means the timer
	// automatically starts when the PCD stops transmitting.
	//
	// Wait here for the command to complete. The bits specified in the
	// `waitIRq` parameter define what bits constitute a completed command.
	// When they are set in the ComIrqReg register, then the command is
	// considered complete. If the command is not indicated as complete in
	// ~36ms, then consider the command as timed out.

    const uint32_t deadline = time_us_32()/1000 + 36; // 36 ms, 1 ms is 1000 us
    bool completed = false;

    do {
        uint8_t n;
        nfc_read_blocking(nfc, ComIrqReg, &n); ///< ComIrqReg[7..0] bits are: Set1 TxIRq RxIRq IdleIRq HiAlertIRq LoAlertIRq ErrIRq TimerIRq
        if (n & waitIRq) { ///< One of the interrupts that signal success has been set.
            completed = true;
            break;
        }
        if (n & 0x01) { ///< Timer interrupt - nothing received in 25ms
            return STATUS_TIMEOUT;
        }
    } while (time_us_32()/1000 < deadline);

    // 36ms and nothing happened. Communication with the MFRC522 might be down.
    if (!completed) { ///< The command hasn't completed in 36ms
        return STATUS_TIMEOUT;
    }

    // Stop now if any errors except collisions were detected.
    uint8_t errorRegValue;
    nfc_read_blocking(nfc, ErrorReg, &errorRegValue); ///< ErrorReg[7..0] bits are: WrErr TempErr reserved BufferOvfl CollErr CRCErr ParityErr ProtocolErr
    if (errorRegValue & 0x13) { ///< BufferOvfl ParityErr ProtocolErr
        return STATUS_ERROR;
    }

    uint8_t _validBits = 0;

    // If the caller wants data back, get it from the MFRC522.
    if (backData && backLen) {
        uint8_t n;
        nfc_read_blocking(nfc, FIFOLevelReg, &n); ///< Number of bytes in the FIFO
        printf("Comunicate - n: %d, backLen: %d\n", n, *backLen);
        if (n > *backLen) {
            return STATUS_NO_ROOM;
        }
        *backLen = n; ///< Number of bytes returned
        nfc_read_mult_blocking(nfc, FIFODataReg, backData, n, rxAlign); ///< Get received data from FIFO
        nfc_read_blocking(nfc, ControlReg, &_validBits); ///< RxLastBits[2:0] indicates the number of valid bits in the last received byte. If this value is 000b, the whole byte is valid.
        _validBits = _validBits & 0x07;
        if (validBits) {
            *validBits = _validBits;
        }
    } 

    // Tell about collisions
    if (errorRegValue & 0x08) { ///< CollErr
        return STATUS_COLLISION;
    }

    // Perform CRC_A validation if requested.
    if (backData && backLen && checkCRC) {
        // In this case a MIFARE Classic NAK is not OK.
        if (*backLen == 1 && _validBits == 4) {
            return STATUS_MIFARE_NACK;
        }
        // We need at least the CRC_A value and all 8 bits of the last byte must be received.
        if (*backLen < 2 || _validBits != 0) {
            return STATUS_CRC_WRONG;
        }
        // Verify CRC_A - do our own calculation and store the control in controlBuffer.
        uint8_t controlBuffer[2];
        uint8_t status = nfc_calculate_crc(nfc, &backData[0], *backLen - 2, &controlBuffer[0]); // (it is not done)
        if (status != STATUS_OK) {
            return status;
        }
        if ((backData[*backLen - 2] != controlBuffer[0]) || (backData[*backLen - 1] != controlBuffer[1])) {
            return STATUS_CRC_WRONG;
        }
    }

    return STATUS_OK;
} // End of nfc_communicate

uint8_t nfc_requestA_or_wakeupA(nfc_rfid_t *nfc, uint8_t command, uint8_t *bufferATQA, uint8_t *bufferSize)
{
    uint8_t validBits;
    uint8_t status;

    if (bufferATQA == NULL || *bufferSize < 2) { // The ATQA response is 2 bytes long.
        return STATUS_NO_ROOM;
    }
    nfc_clear_reg_bitmask(nfc, CollReg, 0x80); // ValuesAfterColl=1 => Bits received after collision are cleared.
    validBits = 7;
    status = nfc_transceive_data(nfc, &command, 1, bufferATQA, bufferSize, &validBits, 0, false);
    if (status != STATUS_OK) {
        return status;
    }
    if (*bufferSize != 2 || validBits != 0) { // ATQA must be exactly 16 bits.
        return STATUS_ERROR;
    }

    return STATUS_OK;
} // End of nfc_requestA_or_wakeupA

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

uint8_t nfc_calculate_crc(nfc_rfid_t *nfc, uint8_t *data, uint8_t len, uint8_t *result)
{
    return 0;
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

