#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/text/framebuffer.h"
#include "header/stdlib/string.h"
#include "header/cpu/portio.h"

void framebuffer_set_cursor(uint8_t r, uint8_t c)
{
    out(CURSOR_PORT_CMD, 0x0A);
    out(CURSOR_PORT_DATA, (in(CURSOR_PORT_DATA) & 0xC0) | r);

    out(CURSOR_PORT_CMD, 0x0B);
    out(CURSOR_PORT_DATA, (in(CURSOR_PORT_DATA) & 0xE0) | c);
}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg)
{
    uint16_t attrib = (bg << 4) | (fg & 0x0F);
    volatile uint16_t *where;
    where = (volatile uint16_t *)0xB8000 + (row * 80 + col);
    *where = c | (attrib << 8);
}

void framebuffer_clear(void)
{
    uint16_t attrib = 0x0F00;
    volatile uint16_t *where = (volatile uint16_t *)0xB8000;
    memset((void *)where, ' ' | (attrib << 8), 80 * 25 * 2);

    for (int i = 1; i < 80 * 25 * 2; i += 2)
    {
        FRAMEBUFFER_MEMORY_OFFSET[i] = 0x00;
    }
}