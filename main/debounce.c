#include "debounce.h"

static const uint16_t DEBOUNCE_MASK = 0b1111000000111111;

static bool debounce_rose(debounce_t *d) {
    if ((d->history & DEBOUNCE_MASK) == 0b0000000000111111) {
        d->history = 0xffff;
        return 1;
    }
    return 0;
}

static bool debounce_fell(debounce_t *d) {
    if ((d->history & DEBOUNCE_MASK) == 0b1111000000000000) {
        d->history = 0x0000;
        return 1;
    }
    return 0;
}

void debounce_update(debounce_t *d, bool value) {
    d->history = (d->history << 1) | value;
}

bool debounce_down(debounce_t *d) {
    if (d->inverted) return debounce_fell(d);
    return debounce_rose(d);
}

bool debounce_up(debounce_t *d) {
    if (d->inverted) return debounce_rose(d);
    return debounce_fell(d);
}

