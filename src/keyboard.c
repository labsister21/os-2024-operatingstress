#include "header/driver/keyboard.h"
// #include "header/driver/framebuffer.h"
#include "header/text/framebuffer.h"
#include "header/cpu/portio.h"
#include "header/stdlib/string.h"

static bool is_capslock = false;

int col = 0;
int row = 0;
int col_recent = 0;
int col_bound = 0;

const char keyboard_scancode_1_to_ascii_map[256] = {
      0, 0x1B, '1', '2', '3', '4', '5', '6',  '7', '8', '9',  '0',  '-', '=', '\b', '\t',
    'q',  'w', 'e', 'r', 't', 'y', 'u', 'i',  'o', 'p', '[',  ']', '\n',   0,  'a',  's',
    'd',  'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0, '\\',  'z', 'x',  'c',  'v',
    'b',  'n', 'm', ',', '.', '/',   0, '*',    0, ' ',   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0, '-',    0,    0,   0,  '+',    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,

      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
};

const char keyboard_scancode_1_to_ascii_capslock[256] = {
      0, 0x1B, '1', '2', '3', '4', '5', '6',  '7', '8', '9',  '0',  '-', '=', '\b', '\t',
    'Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I',  'O', 'P', '[',  ']', '\n',   0,  'A',  'S',
    'D',  'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`',   0, '\\',  'Z', 'X',  'C',  'V',
    'B',  'N', 'M', ',', '.', '/',   0, '*',    0, ' ',   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0, '-',    0,    0,   0,  '+',    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,

      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
};


static struct KeyboardDriverState keyboard_state = {
    .read_extended_mode = false,
    .keyboard_input_on = false,
    .keyboard_buffer = {'\0'},
    .buffer_index = 0,
};

// Activate keyboard ISR / start listen keyboard & save to buffer
void keyboard_state_activate(void) {
    keyboard_state.keyboard_input_on = true;
    keyboard_state.buffer_index = 0;
    memset(keyboard_state.keyboard_buffer, 0, KEYBOARD_BUFFER_SIZE);
}

// Deactivate keyboard ISR / stop listening keyboard interrupt
void keyboard_state_deactivate(void) {
    keyboard_state.keyboard_input_on = false;
}

// Get keyboard buffer value and flush the buffer - @param buf Pointer to char buffer
void get_keyboard_buffer(char *buf) {
        // Copy keyboard buffer value to provided buffer
    // memcpy(buf, &keyboard_state.keyboard_buffer, sizeof(keyboard_state.buffer_index));

    memcpy(buf, &keyboard_state.keyboard_buffer, 120);

    // memcpy(buf, &keyboard_state.keyboard_buffer, KEYBOARD_BUFFER_SIZE);

    // memcpy(buf, &keyboard_state.keyboard_buffer, strlen(buf));

    // memcpy(buf, &keyboard_state.keyboard_buffer, keyboard_state.buffer_index);

    // for (int i = 0; i < KEYBOARD_BUFFER_SIZE; i++) {
    //     buf[i] = keyboard_state.keyboard_buffer[i];
    // }

    // Flush keyboard buffer
    // keyboard_state.keyboard_buffer = '\0';
}

const char* get_scancode_to_ascii_map() {
    if (is_capslock) {
        return keyboard_scancode_1_to_ascii_capslock;
    }
    else {
        return keyboard_scancode_1_to_ascii_map;
    }
}

bool is_keyboard_blocking(void) {
    return keyboard_state.keyboard_input_on;
}

/* -- Keyboard Interrupt Service Routine -- */

/**
 * Handling keyboard interrupt & process scancodes into ASCII character.
 * Will start listen and process keyboard scancode if keyboard_input_on.
 */
void keyboard_isr(void) {
    uint8_t scancode = in(KEYBOARD_DATA_PORT);

    // Handle capslock
    if (scancode == 0x3A) {
        is_capslock = !is_capslock;
        pic_ack(IRQ_KEYBOARD);
        return;
    } 

    if (!keyboard_state.keyboard_input_on) {
        keyboard_state.buffer_index = 0;
    } else {
        uint8_t scancode = in(KEYBOARD_DATA_PORT);
        char ascii_char = get_scancode_to_ascii_map()[scancode];

        if (ascii_char != 0) {
            if (ascii_char == '\b') { // Backspace
                if (col > 0) {
                    if (col > col_bound && (keyboard_state.buffer_index != 0)) {
                        framebuffer_write(row, --col, ' ', 0xF, 0); // Remove character
                        framebuffer_set_cursor(row, col);
                    }
                }
                else if (col == 0 && row > 0) {
                    row--; // Move to the previous row
                    col = col_recent; // Move to the last column of the previous row
                    framebuffer_write(row, col, ' ', 0xF, 0); // Remove character
                    framebuffer_set_cursor(row, col);
                }
                // keyboard_state.keyboard_buffer[keyboard_state.buffer_index-1] = ' ';
                if(keyboard_state.buffer_index > 0){
                    keyboard_state.buffer_index--;
                }
            } else if (ascii_char == '\n') { // Enter
                // col_bound = 0; 
                row++;
                col_recent = col;
                col = 0; // Move to the next line
                keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = '\0';
                keyboard_state.buffer_index = 0;
                framebuffer_set_cursor(row, col_bound);
                keyboard_state_deactivate();
            } else { // Regular character
                keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = ascii_char;
                keyboard_state.buffer_index++;
                framebuffer_write(row, col++, ascii_char, 0xF, 0);
                framebuffer_write(row, col, ' ', 0xf, 0);
                framebuffer_set_cursor(row, col);
            }
        }
    }
    // Acknowledge the interrupt
    pic_ack(IRQ_KEYBOARD);
}

void puts(char *buf, uint32_t len, uint32_t color) {
    for (uint8_t i = 0; i < len; i++) {
        framebuffer_set_cursor(row, col + i);
        if (buf[i] == '\n') {
            row++;
            col = 0;
            framebuffer_set_cursor(row, col);
        } else {
            framebuffer_write(row, col + i, buf[i], color, 0);
            if(i == len - 1) {
                col = col + len;
            }
        }
        col_bound = col;
  }
}

void setCursorCLS(){
    row = 0;
    col = col_bound;
}