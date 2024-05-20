/**
 * \file        nfc_enums.h
 * \brief
 * \details
 * \author      MST_CDA
 * \version     0.0.1
 * \date        05/10/2023
 * \copyright   Unlicensed
 */

#ifndef __NFC_ENUMS_
#define __NFC_ENUMS_

#define FIFO_SIZE 64		        ///< The FIFO is 64 bytes.

// Structure to store ISO/IEC 14443-4 ATS
typedef struct _Ats{
    uint8_t size;
    uint8_t fsc;                 // Frame size for proximity card

    struct {
        bool transmitted;
        bool        sameD;	// Only the same D for both directions supported
        uint8_t ds;		    // Send D (TagBitRates)
        uint8_t dr;		    // Receive D (TagBitRates)
    } ta1;

    struct {
        bool transmitted;
        uint8_t fwi;			// Frame waiting time integer
        uint8_t sfgi;			// Start-up frame guard time integer
    } tb1;

    struct {
        bool transmitted;
        bool supportsCID;
        bool supportsNAD;
    } tc1;

    // Raw data from ATS
    uint8_t data[FIFO_SIZE - 2]; // ATS cannot be bigger than FSD - 2 bytes (CRC), according to ISO 14443-4 5.2.2
} Ats;

// A struct used for passing the UID of a PICC.
typedef struct {
    uint8_t		size;			// Number of bytes in the UID. 4, 7 or 10.
    uint8_t		uidByte[10];
    uint8_t		sak;			// The SAK (Select acknowledge) byte returned from the PICC after successful selection.
} Uid;

// A struct used for passing the PICC information
typedef struct _TagInfo{
    uint16_t	atqa;
    Uid			uid;
    Ats		    ats; 

    // For Block PCB
    bool blockNumber;
} TagInfo;


// --------------------------------------------------------------------------------------------------
// ---------------------------------- NFC ENUMS ----------------------------------------------------
// --------------------------------------------------------------------------------------------------

// MFRC522 registers. Described in chapter 9 of the datasheet.
// When using SPI all addresses are shifted one bit left in the "SPI address byte" (section 8.1.2.3)
enum PCD_Register{
    // Page 0: Command and status
    //						  0x00			// reserved for future use
    CommandReg				= 0x01,	// starts and stops command execution
    ComIEnReg				= 0x02,	// enable and disable interrupt request control bits
    DivIEnReg				= 0x03,	// enable and disable interrupt request control bits
    ComIrqReg				= 0x04,	// interrupt request bits
    DivIrqReg				= 0x05,	// interrupt request bits
    ErrorReg				= 0x06,	// error bits showing the error status of the last command executed 
    Status1Reg				= 0x07,	// communication status bits
    Status2Reg				= 0x08,	// receiver and transmitter status bits
    FIFODataReg				= 0x09,	// input and output of 64 byte FIFO buffer
    FIFOLevelReg			= 0x0A,	// number of bytes stored in the FIFO buffer
    WaterLevelReg			= 0x0B,	// level for FIFO underflow and overflow warning
    ControlReg				= 0x0C,	// miscellaneous control registers
    BitFramingReg			= 0x0D,	// adjustments for bit-oriented frames
    CollReg					= 0x0E,	// bit position of the first bit-collision detected on the RF interface
    //						  0x0F			// reserved for future use
    
    // Page 1: Command
    // 						  0x10			// reserved for future use
    ModeReg					= 0x11,	// defines general modes for transmitting and receiving 
    TxModeReg				= 0x12,	// defines transmission data rate and framing
    RxModeReg				= 0x13,	// defines reception data rate and framing
    TxControlReg			= 0x14,	// controls the logical behavior of the antenna driver pins TX1 and TX2
    TxASKReg				= 0x15,	// controls the setting of the transmission modulation
    TxSelReg				= 0x16,	// selects the internal sources for the antenna driver
    RxSelReg				= 0x17,	// selects internal receiver settings
    RxThresholdReg			= 0x18,	// selects thresholds for the bit decoder
    DemodReg				= 0x19,	// defines demodulator settings
    // 						  0x1A			// reserved for future use
    // 						  0x1B			// reserved for future use
    MfTxReg					= 0x1C,	// controls some MIFARE communication transmit parameters
    MfRxReg					= 0x1D,	// controls some MIFARE communication receive parameters
    // 						  0x1E			// reserved for future use
    SerialSpeedReg			= 0x1F,	// selects the speed of the serial UART interface
    
    // Page 2: Configuration
    // 						  0x20			// reserved for future use
    CRCResultRegH			= 0x21,	// shows the MSB and LSB values of the CRC calculation
    CRCResultRegL			= 0x22,
    // 						  0x23			// reserved for future use
    ModWidthReg				= 0x24,	// controls the ModWidth setting?
    // 						  0x25			// reserved for future use
    RFCfgReg				= 0x26,	// configures the receiver gain
    GsNReg					= 0x27,	// selects the conductance of the antenna driver pins TX1 and TX2 for modulation 
    CWGsPReg				= 0x28,	// defines the conductance of the p-driver output during periods of no modulation
    ModGsPReg				= 0x29,	// defines the conductance of the p-driver output during periods of modulation
    TModeReg				= 0x2A,	// defines settings for the internal timer
    TPrescalerReg			= 0x2B,	// the lower 8 bits of the TPrescaler value. The 4 high bits are in TModeReg.
    TReloadRegH				= 0x2C,	// defines the 16-bit timer reload value
    TReloadRegL				= 0x2D,
    TCounterValueRegH		= 0x2E,	// shows the 16-bit timer value
    TCounterValueRegL		= 0x2F,
    
    // Page 3: Test Registers
    // 						  0x30			// reserved for future use
    TestSel1Reg				= 0x31,	// general test signal configuration
    TestSel2Reg				= 0x32,	// general test signal configuration
    TestPinEnReg			= 0x33,	// enables pin output driver on pins D1 to D7
    TestPinValueReg			= 0x34,	// defines the values for D1 to D7 when it is used as an I/O bus
    TestBusReg				= 0x35,	// shows the status of the internal test bus
    AutoTestReg				= 0x36,	// controls the digital self-test
    VersionReg				= 0x37,	// shows the software version
    AnalogTestReg			= 0x38,	// controls the pins AUX1 and AUX2
    TestDAC1Reg				= 0x39,	// defines the test value for TestDAC1
    TestDAC2Reg				= 0x3A,	// defines the test value for TestDAC2
    TestADCReg				= 0x3B		// shows the value of ADC I and Q channels
    // 						  0x3C			// reserved for production tests
    // 						  0x3D			// reserved for production tests
    // 						  0x3E			// reserved for production tests
    // 						  0x3F			// reserved for production tests
};

// Commands sent to the PICC.
enum PICC_Command{
    // The commands used by the PCD to manage communication with several PICCs (ISO 14443-3, Type A, section 6.4)
    PICC_CMD_REQA			= 0x26,		// REQuest command, Type A. Invites PICCs in state IDLE to go to READY and prepare for anticollision or selection. 7 bit frame.
    PICC_CMD_WUPA			= 0x52,		// Wake-UP command, Type A. Invites PICCs in state IDLE and HALT to go to READY(*) and prepare for anticollision or selection. 7 bit frame.
    PICC_CMD_CT				= 0x88,		// Cascade Tag. Not really a command, but used during anti collision.
    PICC_CMD_SEL_CL1		= 0x93,		// Anti collision/Select, Cascade Level 1
    PICC_CMD_SEL_CL2		= 0x95,		// Anti collision/Select, Cascade Level 2
    PICC_CMD_SEL_CL3		= 0x97,		// Anti collision/Select, Cascade Level 3
    PICC_CMD_HLTA			= 0x50,		// HaLT command, Type A. Instructs an ACTIVE PICC to go to state HALT.
    PICC_CMD_RATS           = 0xE0,     // Request command for Answer To Reset.
    // The commands used for MIFARE Classic (from http://www.mouser.com/ds/2/302/MF1S503x-89574.pdf, Section 9)
    // Use PCD_MFAuthent to authenticate access to a sector, then use these commands to read/write/modify the blocks on the sector.
    // The read/write commands can also be used for MIFARE Ultralight.
    PICC_CMD_MF_AUTH_KEY_A	= 0x60,		// Perform authentication with Key A
    PICC_CMD_MF_AUTH_KEY_B	= 0x61,		// Perform authentication with Key B
    PICC_CMD_MF_READ		= 0x30,		// Reads one 16 byte block from the authenticated sector of the PICC. Also used for MIFARE Ultralight.
    PICC_CMD_MF_WRITE		= 0xA0,		// Writes one 16 byte block to the authenticated sector of the PICC. Called "COMPATIBILITY WRITE" for MIFARE Ultralight.
    PICC_CMD_MF_DECREMENT	= 0xC0,		// Decrements the contents of a block and stores the result in the internal data register.
    PICC_CMD_MF_INCREMENT	= 0xC1,		// Increments the contents of a block and stores the result in the internal data register.
    PICC_CMD_MF_RESTORE		= 0xC2,		// Reads the contents of a block into the internal data register.
    PICC_CMD_MF_TRANSFER	= 0xB0,		// Writes the contents of the internal data register to a block.
    // The commands used for MIFARE Ultralight (from http://www.nxp.com/documents/data_sheet/MF0ICU1.pdf, Section 8.6)
    // The PICC_CMD_MF_READ and PICC_CMD_MF_WRITE can also be used for MIFARE Ultralight.
    PICC_CMD_UL_WRITE		= 0xA2		// Writes one 4 byte page to the PICC.
};

// Return codes from the functions in this class. Remember to update GetStatusCodeName() if you add more.
// last value set to 0xff, then compiler uses less ram, it seems some optimisations are triggered
enum StatusCode{
    STATUS_OK				,	// Success
    STATUS_ERROR			,	// Error in communication
    STATUS_COLLISION		,	// Collission detected
    STATUS_TIMEOUT			,	// Timeout in communication.
    STATUS_NO_ROOM			,	// A buffer is not big enough.
    STATUS_INTERNAL_ERROR	,	// Internal error in the code. Should not happen ;-)
    STATUS_INVALID			,	// Invalid argument.
    STATUS_CRC_WRONG		,	// The CRC_A does not match
    STATUS_MIFARE_NACK		= 0xff	// A MIFARE PICC responded with NAK.
};

// MFRC522 commands. Described in chapter 10 of the datasheet.
enum PCD_Command {
    PCD_Idle				= 0x00,		// no action, cancels current command execution
    PCD_Mem					= 0x01,		// stores 25 bytes into the internal buffer
    PCD_GenerateRandomID	= 0x02,		// generates a 10-byte random ID number
    PCD_CalcCRC				= 0x03,		// activates the CRC coprocessor or performs a self-test
    PCD_Transmit			= 0x04,		// transmits data from the FIFO buffer
    PCD_NoCmdChange			= 0x07,		// no command change, can be used to modify the CommandReg register bits without affecting the command, for example, the PowerDown bit
    PCD_Receive				= 0x08,		// activates the receiver circuits
    PCD_Transceive 			= 0x0C,		// transmits data from FIFO buffer to antenna and automatically activates the receiver after transmission
    PCD_MFAuthent 			= 0x0E,		// performs the MIFARE standard authentication as a reader
    PCD_SoftReset			= 0x0F		// resets the MFRC522
};
// ISO/IEC 14443-4 bit rates
enum TagBitRates {
    BITRATE_106KBITS = 0x00,
    BITRATE_212KBITS = 0x01,
    BITRATE_424KBITS = 0x02,
    BITRATE_848KBITS = 0x03
};


#endif // __NFC_ENUMS_