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
#include "pico/flash.h"
#include "pico/time.h"
#include "hardware/irq.h"

#include "inventory.h"

void inventory_init(inventory_t *inv, bool access)
{
    inv->access = access;
    inv->timer_irq = TIMER_IRQ_3;
    inv->time = 3000000; // 3 seconds
    inv->state = DATA_BASE;
    inv->count.id = 0;
    inv->count.frame = 0;
    inv->today.amount = 0;
    inv->today.purchases = 0;
    inv->today.sales = 0;

    // Initialize the database
    inventory_load(inv);
    printf("Inventory initialized\n");
    inventory_print_data(inv->database[0]);
    
}

void inventory_store(inventory_t *inv)
{
    // An array of 256 bytes, multiple of FLASH_PAGE_SIZE. Database is 60 bytes.
    uint32_t buf[FLASH_PAGE_SIZE/sizeof(uint32_t)];

    // Copy the database into the buffer
    for (int i = 0; i < 5; i++){
        for (int j = 0; j < 3; j++){
            buf[i*3 + j] = inv->database[i][j];
        }
    }
    // Program buf[] into the first page of this sector
    // Each page is 256 bytes, and each sector is 4K bytes
    // Erase the last sector of the flash
    flash_safe_execute(invetory_wrapper, NULL, 500);

    uint32_t ints = save_and_disable_interrupts();
    flash_range_program(FLASH_TARGET_OFFSET, (uint8_t *)buf, FLASH_PAGE_SIZE);
    restore_interrupts (ints);

    // Prints
    printf("\nStored data in flash\n");
    inventory_print_data(buf);
}

void inventory_load(inventory_t *inv)
{
    // Compute the memory-mapped address, remembering to include the offset for RAM
    uint32_t addr = XIP_BASE +  FLASH_TARGET_OFFSET;
    uint32_t *ptr = (uint32_t *)addr; ///< Place an int pointer at our memory-mapped address

    // Load the inventory from the flash memory
    for (int i = 0; i < 5; i++){
        for (int j = 0; j < 3; j++){
            inv->database[i][j] = ptr[i*3 + j];
        }
    }
}

void invetory_wrapper()
{
    flash_range_erase((PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE), FLASH_SECTOR_SIZE);
}

void inventory_print_data(uint32_t *data)
{
    printf("ID\t \t Amount\t Purchase\t Sale \n");
    for (int i = 0; i < 5; i++){
        printf("%d\t \t", i + 1);
        for (int j = 0; j < 3; j++){
            if (j == 2) {
                printf("           %u\t", data[i*3 + j]);
            }else {
                printf("  %u\t", data[i*3 + j]);
            }
        }
        printf("\n");
    }
    printf("\n");

}

void inventory_in_transaction(inventory_t *inv)
{
    inv->database[inv->tag.id - 1][0] += inv->tag.amount;
    inventory_store(inv);

    inv->today.amount += inv->tag.amount;
    inv->today.purchases += (inv->tag.purchase_v * inv->tag.amount);
}

bool inventory_out_transaction(inventory_t *inv)
{
    if (inv->database[inv->tag.id][0] < inv->tag.amount) {
        return false;
    }
    else {
        inv->database[inv->tag.id - 1][0] -= inv->tag.amount;
        inventory_store(inv);

        inv->today.amount -= inv->tag.amount;
        inv->today.sales += (inv->tag.sale_v * inv->tag.amount);
        return true;
    }
}

