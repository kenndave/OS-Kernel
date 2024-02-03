//
// Created by zidane on 4/16/23.
//

#ifndef TUBESOS_FRAME_H
#define TUBESOS_FRAME_H

#include "stdtype.h"
#include "framebuffer.h"

/*
 * */
struct FrameCursor {
    uint8_t vertical_index;
    uint8_t horizontal_index;
    uint8_t vertical_bound_index;
    uint8_t horizontal_bound_index;
    int horizontal_end_index[FRAME_HORIZONTAL_SIZE];
} __attribute__((packed));


void scroll_down();

void check_scroll_down();

void new_line();

void cursor_shift_right();

void cursor_shift_left();

void update_cursor();

void puts(const char *char_buffer, uint32_t char_count, uint32_t text_color);

#endif //TUBESOS_FRAME_H
