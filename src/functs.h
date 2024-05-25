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
        uint8_t nfc_tag     :1; //tag interruption pending
        uint8_t kpad_switch :1; //keypad switch interruption pending
        uint8_t inv_show    :1; //inventory show interruption pending
        uint8_t             :4;
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

/**
 * @brief This function allows to show the inventory of the system on the LCD based on flags and states.
 * 
 */
void show_inventory(void);

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
 * @brief This function initializes a PWM signal as a periodic interrupt timer (PIT).
 * Each slice will generate interruptions at a period of milis miliseconds.
 * Due to each slice share clock counter (period), events with diferents periods 
 * must not be generated in the same slice, i.e, they must not share channel.
 * 
 * @param slice 
 * @param milis Period of the PWM interruption in miliseconds
 * @param enable 
 */
void initPWMasPIT(uint8_t slice, uint16_t milis, bool enable);

/**
 * @brief This function is the handler for the PWM interruptions.
 * 
 */
void pwm_handler(void);

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

