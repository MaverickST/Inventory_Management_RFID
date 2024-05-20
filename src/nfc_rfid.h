/**
 * \file        nfc_rfid.h
 * \brief
 * \details
 * \author      MST_CDA
 * \version     0.0.1
 * \date        05/10/2023
 * \copyright   Unlicensed
 */

#ifndef __NFC_RFID_
#define __NFC_RFID_

#include <stdint.h>
#include "hardware/i2c.h"

#define ADDRESS_SLAVE_MFRC522 0x28 // 0b0101 -> 0010 1000

typedef struct
{
    struct
    {
        uint8_t id;
        uint32_t amount;
        uint32_t purchase_v;
        uint32_t sale_v;
    }tag;

    struct {
        enum {dev_ADDRESS, reg_ADDRESS, data_SENT} tx       :2; // 0: device address sent, 1: register address sent, 2: data sent
        enum {single_WRITE, mult_READ, single_READ} rw      :2; // 0: single write, 1: multiple read, 2: single read (number of fifo byte)
    }i2c_fifo_stat;

    struct {
        uint8_t sda;
        uint8_t scl;
        uint8_t irq;
    }pinout;

    union{
        uint8_t W;
        struct{
            uint8_t nbf     :1; ///< get the number of bytes in the fifo interrupt pending
            uint8_t dfifo   :1; ///< get the data from the nfc fifo interrupt pending
            uint8_t dtag    :1; ///< get the data tag from the nfc fifo interrupt pending
            uint8_t         :5;
        }B;
    }flags;

    uint8_t fifo[64]; ///< Data of the nfc fifo
    uint8_t nbf; ///< number of bytes in the nfc fifo (max 64)
    uint8_t idx_fifo; ///< Index of the nfc fifo
    uint8_t i2c_irq; ///< I2C IRQ number (23 or 24)
    i2c_inst_t *i2c; ///< I2C instance

    uint8_t version; ///< Version of the nfc

    enum {NONE, ADMIN, INV, USER} userType;
    
    
}nfc_rfid_t;

/**
 * @brief This function initializes the nfc_rfid_t structure
 * 
 * @param nfc 
 * @param _i2c i2c instance
 * @param sda gpio pin for the i2c sda
 * @param scl gpio pin for the i2c scl
 * @param irq gpio pin for the nfc irq
 */
void nfc_init_as_i2c(nfc_rfid_t *nfc, i2c_inst_t *_i2c, uint8_t sda, uint8_t scl, uint8_t irq);

/**
 * @brief Callback function for the I2C interruption, which is called by the I2C handler.
 * 
 * @param nfc 
 */
void nfc_i2c_callback(nfc_rfid_t *nfc);

/**
 * @brief From the nfc fifo, get the data tag.
 * 
 * @param nfc 
 */
void nfc_get_data_tag(nfc_rfid_t *nfc);

/**
 * @brief This function allows to configure the MFRC522 IRQ from initilization a sequence on the I2C bus.
 * 
 * @param nfc 
 */
static inline void nfc_config_mfrc522_irq(nfc_rfid_t *nfc)
{
    printf("Configuring the MFRC522 IRQ\n");
    irq_set_enabled(nfc->i2c_irq, true);
    nfc->i2c_fifo_stat.rw = single_WRITE; ///< Sigle write: configures de irq ping of the nfc
    nfc->i2c_fifo_stat.tx = dev_ADDRESS; 
    nfc->i2c->hw->enable = true; ///< Enable the DW_apb_i2c
    nfc->i2c->hw->data_cmd = ADDRESS_SLAVE_MFRC522; ///< Write the slave address to the DW_apb_i2c
}

/**
 * @brief This function allows to get the number of bytes in the NFC FIFO.
 * 
 * @param nfc 
 */
static inline void nfc_get_nbf(nfc_rfid_t *nfc)
{
    printf("Get number of bytes in the FIFO\n");
    irq_set_enabled(nfc->i2c_irq, true);
    nfc->i2c_fifo_stat.rw = single_READ; ///< Single read: read number of bytes of the FIFO
    nfc->i2c_fifo_stat.tx = dev_ADDRESS; 
    nfc->i2c->hw->enable = true; ///< Enable the DW_apb_i2c
    nfc->i2c->hw->data_cmd = ADDRESS_SLAVE_MFRC522; ///< Write the slave address to the DW_apb_i2c
}

/**
 * @brief This function allows to get the data from the NFC FIFO.
 * 
 * @param nfc 
 */

static inline void nfc_get_data_fifo(nfc_rfid_t *nfc)
{
    printf("Get data from the FIFO\n");
    irq_set_enabled(nfc->i2c_irq, true);
    nfc->i2c_fifo_stat.rw = mult_READ; // Multiple read
    nfc->i2c_fifo_stat.tx = dev_ADDRESS;
    nfc->i2c->hw->enable = true; ///< Enable the DW_apb_i2c
    nfc->i2c->hw->data_cmd = ADDRESS_SLAVE_MFRC522; ///< Write the slave address to the DW_apb_i2c
}

void nfc_config_blocking(nfc_rfid_t *nfc);


// MFRC522 registers. Described in chapter 9 of the datasheet.
// When using SPI all addresses are shifted one bit left in the "SPI address byte" (section 8.1.2.3)
enum {
    // Page 0: Command and status
    //						  0x00			// reserved for future use
    CommandReg				= 0x01,	// starts and stops command execution
    ComIEnReg				= 0x02,	// enable and disable interrupt request control bits
    DivIEnReg				= 0x03,	// enable and disable interrupt request control bits
    ComIrqReg				= 0x04,	// interrupt request bits
    DivIrqReg				= 0x05 << 1,	// interrupt request bits
    ErrorReg				= 0x06 << 1,	// error bits showing the error status of the last command executed 
    Status1Reg				= 0x07 << 1,	// communication status bits
    Status2Reg				= 0x08 << 1,	// receiver and transmitter status bits
    FIFODataReg				= 0x09,	// input and output of 64 byte FIFO buffer
    FIFOLevelReg			= 0x0A,	// number of bytes stored in the FIFO buffer
    WaterLevelReg			= 0x0B << 1,	// level for FIFO underflow and overflow warning
    ControlReg				= 0x0C << 1,	// miscellaneous control registers
    BitFramingReg			= 0x0D,	// adjustments for bit-oriented frames
    CollReg					= 0x0E << 1,	// bit position of the first bit-collision detected on the RF interface
    //						  0x0F			// reserved for future use
    
    // Page 1: Command
    // 						  0x10			// reserved for future use
    ModeReg					= 0x11 << 1,	// defines general modes for transmitting and receiving 
    TxModeReg				= 0x12 << 1,	// defines transmission data rate and framing
    RxModeReg				= 0x13 << 1,	// defines reception data rate and framing
    TxControlReg			= 0x14 << 1,	// controls the logical behavior of the antenna driver pins TX1 and TX2
    TxASKReg				= 0x15 << 1,	// controls the setting of the transmission modulation
    TxSelReg				= 0x16 << 1,	// selects the internal sources for the antenna driver
    RxSelReg				= 0x17 << 1,	// selects internal receiver settings
    RxThresholdReg			= 0x18 << 1,	// selects thresholds for the bit decoder
    DemodReg				= 0x19 << 1,	// defines demodulator settings
    // 						  0x1A			// reserved for future use
    // 						  0x1B			// reserved for future use
    MfTxReg					= 0x1C << 1,	// controls some MIFARE communication transmit parameters
    MfRxReg					= 0x1D << 1,	// controls some MIFARE communication receive parameters
    // 						  0x1E			// reserved for future use
    SerialSpeedReg			= 0x1F << 1,	// selects the speed of the serial UART interface
    
    // Page 2: Configuration
    // 						  0x20			// reserved for future use
    CRCResultRegH			= 0x21 << 1,	// shows the MSB and LSB values of the CRC calculation
    CRCResultRegL			= 0x22 << 1,
    // 						  0x23			// reserved for future use
    ModWidthReg				= 0x24 << 1,	// controls the ModWidth setting?
    // 						  0x25			// reserved for future use
    RFCfgReg				= 0x26 << 1,	// configures the receiver gain
    GsNReg					= 0x27 << 1,	// selects the conductance of the antenna driver pins TX1 and TX2 for modulation 
    CWGsPReg				= 0x28 << 1,	// defines the conductance of the p-driver output during periods of no modulation
    ModGsPReg				= 0x29 << 1,	// defines the conductance of the p-driver output during periods of modulation
    TModeReg				= 0x2A << 1,	// defines settings for the internal timer
    TPrescalerReg			= 0x2B << 1,	// the lower 8 bits of the TPrescaler value. The 4 high bits are in TModeReg.
    TReloadRegH				= 0x2C << 1,	// defines the 16-bit timer reload value
    TReloadRegL				= 0x2D << 1,
    TCounterValueRegH		= 0x2E << 1,	// shows the 16-bit timer value
    TCounterValueRegL		= 0x2F << 1,
    
    // Page 3: Test Registers
    // 						  0x30			// reserved for future use
    TestSel1Reg				= 0x31 << 1,	// general test signal configuration
    TestSel2Reg				= 0x32 << 1,	// general test signal configuration
    TestPinEnReg			= 0x33 << 1,	// enables pin output driver on pins D1 to D7
    TestPinValueReg			= 0x34 << 1,	// defines the values for D1 to D7 when it is used as an I/O bus
    TestBusReg				= 0x35 << 1,	// shows the status of the internal test bus
    AutoTestReg				= 0x36 << 1,	// controls the digital self-test
    VersionReg				= 0x37,	// shows the software version
    AnalogTestReg			= 0x38 << 1,	// controls the pins AUX1 and AUX2
    TestDAC1Reg				= 0x39 << 1,	// defines the test value for TestDAC1
    TestDAC2Reg				= 0x3A << 1,	// defines the test value for TestDAC2
    TestADCReg				= 0x3B << 1		// shows the value of ADC I and Q channels
    // 						  0x3C			// reserved for production tests
    // 						  0x3D			// reserved for production tests
    // 						  0x3E			// reserved for production tests
    // 						  0x3F			// reserved for production tests
};


#endif // __NFC_RFID_