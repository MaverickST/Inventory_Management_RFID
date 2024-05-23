/**
 * \file        functs.h
 * \brief
 * \details
 * \author      MST_CDA
 * \version     0.0.1
 * \date        10/04/2024
 * \copyright   Unlicensed
 */

#ifndef __FUNTCS_
#define __FUNTCS_

#include <stdint.h>

/**
 * @brief This typedef is for generate a word on we have the flags of interrups pending.
 * @typedef flags_t
 */
typedef union{
    uint8_t W;
    struct{
        uint8_t key         :1; //keypad interruption pending
        uint8_t nfc         :1; //tag interruption pending
        uint8_t kpad_cols   :1; //keypad cols interruption pending
        uint8_t kpad_rows   :1; //keypad rows interruption pending
        uint8_t             :3;
    }B;
}flags_t;

/**
 * @brief This function initializes the global variables of the system: keypad, signal generator, button, and DAC.
 * 
 */
void initGlobalVariables(void);

/**
 * @brief This function is the main, here the program is executed when a flag of interruption is pending.
 * 
 */
void program(void);

/**
 * @brief This function checks if there are a flag of interruption pending for execute the program.
 * 
 * @return true When there are a flag of interruption pending
 * @return false When there are not a flag of interruption pending
 */
bool check();

// -------------------------------------------------------------
// ---------------- Callback and handler functions -------------
// -------------------------------------------------------------

/**
 * @brief This function is the main callback function for the GPIO interruptions.
 * 
 * @param num 
 * @param mask 
 */
void gpioCallback(uint num, uint32_t mask);

/**
 * @brief Handler for the debouncer timer interruptions.
 * 
 */
void dbnc_timer_handler(void);

/**
 * @brief Handler for the led timer interruptions.
 * 
 */
void led_timer_handler(void);

/**
 * @brief Handler for the check tag timer interruptions.
 * 
 */
void check_tag_timer_handler(void);

/**
 * @brief Handler for the I2C interruptions.
 * 
 */
// void i2c_handler(void);

/**
 * @brief Handler fot the lcd timer interruptions.
 * 
 */
void lcd_send_str_callback(void);

/**
 * @brief Handler for the lcd initialization sequence.
 * 
 */
void lcd_initialization_timer_handler(void);

// -------------------------------------------------------------
// ---------------------- Check functions ----------------------
// -------------------------------------------------------------

static inline bool checkNumber(uint8_t number){
    return (number >= 0 && number <= 9);
}

static inline bool checkLetter(uint8_t letter){
    return (letter >= 0x0A && letter <= 0x0D);
}


#endif // FUNTCS

