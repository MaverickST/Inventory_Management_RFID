/**
 * \file        liquid_crystal_i2c.h
 * \brief       Liquid Crystal I2C library for use with raspberry pi pico
 * \details     This library is used to control a MxN LCD display using the I2C protocol
 * \author      CDA and MST
 * \version     0.0.1
 * \date        05/05/2024
 * \copyright   Unlicensed
 */

#ifndef __LIQUID_CRYSTAL_I2C_H__
#define __LIQUID_CRYSTAL_I2C_H__

#include <stdint.h>
#include "pico.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
//#include "pico/binary_info.h"


/**
 * \addtogroup LCD constants
 * @{
 * \brief Constants for the LCD display
 */

// commands
#define LCD_CLEAR_DISPLAY 0x01u
#define LCD_RETURN_HOME 0x02u
#define LCD_ENTRY_MODE_NO_SHIFT 0x00u
#define LCD_ENTRYMODESET 0x04
#define LCD_ENTRYMODEINCREMENT 0x02
#define LCD_DISPLAY_CONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTION_SET 0x20u
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYLEFT 0x02

// flags for display and cursor control
#define LCD_BLINK_ON 0x01
#define LCD_BLINK_OFF 0x00
#define LCD_CURSOR_OFF 0x00
#define LCD_CURSORON 0x02
#define LCD_DISPLAY_OFF 0x00
#define LCD_DISPLAY_ON 0x04
#define LCD_BLINK_ON 0x01
#define LCD_BLINK_OFF 0x00

// flags for display and cursor shift
#define LCD_MOVECURSOR 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_DISPLAYMOVE 0x08

// flags for function set
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00
#define LCD_2LINE 0x08
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00

// flag for backlight control
#define LCD_BACKLIGHT 0x08

#define LCD_ENABLE_BIT 0x04

// Modes for lcd_send_byte
#define LCD_CHARACTER 0x01
#define LCD_COMMAND 0x00

/** @} */

/**
 * \typedef lcd_t
 * \brief Data structure to manage a MxN LCD display
 * 
 */
typedef struct{
    uint8_t addr;       // I2C address of the LCD
    uint8_t cols;       // Number of columns in the LCD
    uint8_t rows;       // Number of rows in the LCD
    uint8_t backlight;  // Backlight state
    i2c_inst_t *i2c;    // I2C HW block
    uint16_t baudrate;  // Baudrate in kHz for I2C communication
    uint8_t sda;        // SDA pin
    uint8_t scl;        // SCL pin
    uint8_t display;    // Display state
    uint8_t cursor;     // Cursor state
    char *temp_message; // Temporary message buffer
}lcd_t;

/**
 * @brief Function to initialize the LCD display I2C
 * 
 * @param lcd Pointer to the LCD structure
 * @param addr Address of the I2C device 
 * @param cols Number of columns
 * @param rows Number of rows
 * @param baudrate Frequency of the I2C communication in kHz
 * @param sda GPIO pin for SDA
 * @param scl GPIO pin for SCL
 */
void lcd_init(lcd_t *lcd, uint8_t addr, i2c_inst_t *i2c, uint8_t cols, uint8_t rows, uint16_t baudrate, uint8_t sda, uint8_t scl);

/**
 * @brief Quick helper function for single byte transfers
 * 
 * @param lcd Pointer to the LCD structure
 * @param val Byte to be sent
 */
void lcd_write(lcd_t *lcd, uint8_t val);

/**
 * @brief Clear the display
 * 
 * @param lcd  Pointer to the LCD structure
 */
void lcd_clear_display(lcd_t *lcd);

/**
 * @brief Return the cursor to the home position
 * 
 * @param lcd Pointer to the LCD structure
 */
void lcd_return_home(lcd_t *lcd);

/**
 * @brief Move the cursor to a specific position in the LCD
 * 
 * @param lcd Pointer to the LCD structure
 * @param row Number of the row position for the cursor
 * @param col Number of the column position for the cursor
 */
void lcd_move_cursor(lcd_t *lcd, uint8_t row, uint8_t col);

/**
 * @brief Send a byte to the LCD 
 * 
 * @param lcd Pointer to the LCD structure
 * @param val Value to be sent
 * @param mode Operation mode (LCD_COMMAND or LCD_CHARACTER)
 */
void lcd_send_byte(lcd_t *lcd, uint8_t val, uint8_t mode);

/**
 * @brief Send a character to the LCD
 * 
 * @param lcd Pointer to the LCD structure
 * @param character Character to be sent
 */
void lcd_send_char(lcd_t *lcd, uint8_t character);

/**
 * @brief Send a string to the LCD
 * 
 * @param lcd Pointer to the LCD structure
 * @param str String to be sent
 */
void lcd_send_str(lcd_t *lcd, uint8_t *str);



#endif // __LIQUID_CRYSTAL_I2C_H__