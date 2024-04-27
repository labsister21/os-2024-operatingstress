#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/text/framebuffer.h"
#include "header/stdlib/string.h"
#include "header/cpu/portio.h"

void framebuffer_set_cursor(uint8_t r, uint8_t c)
{
    uint16_t pos = r * 80 + c; // Menghitung posisi cursor
    // Mengirimkan posisi cursor ke port CURSOR_PORT_CMD dan CURSOR_PORT_DATA
    out(CURSOR_PORT_CMD, 0x0F);
    out(CURSOR_PORT_DATA, (uint8_t)(pos & 0xFF));
    out(CURSOR_PORT_CMD, 0x0E);
    out(CURSOR_PORT_DATA, (uint8_t)((pos >> 8) & 0xFF));
}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg)
{
    uint16_t attrib = (bg << 4) | (fg & 0x0F);
    volatile uint16_t *where;
    where = (volatile uint16_t *)FRAMEBUFFER_MEMORY_OFFSET + (row * 80 + col);
    *where = c | (attrib << 8);
}

void framebuffer_clear(void)
{
    uint16_t attrib = 0x0F00;
    volatile uint16_t *where = (volatile uint16_t *)FRAMEBUFFER_MEMORY_OFFSET;
    memset((void *)where, ' ' | (attrib << 8), 80 * 25 * 2);

    for (int i = 1; i < 80 * 25 * 2; i += 2)
    {
        FRAMEBUFFER_MEMORY_OFFSET[i] = 0x00;
    }
}

// #include <stdint.h>
// #include <stdbool.h>
// #include <stddef.h>
// #include "header/text/framebuffer.h"
// #include "header/stdlib/string.h"
// #include "header/cpu/portio.h"

// void framebuffer_set_cursor(uint8_t r, uint8_t c) {
//     /*
//     I.S. Menerima r sebagai baris cursor dan c sebagai kolom cursor
//     F.S. Mengubah posisi cursor ke baris r dan kolom c
//     */
//     uint16_t pos = r * 80 + c; // Menghitung posisi cursor
//     // Mengirimkan posisi cursor ke port CURSOR_PORT_CMD dan CURSOR_PORT_DATA
//     out(CURSOR_PORT_CMD, 0x0F);
//     out(CURSOR_PORT_DATA, (uint8_t) (pos & 0xFF));
//     out(CURSOR_PORT_CMD, 0x0E);
//     out(CURSOR_PORT_DATA, (uint8_t) ((pos >> 8) & 0xFF));
// }

// void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) {
//     /*
//     I.S. Menerima :
//             row = baris dari karakter c
//             col = kolom dari karakter c
//             c = karakter yang akan ditulis
//             fg = warna karakter
//             bg = warna background
//     F.S. Menulis karakter c ke framebuffer dengan warna fg dan warna background bg
//     */
//     uint16_t pos = row * 80 + col; // Menghitung posisi karakter c
//     uint8_t color = (bg << 4) | fg; // Memberi warna dan warna background pada karakter c
//     uint16_t *fb = (uint16_t *) FRAMEBUFFER_MEMORY_OFFSET; // Mengambil alamat memory framebuffer
//     fb[pos] = (color << 8) | c; // Menulis karakter c ke framebuffer dengan warna color (shift informasi color ke HIGH BYTE)

// }

// void framebuffer_clear(void) {
//     /*
//     I.S. -
//     F.S. Menghapus semua karakter yang ada di framebuffer
//     */
//     uint16_t *fb = (uint16_t *) FRAMEBUFFER_MEMORY_OFFSET; // Mengambil alamat memory framebuffer
//     // Looping untuk menghapus semua karakter yang ada di framebuffer
//     for (uint16_t i = 0; i < 80 * 25; i++) {
//         fb[i] = (0x07 << 8) | 0x00;
//     }
// }