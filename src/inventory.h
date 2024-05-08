/**
 * \file        keypad_irq.h
 * \brief
 * \details
 * \author      MST_CDA
 * \version     0.0.2
 * \date        03/05/2024
 * \copyright   Unlicensed
 */

#ifndef __INVENTORY_
#define __INVENTORY_

#include <stdint.h>

#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE) ///< Flash-based address of the last sector

/**
 * @brief Definition of the inventory structure
 * 
 */
typedef struct
{
    struct
    {
        uint32_t amount;
        uint32_t sale_v;
        uint32_t purchase_v;
    }id1;

    uint32_t database[5][3]; //[id][amount, purchase_v, sale_v]
    
}inventory_t;

/**
 * @brief This function initializes the inventory_t structure
 * 
 * @param inv 
 */
void inventory_init(inventory_t *inv);

/**
 * @brief This function stores the inventory_t structure in the flash memory
 * 
 * @param inv 
 */
void inventory_store(inventory_t *inv);

/**
 * @brief This function loads the inventory_t structure from the flash memory
 * 
 * @param inv 
 */
void inventory_load(inventory_t *inv);

#endif // __INVENTORY_