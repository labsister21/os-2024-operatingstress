#include <stdint.h>
#include <stdbool.h>
#include "header/cpu/gdt.h"
#include "header/kernel-entrypoint.h"
#include "header/text/framebuffer.h"
#include "header/interrupt/interrupt.h"
#include "header/interrupt/idt.h"
#include "header/driver/keyboard.h"
#include "header/filesystem/fat32.h"
#include "header/memory/paging.h"

void kernel_setup(void)
{
    load_gdt(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    initialize_filesystem_fat32();
    gdt_install_tss();
    set_tss_register();

    // struct PageDirectory page_directory;
    // void *virtual_address = (void *)0x12345678;
    // paging_allocate_user_page_frame(&page_directory, virtual_address);

    // Allocate first 4 MiB virtual memory
    paging_allocate_user_page_frame(&_paging_kernel_page_directory, (uint8_t *)0);

    // Write shell into memory
    struct FAT32DriverRequest request = {
        .buf = (uint8_t *)0,
        .name = "shell",
        .ext = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size = 0x100000,
    };
    read(request);

    int n = 30;
    char input[n];
    for (int i = 0; i < n; i++)
    {
        if (i < 10)
            input[i] = 'a';
        else if (i < 4096)
            input[i] = 'c';
        else
            input[i] = 'b';
    }
    input[n - 1] = '\0';

    struct FAT32DriverRequest request3 = {
        .buf = input,
        .buffer_size = n,
        .name = {'e', 'd', '3', '\0', '\0', '\0'},
        .ext = {'t', 'x', 't'},
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
    };

    write(request3);

    // Set TSS $esp pointer and jump into shell
    set_tss_kernel_current_stack();
    kernel_execute_user_program((uint8_t *)0);

    while (true)
        ;

    // int col = 0;
    // int row = 0;
    // int col_recent = 0;
    // keyboard_state_activate();
    // while (true) {
    //     char c;
    //     get_keyboard_buffer(&c);
    //     if (c) {
    //         if (c == '\b') { // Backspace
    //             if (col > 0) {
    //                 framebuffer_write(row, --col, ' ', 0xF, 0); // Remove character
    //                 framebuffer_set_cursor(row, col);
    //             }
    //             else if (col == 0 && row > 0) {
    //                 row--; // Move to the previous row
    //                 col = col_recent; // Move to the last column of the previous row
    //                 framebuffer_write(row, col, ' ', 0xF, 0); // Remove character
    //                 framebuffer_set_cursor(row, col);
    //             }
    //         } else if (c == '\n') { // Enter
    //             row++;
    //             col_recent = col;
    //             col = 0; // Move to the next line
    //             framebuffer_set_cursor(row, col);
    //         } else { // Regular character
    //             framebuffer_write(row, col++, c, 0xF, 0);
    //             framebuffer_write(row, col, ' ', 0xf, 0);
    //             framebuffer_set_cursor(row, col);
    //         }
    //     }
    // }
}