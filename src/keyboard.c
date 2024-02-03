#include "lib-header/keyboard.h"
#include "lib-header/portio.h"
#include "lib-header/frame.h"
#include "lib-header/stdmem.h"

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

struct KeyboardDriverState keyboard_state;
extern struct FrameCursor cursor;

void process_key() {
    uint8_t scancode= in(KEYBOARD_DATA_PORT);
    char mapped_char = keyboard_scancode_1_to_ascii_map[scancode];

    switch (mapped_char) {
        case 0:
            return;
        case '\b':
            // to avoid erasing terminal symbols
            if (cursor.horizontal_index <= cursor.horizontal_bound_index &&
                cursor.vertical_index <= cursor.vertical_bound_index) {
                return;
            }

            // erase char
            framebuffer_write(cursor.vertical_index, cursor.horizontal_index - 1, 0, 0, 0);
            keyboard_state.buffer_index -= 1;
            cursor_shift_left();
            break;
        case '\n':
            new_line();
            keyboard_state_deactivate();
            break;
        default:
            if (keyboard_state.buffer_index < KEYBOARD_BUFFER_SIZE - 1) {
                keyboard_state.keyboard_buffer[keyboard_state.buffer_index++] = mapped_char;
            }
            framebuffer_write(cursor.vertical_index, cursor.horizontal_index, mapped_char, 0xF, 0);
            cursor_shift_right();
    }

}

void keyboard_isr(void) {
    if (!keyboard_state.keyboard_input_on) {
        keyboard_state.buffer_index = 0;
    } else {
        process_key();
    }
    pic_ack(IRQ_KEYBOARD);
}

void keyboard_state_activate(void) {
    cursor.horizontal_bound_index = cursor.horizontal_index;
    cursor.vertical_bound_index = cursor.vertical_index;
    keyboard_state.keyboard_input_on = TRUE;
    keyboard_state.buffer_index = 0;
}

void keyboard_state_deactivate(void) {
    keyboard_state.keyboard_input_on = FALSE;
}

void get_keyboard_buffer(__attribute__ ((unused))char *buf) {
    keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = '\0';
    memcpy(buf, keyboard_state.keyboard_buffer, KEYBOARD_BUFFER_SIZE);
}

bool is_keyboard_blocking(void) {
    return keyboard_state.keyboard_input_on;
}
