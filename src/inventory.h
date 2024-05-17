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
#include <stdbool.h>

#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE) ///< Flash-based address of the last sector

/**
 * @brief Definition of the inventory structure
 * 
 */
typedef struct
{

    uint32_t database[5][3]; //[id][amount, purchase_v, sale_v]
    bool access; // Flag that indicates that the inventory is able to be accessed
    
}inventory_t;

/**
 * @brief This function initializes the inventory_t structure
 * 
 * @param inv 
 * @param access
 */
void inventory_init(inventory_t *inv, bool access);

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

/**
 * @brief Definition of the wrapper function for the flash_safe_execute function,
 * which is used to erase the last sector of the flash memory.
 * 
 */
void invetory_wrapper();

/**
 * @brief Auxiliary function to print the data of the inventory
 * 
 * @param data 
 */
void inventory_print_data(uint32_t *data);

/**
 * @brief Perform a inbound transaction.
 * The parameters are each of the values stored in the tag.
 * 
 * @param inv 
 * @param id 
 * @param amount 
 * @param purchase_v 
 * @param sale_v 
 */
void inventory_in_transaction(inventory_t *inv, uint8_t id, uint32_t amount, uint32_t purchase_v, uint32_t sale_v);

/**
 * @brief Perform a outbound transaction.
 * The parameters are each of the values stored in the tag.
 * 
 * @param inv 
 * @param id 
 * @param amount 
 * @param purchase_v 
 * @param sale_v 
 */
void inventory_out_transaction(inventory_t *inv, uint8_t id, uint32_t amount, uint32_t purchase_v, uint32_t sale_v);

static inline void inventory_reset(inventory_t *inv)
{
    for (int i = 0; i < 5; i++){
        for (int j = 0; j < 3; j++){
            inv->database[i][j] = 0;
        }
    }
    inventory_store(inv);
}

#endif // __INVENTORY_