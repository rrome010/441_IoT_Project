#include "sensor.h"
#include "hw_io.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

extern int random_mode;
extern int hw_mode;

typedef struct {
    int on;
    int prev_on;
} faucet_state_t;

static int faucet_init(sensor_t *s) {
    faucet_state_t *st = calloc(1, sizeof(*st));
    if (!st) return -1;

    st->prev_on = -1;
    s->state = st;
    return 0;
}

static int faucet_poll(sensor_t *s, event_t *out) {
    faucet_state_t *st = s->state;

    if (hw_mode) {
        st->on = hw_read_switch(2);
    }
    else if (random_mode) {
        st->on = rand() % 2;
    }

    if (st->on == st->prev_on) {
        return 1; // no change
    }

    st->prev_on = st->on;

    out->type      = SENSOR_FAUCET;
    out->sensor_id = s->sensor_id;
    out->timestamp = time(NULL);
    out->value     = st->on;

    strncpy(out->location, "bathroom_sink", sizeof(out->location) - 1);
    out->location[sizeof(out->location) - 1] = '\0';

    return 0;
}

static void faucet_shutdown(sensor_t *s) {
    free(s->state);
    s->state = NULL;
}

sensor_t faucet_sensor = {
    .name      = "faucet",
    .type      = SENSOR_FAUCET,
    .sensor_id = 2,
    .init      = faucet_init,
    .poll      = faucet_poll,
    .shutdown  = faucet_shutdown,
};