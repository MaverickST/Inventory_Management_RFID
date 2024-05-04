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
        uint8_t key     :1; //keypad interruption pending
        uint8_t tag     :1; //tag interruption pending
        uint8_t admin   :1; //admin interruption pending
        uint8_t inv     :1; //inventory interruption pending
        uint8_t user    :1; //user interruption pending
        uint8_t         :3;
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
// ---------------------- Callback functions -------------------
// -------------------------------------------------------------

/**
 * @brief This function is the main callback function for the GPIO interruptions.
 * 
 * @param num 
 * @param mask 
 */
void gpioCallback(uint num, uint32_t mask);

/**
 * @brief Definition of the keypad callback function, which will be called by the handler of the GPIO interruptions.
 * 
 * @param num 
 * @param mask 
 */
void keypadCallback(uint num, uint32_t mask);


/**
 * @brief Handler for the debouncer timer interruptions.
 * 
 */
void dbnc_timer_handler(void);

// -------------------------------------------------------------
// ---------------------- Check functions ----------------------
// -------------------------------------------------------------

static inline bool checkNumber(uint8_t number){
    return (number >= 0 && number <= 9);
}

static inline bool checkLetter(uint8_t letter){
    return (letter >= 0x0A && letter <= 0x0C);
}

static inline bool checkFreq(uint32_t freq){
    return (freq >= 1 && freq <= 12000000);
}

static inline bool checkAmp(uint32_t amp){
    return (amp >= 100 && amp <= 2500);
}

static inline bool checkOffset(uint32_t offset){
    return (offset >= 50 && offset <= 1250);
}

#endif // FUNTCS

