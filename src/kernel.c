#include <stdint.h>
#include <stdbool.h>
#include "header/cpu/gdt.h"
#include "header/kernel-entrypoint.h"
#include "header/text/framebuffer.h"
#include "header/interrupt/interrupt.h"
#include "header/interrupt/idt.h"
#include "header/driver/keyboard.h"

void kernel_setup(void) {
    load_gdt(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
        
    int col = 0;
    int row = 0;
    int col_recent = 0;
    keyboard_state_activate();
    while (true) {
        char c;
        get_keyboard_buffer(&c);
        if (c) {
            if (c == '\b') { // Backspace
                if (col > 0) {
                    framebuffer_write(row, --col, ' ', 0xF, 0); // Remove character
                    framebuffer_set_cursor(row, col);
                }
                else if (col == 0 && row > 0) {
                    row--; // Move to the previous row
                    col = col_recent; // Move to the last column of the previous row
                    framebuffer_write(row, col, ' ', 0xF, 0); // Remove character
                    framebuffer_set_cursor(row, col);
                }
            } else if (c == '\n') { // Enter
                row++; 
                col_recent = col;
                col = 0; // Move to the next line
                framebuffer_set_cursor(row, col);
            } 
            else if (c == 0) { // Arrow keys
                // Handle arrow key movements
                get_keyboard_buffer(&c); // Read the extended scancode byte
                if (c == EXT_SCANCODE_UP && row > 0) {
                    row--;
                    framebuffer_set_cursor(row, col);
                } else if (c == EXT_SCANCODE_DOWN && row < 25 - 1) {
                    row++;
                    framebuffer_set_cursor(row, col);
                } else if (c == EXT_SCANCODE_LEFT && col > 0) {
                    col--;
                    framebuffer_set_cursor(row, col);
                } else if (c == EXT_SCANCODE_RIGHT && col < 80 - 1) {
                    col++;
                    framebuffer_set_cursor(row, col);
                }
            }
            else { // Regular character
                framebuffer_write(row, col++, c, 0xF, 0);
                framebuffer_write(row, col, ' ', 0xf, 0);
                framebuffer_set_cursor(row, col);
            }
        }
    }
}


