#include "liquid_crystal_i2c.h"


void lcd_init(lcd_t *lcd, uint8_t addr, i2c_inst_t *i2c, uint8_t cols, uint8_t rows, uint16_t baudrate, uint8_t sda, uint8_t scl)
{
    // Initialize the LCD structure
    lcd->addr = addr;
    lcd->cols = cols;
    lcd->rows = rows;
    lcd->backlight = 0;
    lcd->i2c = i2c;
    lcd->baudrate = baudrate;
    lcd->sda = sda;
    lcd->scl = scl;
    lcd->display = 0;
    lcd->cursor = 0;
    lcd->temp_message = NULL;
    lcd->num_alarm = 0;
    lcd->pos_secuence = 0;


    // Initialize the I2C communication
    i2c_init(lcd->i2c, baudrate*1000);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_pull_up(sda);
    gpio_pull_up(scl);

    lcd_init_sequence_callback();

    // Make the I2C pins available to picotool
    //bi_decl(bi_2pins_with_func(sda, scl, GPIO_FUNC_I2C));

    // Initialize the LCD display
    // uint8_t lcd_function = (LCD_FUNCTION_SET | LCD_2LINE);
    // uint8_t lcd_entry_mode = (LCD_ENTRYMODESET | LCD_ENTRYLEFT);
    // uint8_t lcd_display_ctrl = (LCD_DISPLAY_CONTROL | LCD_DISPLAY_ON);
    
    // sleep_ms(15);
    // lcd_send_byte(lcd, 0x03, LCD_COMMAND);
    // sleep_ms(5);
    // lcd_send_byte(lcd, 0x03, LCD_COMMAND);
    // sleep_us(100);
    // lcd_send_byte(lcd, 0x03, LCD_COMMAND);
    // sleep_us(100);
    // lcd_send_byte(lcd, 0x02, LCD_COMMAND);
    
    // // Initialisation sequence as per the Hitachi manual (Figure 24, p.46).
    // sleep_ms(15);
    // // Function set
    // lcd_send_byte(lcd, lcd_entry_mode, LCD_COMMAND);
    // sleep_us(40);
    // // Display control
    // lcd_send_byte(lcd, lcd_function, LCD_COMMAND);
    // sleep_us(40);
    // // Entry mode
    // lcd_send_byte(lcd, lcd_display_ctrl, LCD_COMMAND);
    // sleep_us(40);
    // // Display clear
    // lcd_clear_display(lcd);
    // sleep_ms(2);
    

}

void lcd_write(lcd_t *lcd, uint8_t val)
{
    //uint8_t buf[] = {val0, val1};
    i2c_write_blocking(lcd->i2c, lcd->addr, &val, 1, false);
}

void lcd_clear_display(lcd_t *lcd)
{
    lcd_send_byte(lcd, LCD_CLEAR_DISPLAY, LCD_COMMAND);
}

void lcd_return_home(lcd_t *lcd)
{
    lcd_send_byte(lcd, LCD_RETURN_HOME, LCD_COMMAND);
}

void lcd_move_cursor(lcd_t *lcd, uint8_t row, uint8_t col)
{
    int val = (row == 0) ? 0x80 + col : 0xC0 + col;
    lcd_send_byte(lcd, val, LCD_COMMAND);
}

void lcd_send_byte(lcd_t *lcd, uint8_t val, uint8_t mode)
{
    // The display is sent a byte as two separate nibble transfers. The
    // high nibble is sent first, followed by the low nibble. The
    // transfer is done in 4-bit mode, so the high nibble is sent first
    // by shifting the byte 4 places to the right. The high nibble is
    // then sent by masking the byte with 0x0F.
    uint8_t high_nibble = mode | (val & 0xF0) | LCD_BACKLIGHT;
    uint8_t low_nibble =  mode | ((val << 4) & 0xF0) | LCD_BACKLIGHT;
    //lcd_write(lcd, high_nibble, low_nibble);

    // Send high nibble
    lcd_write(lcd, high_nibble | LCD_ENABLE_BIT);
    lcd_write(lcd, high_nibble & ~LCD_ENABLE_BIT);

    // Send low nibble
    lcd_write(lcd, low_nibble | LCD_ENABLE_BIT);
    lcd_write(lcd, low_nibble & ~LCD_ENABLE_BIT);

}

void lcd_send_char(lcd_t *lcd, uint8_t character)
{
    // To display characters, the action is "write data to DDRAM". For
    // that, (i) set RS to 1 (i.e. select Data Register), (ii) set R/~W
    // to 0 (write), then (iii) write the 8 bits that code the (ascii)
    // character.
    lcd_send_byte(lcd, character, LCD_CHARACTER);
}

void lcd_send_str(lcd_t *lcd, uint8_t *str)
{

    // Send a string of characters to the LCD
    while (*str) {
        lcd_send_char(lcd, *str++);
    }
}

void lcd_send_str_cursor(lcd_t *lcd, uint8_t *str, uint8_t row, uint8_t col)
{
    lcd_move_cursor(lcd, row, col);

    lcd->temp_message = str;

    // Setting the IRQ handler
    irq_set_exclusive_handler(TIMER_IRQ_0, lcd_send_str_callback);
    irq_set_enabled(TIMER_IRQ_0, true);
    hw_set_bits(&timer_hw->inte, 1u << lcd->alarm); // Enable alarm0 for signal value calculation
    timer_hw->alarm[lcd->alarm] = (uint32_t)(time_us_64() + 150); // Set alarm0 to trigger in t_sample
}

void lcd_send_str_callback(void)
{
    // Interrupt acknowledge
    hw_clear_bits(&timer_hw->intr, 1u << gLcd.alarm);

    // Send the string to the LCD
    lcd_send_char(&glcd, glcd->temp_message++);
    gLcd.temp_message = NULL;

}

void lcd_init_sequence_callback(void)
{
    // Interrupt acknowledge
    hw_clear_bits(&timer_hw->intr, 1u << gLcd.alarm);

    // position of the sequence
    uint16_t time_next_secuence_us = 0;

    // Initialisation sequence as per the Hitachi manual (Figure 24, p.46).
    uint8_t lcd_function = (LCD_FUNCTION_SET | LCD_2LINE);
    uint8_t lcd_entry_mode = (LCD_ENTRYMODESET | LCD_ENTRYLEFT);
    uint8_t lcd_display_ctrl = (LCD_DISPLAY_CONTROL | LCD_DISPLAY_ON);
    
    
    switch (gLcd.pos_secuence)
    {
    case 0:
        lcd_send_byte(&gLcd, 0x03, LCD_COMMAND);
        time_next_secuence_us = 5000;
        break;
    case 1:
        lcd_send_byte(&gLcd, 0x03, LCD_COMMAND);
        time_next_secuence_us = 100;
        break;
    case 2:
        lcd_send_byte(&gLcd, 0x03, LCD_COMMAND);
        time_next_secuence_us = 100;
        break;
    case 3:
        lcd_send_byte(&glcd, 0x02, LCD_COMMAND);
        time_next_secuence_us = 150000;
        break;
    case 4:
        // Function set
        lcd_send_byte(&gLcd, lcd_entry_mode, LCD_COMMAND);
        time_next_secuence_us = 40;
        break;
    case 5:
        // Display control
        lcd_send_byte(&gLcd, lcd_function, LCD_COMMAND);
        time_next_secuence_us = 40;
        break;
    case 6:
        // Entry mode
        lcd_send_byte(&gLcd, lcd_display_ctrl, LCD_COMMAND);
        time_next_secuence_us = 40;
        break;
    case 7:
        // Display clear
        lcd_clear_display(&gLcd);
        time_next_secuence_us = 2000;
        break;
    case default:
        break;
    }

    gLcd.pos_secuence++;

    if (gLcd.pos_secuence <= 7)
    {
        // Setting the IRQ handler
        irq_set_exclusive_handler(TIMER_IRQ_0, lcd_send_str_callback);
        irq_set_enabled(TIMER_IRQ_0, true);
        hw_set_bits(&timer_hw->inte, 1u << gLcd.alarm); // Enable alarm0 for signal value calculation
        timer_hw->alarm[gLcd.alarm] = (uint32_t)(time_us_64() + time_next_secuence_us); // Set alarm0 to trigger in t_sample
    }
}