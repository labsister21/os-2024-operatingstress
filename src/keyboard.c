#include "header/driver/keyboard.h"
// #include "header/driver/framebuffer.h"
#include "header/text/framebuffer.h"
#include "header/cpu/portio.h"
#include "header/stdlib/string.h"

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

static struct KeyboardDriverState keyboard_state = {false, false, 0, 0, 0};

// Activate keyboard ISR / start listen keyboard & save to buffer
void keyboard_state_activate(void) {
    keyboard_state.keyboard_input_on = true;
}

// Deactivate keyboard ISR / stop listening keyboard interrupt
void keyboard_state_deactivate(void) {
    keyboard_state.keyboard_input_on = false;
}

// Get keyboard buffer value and flush the buffer - @param buf Pointer to char buffer
void get_keyboard_buffer(char *buf) {
        // Copy keyboard buffer value to provided buffer
    memcpy(buf, &keyboard_state.keyboard_buffer, sizeof(char));

    // Flush keyboard buffer
    keyboard_state.keyboard_buffer = '\0';
}

/* -- Keyboard Interrupt Service Routine -- */

/**
 * Handling keyboard interrupt & process scancodes into ASCII character.
 * Will start listen and process keyboard scancode if keyboard_input_on.
 */
void keyboard_isr(void) {
    uint8_t scancode = in(KEYBOARD_DATA_PORT);
    // TODO : Implement scancode processing

    // Check if the scancode is an extended scancode
    if (scancode == EXTENDED_SCANCODE_BYTE) {
        // Set the flag to indicate extended mode
        keyboard_state.read_extended_mode = true;
        return; // Exit ISR, wait for the next byte
    } else if (keyboard_state.read_extended_mode) {
        // Combine the extended byte with the current scancode
        scancode |= EXTENDED_SCANCODE_BYTE;
        // Reset the flag
        keyboard_state.read_extended_mode = false;
    }

    // Check if keyboard input is on
    if (keyboard_state.keyboard_input_on) {
        // Process scancode into ASCII character using mapping
        char ascii_char = keyboard_scancode_1_to_ascii_map[scancode];

        // Save ASCII character to keyboard buffer
        keyboard_state.keyboard_buffer = ascii_char;

        if (ascii_char == '\n') {
            keyboard_state.row += 1;
            keyboard_state.col = 0;
        }

        // keyboard_state.col++;

        // // Set cursor position
        // framebuffer_write(keyboard_state.row,  keyboard_state.col, ' ', 0xf, 0);
        // framebuffer_set_cursor(keyboard_state.row, keyboard_state.col);
    }

    // Acknowledge the interrupt
    pic_ack(IRQ_KEYBOARD);
}


// void keyboard_isr(void) {
//     uint8_t scancode = in(KEYBOARD_DATA_PORT);

//     // Check if the scancode is an extended scancode
//     if (scancode == EXTENDED_SCANCODE_BYTE) {
//         // Set the flag to indicate extended mode
//         keyboard_state.read_extended_mode = true;
//         return; // Exit ISR, wait for the next byte
//     } else if (keyboard_state.read_extended_mode) {
//         // Combine the extended byte with the current scancode
//         scancode |= EXTENDED_SCANCODE_BYTE;
//         // Reset the flag
//         keyboard_state.read_extended_mode = false;
//     }

//     // Check if keyboard input is on
//     if (keyboard_state.keyboard_input_on) {
//         // Process scancode into ASCII character using mapping
//         char ascii_char = keyboard_scancode_1_to_ascii_map[scancode];

//         // Check for special characters
//         switch (ascii_char) {
//             case '\n': // Enter key pressed
//                 // Handle enter key: Move to next line
//                 keyboard_state.row += 1;
//                 keyboard_state.col = 0;
//                 break;
//             case '\b': // Backspace key pressed
//                 // Handle backspace key: Move cursor back and clear previous character
//                 if (keyboard_state.col > 0) {
//                     keyboard_state.col -= 1;
//                     framebuffer_write(keyboard_state.row, keyboard_state.col, ' ', 0xF, 0);
//                     framebuffer_set_cursor(keyboard_state.row, keyboard_state.col);
//                 }
//                 break;
//             default:
//                 // Save ASCII character to keyboard buffer
//                 keyboard_state.keyboard_buffer = ascii_char;
                
//                 // Write character to framebuffer
//                 framebuffer_write(keyboard_state.row, keyboard_state.col, ascii_char, 0xF, 0);
                
//                 // Move cursor to next position
//                 keyboard_state.col += 1;
//                 framebuffer_set_cursor(keyboard_state.row, keyboard_state.col);
//         }
//     }

//     // Acknowledge the interrupt
//     pic_ack(IRQ_KEYBOARD);
// }