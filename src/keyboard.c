#include "header/driver/keyboard.h"
// #include "header/driver/framebuffer.h"
#include "header/text/framebuffer.h"
#include "header/cpu/portio.h"
#include "header/stdlib/string.h"

static bool is_capslock = false;
static bool key_pressed = false;
static bool backspace_pressed = false;

int col = 0;
int row = 0;
int col_recent = 0;
int col_bound = 0;

const char keyboard_scancode_1_to_ascii_map[256] = {
    0,
    0x1B,
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    '0',
    '-',
    '=',
    '\b',
    '\t',
    'q',
    'w',
    'e',
    'r',
    't',
    'y',
    'u',
    'i',
    'o',
    'p',
    '[',
    ']',
    '\n',
    0,
    'a',
    's',
    'd',
    'f',
    'g',
    'h',
    'j',
    'k',
    'l',
    ';',
    '\'',
    '`',
    0,
    '\\',
    'z',
    'x',
    'c',
    'v',
    'b',
    'n',
    'm',
    ',',
    '.',
    '/',
    0,
    '*',
    0,
    ' ',
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    '-',
    0,
    0,
    0,
    '+',
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,

    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

const char keyboard_scancode_1_to_ascii_capslock[256] = {
    0,
    0x1B,
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    '0',
    '-',
    '=',
    '\b',
    '\t',
    'Q',
    'W',
    'E',
    'R',
    'T',
    'Y',
    'U',
    'I',
    'O',
    'P',
    '[',
    ']',
    '\n',
    0,
    'A',
    'S',
    'D',
    'F',
    'G',
    'H',
    'J',
    'K',
    'L',
    ';',
    '\'',
    '`',
    0,
    '\\',
    'Z',
    'X',
    'C',
    'V',
    'B',
    'N',
    'M',
    ',',
    '.',
    '/',
    0,
    '*',
    0,
    ' ',
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    '-',
    0,
    0,
    0,
    '+',
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,

    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

static struct KeyboardDriverState keyboard_state = {
    .read_extended_mode = false,
    .keyboard_input_on = false,
    .keyboard_buffer = {'\0'},
    .buffer_index = 0,
};

// Activate keyboard ISR / start listen keyboard & save to buffer
void keyboard_state_activate(void)
{
    keyboard_state.keyboard_input_on = true;
    keyboard_state.buffer_index = 0;
    memset(keyboard_state.keyboard_buffer, 0, KEYBOARD_BUFFER_SIZE);
}

// Deactivate keyboard ISR / stop listening keyboard interrupt
void keyboard_state_deactivate(void)
{
    keyboard_state.keyboard_input_on = false;
}

// Get keyboard buffer value and flush the buffer - @param buf Pointer to char buffer
void get_keyboard_buffer(char *buf)
{
    // Copy keyboard buffer value to provided buffer
    memcpy(buf, &keyboard_state.keyboard_buffer, keyboard_state.buffer_index);
}

const char *get_scancode_to_ascii_map()
{
    if (is_capslock)
    {
        return keyboard_scancode_1_to_ascii_capslock;
    }
    else
    {
        return keyboard_scancode_1_to_ascii_map;
    }
}

bool is_keyboard_blocking(void)
{
    return keyboard_state.keyboard_input_on;
}

int8_t terminal_length = 0;

/* -- Keyboard Interrupt Service Routine -- */

/**
 * Handling keyboard interrupt & process scancodes into ASCII character.
 * Will start listen and process keyboard scancode if keyboard_input_on.
 */
void keyboard_isr(void)
{
    uint8_t scancode = in(KEYBOARD_DATA_PORT);

    // Handle capslock
    if (scancode == 0x3A)
    {
        is_capslock = !is_capslock;
        pic_ack(IRQ_KEYBOARD);
        return;
    }

    if (!keyboard_state.keyboard_input_on)
    {
        keyboard_state.buffer_index = 0;
    }
    else
    {
        uint8_t scancode = in(KEYBOARD_DATA_PORT);
        char mapped_char = keyboard_scancode_1_to_ascii_map[scancode];
        if (mapped_char == '\b')
        {
            if (col >= terminal_length + 1)
            {
                backspace_pressed = true;
                framebuffer_write(row, col - 1, ' ', 0x0F, 0x00);
                framebuffer_set_cursor(row, col - 1);
                keyboard_state.keyboard_buffer[keyboard_state.buffer_index - 1] = ' ';
            }
        }
        else if (scancode == 0x1C && !key_pressed)
        {
            keyboard_state_deactivate();
            row++;
            col = terminal_length;
            framebuffer_set_cursor(row, col);
            key_pressed = true;
        }
        else if (scancode >= 0x02 && scancode <= 0x4A && !key_pressed)
        {
            framebuffer_write(row, col, mapped_char, 0x0F, 0x00);
            framebuffer_set_cursor(row, col + 1);
            keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = mapped_char;
            key_pressed = true;
        }
        else if (scancode >= 0x80 && backspace_pressed)
        {
            backspace_pressed = false;
            if (keyboard_state.buffer_index != 0)
            {
                keyboard_state.buffer_index--;
                col--;
            }
        }
        else if (scancode >= 0x80 && scancode != 0x9C && key_pressed)
        {
            key_pressed = false;
            keyboard_state.buffer_index++;
            col++;
        }
        else if (scancode == 0x9C)
        {
            key_pressed = false;
        }
    }
    // Acknowledge the interrupt
    pic_ack(IRQ_KEYBOARD);
}

void puts(char *buf, uint32_t len, uint32_t color)
{
    for (uint8_t i = 0; i < len; i++)
    {
        framebuffer_set_cursor(row, col + i);
        if (buf[i] == '\n')
        {
            row++;
            col = 0;
            framebuffer_set_cursor(row, col);
        }
        else
        {
            framebuffer_write(row, col + i, buf[i], color, 0);
            if (i == len - 1)
            {
                col = col + len;
            }
        }
        col_bound = col;
    }
    // for (uint32_t i = 0; i < len; i++)
    // {
    //     framebuffer_write(row, col + i, buf[i], color, 0);
    // }
    // col = col + len;
}