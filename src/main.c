/**
 * \file        main.c
 * \brief       This proyect implements an digital signal generator. 
 * \details     
 * An inventory management system is a tool or set of tools designed to monitor, control, 
 * and administer the stock of products or materials of a company or entity. It allows tracking 
 * the quantity of available items, recording inbound and outbound transactions, and providing 
 * updated information on inventory, helping to avoid shortages or excess stock and facilitating 
 * decision-making in resource management. In this practice, we will develop an inventory control 
 * system for the warehouse of an agricultural supplies company that handles 5 different products. 
 * The entry and exit of products from the warehouse are handled at the box level. Each box is labeled 
 * with an RFID TAG containing the type of product, the quantity of items in the product, the unit purchase 
 * price of each item, and the unit selling price of each item. The detailed characteristics of the system 
 * are as follows:
 * 
 *        • At the start of the day when the warehouse begins operations, the inventory management 
 *          system is activated, and the main user must authenticate with a special RFID card and a 
 *          4-digit password via a 4x4 matrix keypad. This ensures that only authorized personnel 
 *          can access inventory management.
 *        • During warehouse operation, the user scans blue RFID tags representing product boxes 
 *          by bringing the tags close to the RFID reader integrated into the system. The system 
 *          extracts information stored on the RFID tag: product type, number of items in the box, 
 *          and the unit price of each item. The system supports two types of transactions: inbound 
 *          and outbound. The transaction type is indicated via the matrix keypad after reading the tag: 
 *          Letter A for inbound transactions and Letter B for outbound transactions. If the transaction 
 *          is inbound, the number of items read from the tag must be added to the inventory of the respective 
 *          product type, and the total value of the goods in the warehouse must be updated. If the transaction 
 *          is outbound, the number of items is subtracted from the product inventory, the total value of the 
 *          goods in the warehouse is updated, and the total sales value since the system was opened is updated.
 *        • The warehouse has a maximum storage capacity for each product type. Only boxes that do not 
 *          exceed the maximum storage limit are accepted. In the case of an inbound transaction where 
 *          this occurs, the box will not be admitted, and the inventory status should not be updated. 
 *          The warehouse's storage capacity can be read from an RFID tag. The configuration of the 
 *          warehouse capacity must be done at least once before any transaction of box entry or exit. 
 *          This procedure for registering the warehouse capacity is left to be defined by the designer.
 *        • The inventory status must be maintained even if the system loses power or is disconnected. 
 *          However, the main user of the system must have the ability to reset the inventory to zero for 
 *          all products. The system designer will determine the procedure for this process.
 *        • At all times, a display shows the current inventory of the warehouse, the value of the goods, 
 *          and the total sales since the warehouse was opened.
 *        • Data extracted from a tag in an inbound or outbound transaction of a box is also displayed 
 *          momentarily on a screen until the user presses the transaction type on the keypad, allowing the 
 *          user to quickly view box details, including product type, number of items in the box, and purchase 
 *          and selling prices.
 *        • The warehouse handles only 5 products, and the system does not allow adding additional products. 
 *          To emulate box tags, use blue tags. Use the EPC field of the tag memory to store the product type.
 *        • There are 2 special tags, the main user tag and the warehouse capacity tag. Use white cards for 
 *          these tags. Use the EPC field of the tag memory to differentiate these two tags from the product box tags. 
 *        • Except for the EPC field, the structure and handling of data within each tag are the responsibility 
 *          of the system designer. The inventory management system only reads the tags and does 
 *          not write them. However, it is clear that the tags must be written with the desired information.
 *        • A parallel embedded application allows writing the required information on each tag from a 
 *          terminal or graphical interface. This application can be developed by you, or it is also possible 
 *          to use an available application that allows configuring the tag. During the practice presentation, 
 *          the teacher will request you to write the tags with certain information, so this procedure must be agile.
 * 
 * Interrupts:  button debouncer, keypad debouncer, secuence -> Slices 0,1,2 of the PWM
 *              value calculation, printing -> Timers 0, 1
 *              gpio columns -> GPIO 
 * 
 * \author      MST_CDA
 * \version     0.0.1
 * \date        05/10/2023
 * \copyright   Unlicensed
 * 
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/i2c.h"

#include "functs.h"


int main() {
    stdio_init_all();
    sleep_ms(5000);
    printf("Run Program\n");

    // Initialize global variables: keypad, signal generator, button, and DAC.
    initGlobalVariables();

    // Setup of the PWM as PIT
    initPWMasPIT(0,100, false); // 100ms for the button debouncer
    irq_set_exclusive_handler(PWM_IRQ_WRAP,pwm_handler);

    // Set check tag alarm
    check_tag_timer_handler();

    // Set inventary show alarm
    // show_inventory_timer_handler();

    while(1){
        while(check()){
            program();
        }
        __wfi(); // Wait for interrupt (Will put the processor into deep sleep until woken by the RTC interrupt)
    }
}

