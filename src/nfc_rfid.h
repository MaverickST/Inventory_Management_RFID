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
#include "hardware/spi.h"

#include "nfc_enums.h"

#define ADDRESS_SLAVE_MFRC522 0x28  ///< 0b0101 -> 0010 1000
#define MF_KEY_SIZE             6	///< A Mifare Crypto1 key is 6 bytes.
#define READ_BIT 0x80 ///< Bit used in I2C to read a register

/**
 * \typedef nfc_rfic_t
 * \brief Data strcuture to manage the NFC RFID device.
 */
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
        uint8_t rst;
        uint8_t sck;
        uint8_t mosi;
        uint8_t miso;
        uint8_t cs;
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
	
    uint8_t keyByte[MF_KEY_SIZE]; ///< Mifare Crypto1 key	
    TagInfo tagInfo; ///< Tag information
    uint32_t timeCheck; ///< Time check (1s)
    uint8_t timer_irq; ///< Alarm timer IRQ number (TIMER_IRQ_1)

    uint8_t fifo[64]; ///< Data of the nfc fifo
    uint8_t nbf; ///< number of bytes in the nfc fifo (max 64)
    uint8_t idx_fifo; ///< Index of the nfc fifo
    uint8_t i2c_irq; ///< I2C IRQ number (23 or 24)
    uint8_t spi_irq; ///< SPI IRQ number (25 or 26)
    i2c_inst_t *i2c; ///< I2C instance
    spi_inst_t *spi; ///< SPI instance

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
 * @param rst gpio pin for the nfc rst
 */
void nfc_init_as_i2c(nfc_rfid_t *nfc, i2c_inst_t *_i2c, uint8_t sda, uint8_t scl, uint8_t irq, uint8_t rst);

/**
 * @brief This function initializes the nfc_rfid_t structure as SPI
 * 
 * @param nfc 
 * @param _spi 
 * @param sck 
 * @param mosi 
 * @param miso 
 * @param cs 
 * @param irq 
 * @param rst 
 */
void nfc_init_as_spi(nfc_rfid_t *nfc, spi_inst_t *_spi, uint8_t sck, uint8_t mosi, uint8_t miso, uint8_t cs, uint8_t irq, uint8_t rst);

/**
 * @brief This function tell us if there is a new tag in the NFC.
 * Returns true if a PICC responds to PICC_CMD_REQA.
 * Only "new" cards in state IDLE are invited. Sleeping cards in state HALT are ignored.
 * 
 * @param nfc 
 * @return true 
 * @return false 
 */
bool nfc_is_new_tag(nfc_rfid_t *nfc);

/**
 * @brief Transfers data to the MFRC522 FIFO, executes a command, waits for completion and transfers data back from the FIFO.
 * CRC validation can only be done if backData and backLen are specified.
 * 
 * @param nfc 
 * @param command   ///< The command to execute. One of the PCD_Command enums.
 * @param waitIRq   ///< The bits in the ComIrqReg register that signals successful completion of the command.
 * @param sendData  ///< Pointer to the data to transfer to the FIFO.
 * @param sendLen   ///< Number of bytes to transfer to the FIFO.
 * @param backData  ///< nullptr or pointer to buffer if data should be read back after executing the command.
 * @param backLen   ///< In: Max number of bytes to write to *backData. Out: The number of bytes returned.
 * @param validBits ///< In/Out: The number of valid bits in the last byte. 0 for 8 valid bits.
 * @param rxAlign   ///< In: Defines the bit position in backData[0] for the first bit received. Default 0.
 * @param checkCRC  ///< In: True => The last two bytes of the response is assumed to be a CRC_A that must be validated.
 * @return uint8_t  ///< STATUS_OK on success, STATUS_??? otherwise.
 */
uint8_t nfc_communicate(nfc_rfid_t *nfc, uint8_t command, uint8_t waitIRq, uint8_t *sendData, uint8_t sendLen, uint8_t *backData, 
                            uint8_t *backLen, uint8_t *validBits, uint8_t rxAlign, bool checkCRC);

/**
 * @brief Executes the Transceive command.
 * CRC validation can only be done if backData and backLen are specified.
 * 
 * @param nfc 
 * @param sendData  ///< Pointer to the data to transfer to the FIFO.
 * @param sendLen   ///< Number of bytes to transfer to the FIFO.
 * @param backData  ///< nullptr or pointer to buffer if data should be read back after executing the command.
 * @param backLen   ///< In: Max number of bytes to write to *backData. Out: The number of bytes returned.
 * @param validBits ///< In/Out: The number of valid bits in the last byte. 0 for 8 valid bits. Default nullptr.
 * @param rxAlign   ///< In: Defines the bit position in backData[0] for the first bit received. Default 0.
 * @param checkCRC  ///< In: True => The last two bytes of the response is assumed to be a CRC_A that must be validated.
 * @return uint8_t 
 */
static inline uint8_t nfc_transceive_data(nfc_rfid_t *nfc, uint8_t *sendData, uint8_t sendLen, uint8_t *backData, 
                            uint8_t *backLen, uint8_t *validBits, uint8_t rxAlign, bool checkCRC)
{
    uint8_t waitIRq = 0x30; // RxIRq and IdleIRq
    return nfc_communicate(nfc, (uint8_t)PCD_Transceive, waitIRq, sendData, sendLen, backData, backLen, validBits, rxAlign, checkCRC);
}

/**
 * @brief Transmits REQA or WUPA commands.
 * 
 * @param nfc 
 * @param command ///< The command to send - PICC_CMD_REQA or PICC_CMD_WUPA
 * @param bufferATQA ///< The buffer to store the ATQA (Answer to request) in
 * @param bufferSize ///< Buffer size, at least two bytes. Also number of bytes returned if STATUS_OK.
 * @return ///< uint8_t StatusCode
 */
uint8_t nfc_requestA_or_wakeupA(nfc_rfid_t *nfc, uint8_t command, uint8_t *bufferATQA, uint8_t *bufferSize);

/**
 * @brief Transmits a REQuest command, Type A. 
 * Invites PICCs in state IDLE to go to READY and prepare for anticollision or selection. 7 bit frame.
 * 
 * @param nfc 
 * @param bufferATQA ///< The buffer to store the ATQA (Answer to request) in
 * @param bufferSize ///< Buffer size, at least two bytes. Also number of bytes returned if STATUS_OK.
 * @return uint8_t  StatusCode
 */
static inline uint8_t nfc_requestA(nfc_rfid_t *nfc, uint8_t *bufferATQA, uint8_t *bufferSize)
{
    return nfc_requestA_or_wakeupA(nfc, (uint8_t)PICC_CMD_REQA, bufferATQA, bufferSize);
}

/**
 * @brief Transmits a Wake-UP command, Type A. 
 * Invites PICCs in state IDLE and HALT to go to READY(*) and prepare for anticollision or selection. 7 bit frame.
 * 
 * @param nfc 
 * @param bufferATQA ///< The buffer to store the ATQA (Answer to request) in
 * @param bufferSize ///< Buffer size, at least two bytes. Also number of bytes returned if STATUS_OK.
 * @return uint8_t  StatusCode
 */
static inline uint8_t nfc_wakeupA(nfc_rfid_t *nfc, uint8_t *bufferATQA, uint8_t *bufferSize)
{
    return nfc_requestA_or_wakeupA(nfc, (uint8_t)PICC_CMD_WUPA, bufferATQA, bufferSize);
}

/**
 * @brief // Select slave
 * 
 * @param nfc 
 */
static inline void cs_select(nfc_rfid_t *nfc) {
    asm volatile("nop \n nop \n nop");
    gpio_put(nfc->pinout.cs, 0);  // Active low
    asm volatile("nop \n nop \n nop");
}

/**
 * @brief // Deselect slave
 * 
 * @param nfc 
 */
static inline void cs_deselect(nfc_rfid_t *nfc) {
    asm volatile("nop \n nop \n nop");
    gpio_put(nfc->pinout.cs, 1);
    asm volatile("nop \n nop \n nop");
}

/**
 * @brief Perform a write operation to the NFC.
 * 
 * @param nfc 
 * @param reg 
 * @param data 
 */
static inline void nfc_write(nfc_rfid_t *nfc, uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    cs_select(nfc);
    spi_write_blocking(nfc->spi, buf, 2);
    cs_deselect(nfc);
}

/**
 * @brief Perform multiple write operations to the NFC.
 * 
 * @param nfc 
 * @param reg 
 * @param data 
 * @param len 
 */
static inline void nfc_write_mult(nfc_rfid_t *nfc, uint8_t reg, uint8_t *data, uint8_t len)
{
    uint8_t buf[64] = {reg};
    for (int i = 0; i < len; i++)
    {
        buf[i+1] = data[i];
    }
    cs_select(nfc);
    spi_write_blocking(nfc->spi, buf, len+1);
    cs_deselect(nfc);
}

/**
 * @brief Perform a read operation to the NFC.
 * 
 * @param nfc 
 * @param reg 
 * @param data 
 */
static inline void nfc_read(nfc_rfid_t *nfc, uint8_t reg, uint8_t *data)
{
    reg = reg | READ_BIT;
    cs_select(nfc);
    spi_write_blocking(nfc->spi, &reg, 1);
    sleep_ms(10);
    spi_read_blocking(nfc->spi, 0, data, 1);
    cs_deselect(nfc);
}

/**
 * @brief Perform multiple read operations to the NFC.
 * 
 * @param nfc 
 * @param reg 
 * @param data 
 * @param len 
 * @param rxAlign ///< Only bit positions rxAlign..7 in values[0] are updated. Default 0.
 */ 
static inline void nfc_read_mult(nfc_rfid_t *nfc, uint8_t reg, uint8_t *data, uint8_t len, uint8_t rxAlign)
{
    reg = reg | READ_BIT;
    cs_select(nfc);
    spi_write_blocking(nfc->spi, &reg, 1);
    sleep_ms(10);
    spi_read_blocking(nfc->spi, 0, data, len);
    cs_deselect(nfc);
    if (rxAlign)
    {
        uint8_t mask = (0xFF << rxAlign) & 0xFF;
        data[0] = (data[0] & ~mask) | (data[1] & mask);
    }
}

/**
 * @brief Clears the bits given in mask from register reg.
 * 
 * @param nfc 
 * @param reg 
 * @param mask ///< The bits to clear.
 */
static inline void nfc_clear_reg_bitmask(nfc_rfid_t *nfc, uint8_t reg, uint8_t mask)
{
    uint8_t value;
    nfc_read(nfc, reg, &value);
    sleep_ms(10);
    nfc_write(nfc, reg, (uint8_t)(value & (~mask)));
}

/**
 * @brief Sets the bits given in mask in register reg.
 * 
 * @param nfc 
 * @param reg 
 * @param mask 
 */
static inline void nfc_set_reg_bitmask(nfc_rfid_t *nfc, uint8_t reg, uint8_t mask)
{
    uint8_t value;
    nfc_read(nfc, reg, &value);
    sleep_ms(10);
    nfc_write(nfc, reg, (uint8_t)(value | mask));
}

/**
 * @brief Turn on the NFC antenna.
 * Turns the antenna on by enabling pins TX1 and TX2.
 * After a reset these pins are disabled.
 * 
 * @param nfc 
 */
static inline void nfc_antenna_on(nfc_rfid_t *nfc)
{
    uint8_t value;
    nfc_read(nfc, TxControlReg, &value);
    if ((value & 0x03) != 0x03)
    {
        nfc_write(nfc, TxControlReg, value | 0x03);
    }
}

/**
 * @brief Performs a soft reset on the MFRC522 chip and waits for it to be ready again.
 * 
 * @param nfc 
 */
static inline void nfc_reset(nfc_rfid_t *nfc)
{
    nfc_write(nfc, CommandReg, PCD_SoftReset);
    uint8_t count = 0;
    uint8_t value;
    do {
        sleep_ms(1);
        nfc_read(nfc, CommandReg, &value);
    } while ((value & (1<<4)) && ((++count) < 3));
}

/**
 * @brief Use the CRC coprocessor in the MFRC522 to calculate a CRC_A.
 * 
 * @param nfc 
 * @param data 
 * @param len 
 * @param result 
 * @return uint8_t STATUS_OK on success, STATUS_??? otherwise.
 */
uint8_t nfc_calculate_crc(nfc_rfid_t *nfc, uint8_t *data, uint8_t len, uint8_t *result);

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



#endif // __NFC_RFID_