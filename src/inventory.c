#include "inventory.h"
/**
 * \file        inventory.c
 * \brief
 * \details
 * \author      MST_CDA
 * \version     0.0.1
 * \date        05/10/2023
 * \copyright   Unlicensed
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include "hardware/flash.h"

#include "inventory.h"

void inventory_init(inventory_t *inv)
{
    inventory_load(inv);
}

void inventory_store(inventory_t *inv)
{
    // An array of 256 bytes, multiple of FLASH_PAGE_SIZE. 
    // We need to store the inventory_t structure in the flash memory, which sizes 60 bytes (database)
    uint32_t buf[FLASH_PAGE_SIZE/sizeof(uint32_t)];

    for (int i = 0; i < 5; i++){
        for (int j = 0; j < 3; j++){
            buf[i*3 + j] = inv->database[i][j];
        }
    }

    // Erase the last sector of the flash
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);

    // Program buf[] into the first page of this sector
    // Each page is 256 bytes, and each sector is 4K bytes
    uint32_t ints = save_and_disable_interrupts();
    flash_range_program(FLASH_TARGET_OFFSET, (uint8_t *)buf, FLASH_PAGE_SIZE);
    restore_interrupts (ints);
}

void inventory_load(inventory_t *inv)
{
    // Compute the memory-mapped address, remembering to include the offset for RAM
    uint32_t addr = XIP_BASE +  FLASH_TARGET_OFFSET;
    uint32_t ptr = (uint32_t *)addr; // Place an int pointer at our memory-mapped address

    // Copy the data from the flash memory to the inventory_t structure
    for (int i = 0; i < 5; i++){
        for (int j = 0; j < 3; j++){
            inv->database[i][j] = ptr[i*3 + j];
        }
    }
}
