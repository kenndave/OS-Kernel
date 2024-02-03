#include "lib-header/frame.h"
#include "lib-header/stdmem.h"

struct FrameCursor cursor = {
        .vertical_index = 0,
        .horizontal_index = 0
};

void scroll_down() {
    for (int i = 0; i < FRAME_VERTICAL_SIZE-1; ++i) {
        memcpy((uint16_t*)MEMORY_FRAMEBUFFER + (i* 80),
               (uint16_t *)MEMORY_FRAMEBUFFER + ((i+1)* 80), FRAME_HORIZONTAL_SIZE-1);
        cursor.horizontal_end_index[i] = cursor.horizontal_end_index[i+1];
    }
    for (int j = 0; j < FRAME_HORIZONTAL_SIZE; ++j) {
        framebuffer_write(FRAME_VERTICAL_SIZE-1, j, 0, 0, 0);
    }
}

void check_scroll_down() {
    if (cursor.vertical_index >= FRAME_VERTICAL_SIZE-1) {
        // add_line_history();
        scroll_down();
        --cursor.vertical_index;
    }
}

void new_line() {
    cursor.horizontal_end_index[cursor.vertical_index] = cursor.horizontal_index;
    check_scroll_down();
    ++cursor.vertical_index;
    cursor.horizontal_index = 0;
}

void cursor_shift_right() {
    // new row
    if (cursor.horizontal_index >= FRAME_HORIZONTAL_SIZE - 1) {
        new_line();
        return;
    }
    // shift right
    ++cursor.horizontal_index;
    update_cursor();
}

void cursor_shift_left() {
    if (cursor.horizontal_index == 0 && cursor.vertical_index == 0) {
        return;
    }
    // move up row
    if (cursor.horizontal_index == 0) {
        --cursor.vertical_index;

        // restore cursor to prev index
        cursor.horizontal_index = cursor.horizontal_end_index[cursor.vertical_index];
        return;
    }
    --cursor.horizontal_index;
    update_cursor();
}

void update_cursor() {
    // untuk sekarang, karena belum ada fitur arrow maka 1 posisi framebuffer di write sbg karakter 0 dengan
    // foreground putih agar cursor muncul. ketika terdapat implementasi arrow (kanan-kiri), maka line berikut
    // harus diganti agar tidak meng-overwrite existing memory dengan char 0.
    framebuffer_write(cursor.vertical_index, cursor.horizontal_index, 0, 0xF, 0);
    framebuffer_set_cursor(cursor.vertical_index, cursor.horizontal_index);
}

void puts(const char *char_buffer, uint32_t char_count, uint32_t text_color) {
    for (uint32_t i = 0; i < char_count; ++i) {
        if (char_buffer[i] == '\n') {
            new_line();
            continue;
        }
        framebuffer_write(cursor.vertical_index, cursor.horizontal_index, char_buffer[i], text_color, 0);
        cursor_shift_right();
    }
}