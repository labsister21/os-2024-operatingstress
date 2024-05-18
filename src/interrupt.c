#include "header/interrupt/interrupt.h"
#include "header/cpu/portio.h"
#include "header/cpu/gdt.h"
#include "header/text/framebuffer.h"
#include "header/driver/keyboard.h"

uint8_t curr_row = 0;
uint8_t curr_col = 0;
uint8_t length_of_terminal = 0;

void syscall(struct InterruptFrame frame);

void io_wait(void)
{
    out(0x80, 0);
}

void pic_ack(uint8_t irq)
{
    if (irq >= 8)
        out(PIC2_COMMAND, PIC_ACK);
    out(PIC1_COMMAND, PIC_ACK);
}

void pic_remap(void)
{
    // Starts the initialization sequence in cascade mode
    out(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    out(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    out(PIC1_DATA, PIC1_OFFSET); // ICW2: Master PIC vector offset
    io_wait();
    out(PIC2_DATA, PIC2_OFFSET); // ICW2: Slave PIC vector offset
    io_wait();
    out(PIC1_DATA, 0b0100); // ICW3: tell Master PIC, slave PIC at IRQ2 (0000 0100)
    io_wait();
    out(PIC2_DATA, 0b0010); // ICW3: tell Slave PIC its cascade identity (0000 0010)
    io_wait();

    out(PIC1_DATA, ICW4_8086);
    io_wait();
    out(PIC2_DATA, ICW4_8086);
    io_wait();

    // Disable all interrupts
    out(PIC1_DATA, PIC_DISABLE_ALL_MASK);
    out(PIC2_DATA, PIC_DISABLE_ALL_MASK);
}

void main_interrupt_handler(struct InterruptFrame frame)
{
    if (frame.int_number < 32)
    {
        __asm__("hlt");
    }

    switch (frame.int_number) // kalo di bawah 32, berarti ada exception (error)
    {
    case PIC1_OFFSET + IRQ_KEYBOARD:
        keyboard_isr();
        break;

    case 0x30:
        syscall(frame);
        break;

    default:
        break;
    }
}

void activate_keyboard_interrupt(void)
{
    out(PIC1_DATA, in(PIC1_DATA) & ~(1 << IRQ_KEYBOARD));
}

// void activate_keyboard_interrupt(void)
// {
//     out(PIC1_DATA, PIC_DISABLE_ALL_MASK ^ (1 << IRQ_KEYBOARD));
//     out(PIC2_DATA, PIC_DISABLE_ALL_MASK);
// }

struct TSSEntry _interrupt_tss_entry = {
    .ss0 = GDT_KERNEL_DATA_SEGMENT_SELECTOR,
};

void set_tss_kernel_current_stack(void)
{
    uint32_t stack_ptr;
    // Reading base stack frame instead esp
    __asm__ volatile("mov %%ebp, %0" : "=r"(stack_ptr) : /* <Empty> */);
    // Add 8 because 4 for ret address and other 4 is for stack_ptr variable
    _interrupt_tss_entry.esp0 = stack_ptr + 8;
}

// void puts(char *str, uint32_t len, uint32_t color)
// {
//     if (memcmp(str, "cls", 3) == 0)
//     {
//         curr_row = 0;
//         for (uint32_t i = 0; i < 25; i++)
//         {
//             for (uint32_t j = 0; j < 80; j++)
//             {
//                 framebuffer_write(i, j, ' ', color, 0);
//             }
//         }
//     }
//     else
//     {
//         curr_row++;
//         uint32_t col = 0;
//         for (uint32_t i = 0; i < len; i++)
//         {
//             if (str[i] == '\n')
//             {
//                 curr_row++;
//                 col = 0;
//             }
//             else
//             {
//                 framebuffer_write(curr_row, col, str[i], color, 0);
//                 col++;
//             }
//         }
//         curr_row++;
//     }
// }

void putchar(char str, uint32_t color)
{
    int size = sizeof(str);
    if (!memcmp(&str, "\0", 1))
    {
        puts(&str, size, color);
    }
}

// void puts(char *str, uint32_t len, uint32_t color)
// {
//     // int size = sizeof(str);
//     // if (!memcmp(&str, "\0", 1)) {
//     //     puts(&str, size, color);
//     // }
//     for (uint8_t i = 0; i < len; i++) {
//         framebuffer_set_cursor(curr_row, curr_col + i);
//         if (str[i] == '\n') {
//             curr_row++;
//             curr_col = 0;
//             framebuffer_set_cursor(curr_row, curr_col);
//         } else {
//             framebuffer_write(curr_row, curr_col + i, str[i], color, 0);
//             if(i == len - 1) {
//                 curr_col = curr_col + len;
//             }
//         }
//         // curr_col_threshold = curr_col;
//         // curr_col = col;
//     }
// }

void syscall(struct InterruptFrame frame)
{
    switch (frame.cpu.general.eax)
    {
    case 0:
        *((int8_t *)frame.cpu.general.ecx) = read(
            *(struct FAT32DriverRequest *)frame.cpu.general.ebx);
        break;
    case 1:
        *((int8_t *)frame.cpu.general.ecx) = read_directory(
            *(struct FAT32DriverRequest *)frame.cpu.general.ebx);
        break;
    case 2:
        *((int8_t *)frame.cpu.general.ecx) = write(
            *(struct FAT32DriverRequest *)frame.cpu.general.ebx);
        break;
    case 3:
        *((int8_t *)frame.cpu.general.ecx) = delete (
            *(struct FAT32DriverRequest *)frame.cpu.general.ebx);
        break;
    case 4:
        keyboard_state_activate();
        __asm__("sti");
        while (is_keyboard_blocking())
            ;
        char buffer[KEYBOARD_BUFFER_SIZE];
        get_keyboard_buffer(buffer);
        memcpy((char *)frame.cpu.general.ebx, buffer, KEYBOARD_BUFFER_SIZE);
        break;
    case 5:
        putchar(frame.cpu.general.ebx, frame.cpu.general.edx);
        break;
    case 6:
        puts(
            (char *)frame.cpu.general.ebx,
            frame.cpu.general.ecx,
            frame.cpu.general.edx); // Assuming puts() exist in kernel
        break;
    case 7:
        keyboard_state_activate();
        break;
    }
}
