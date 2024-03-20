#include <stdint.h>
#include <stdbool.h>
#include "header/cpu/gdt.h"
#include "header/kernel-entrypoint.h"
#include "header/text/framebuffer.h"
#include "header/interrupt/interrupt.h"
#include "header/interrupt/idt.h"
#include "header/driver/keyboard.h"

void kernel_setup(void)
{
    load_gdt(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    // framebuffer_clear();
    framebuffer_write(3, 10, 'h', 0xF, 0x0);
    framebuffer_set_cursor(0, 0);
        
    int col = 0;
    keyboard_state_activate();
    while (true) {
         char c;
         get_keyboard_buffer(&c);
         if (c) framebuffer_write(0, col++, c, 0xF, 0);
    }
}
