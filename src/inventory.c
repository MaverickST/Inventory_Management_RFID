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
#include "hardware/dma.h"

#include "inventory.h"

void inventory_init(inventory_t *inv, bool access)
{
    inv->access = access;
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
    uint32_t *ptr = (uint32_t *)addr; // Place an int pointer at our memory-mapped address

    // // Copy the data from the flash memory to the inventory_t structure
    // for (int i = 0; i < 5; i++){
    //     for (int j = 0; j < 3; j++){
    //         inv->database[i][j] = ptr[i*3 + j];
    //     }
    // }

    // Get a free channel, panic() if there are none
    int chan = dma_claim_unused_channel(true);

    // 8 bit transfers. Both read and write address increment after each
    // transfer (each pointing to a location in src or dst respectively).
    // No DREQ is selected, so the DMA transfers as fast as it can.

    dma_channel_config c = dma_channel_get_default_config(chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, true);

    dma_channel_configure(
        chan,          // Channel to be configured
        &c,            // The configuration we just created
        &inv->database[0],           // The initial write address
        ptr,           // The initial read address
        count_of(inv->database), // Number of transfers; in this case each is 1 byte.
        true           // Start immediately.
    );
}
