#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
  bool inverted;
  uint16_t history;
  uint32_t down_time;
  uint32_t next_long_time;
} debounce_t;

void debounce_update(debounce_t *d, bool value);
bool debounce_down(debounce_t *d);
bool debounce_up(debounce_t *d);
