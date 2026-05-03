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
} appliance_state_t;

static int appliance_init(sensor_t *s) {
    appliance_state_t *st = calloc(1, sizeof(*st));
    if (!st) return -1;

    st->prev_on = -1;
    s->state = st;
    return 0;
}

static int appliance_poll(sensor_t *s, event_t *out) {
    appliance_state_t *st = s->state;

    if (hw_mode) {
        st->on = hw_read_switch(3);
    }
    else if (random_mode) {
        st->on = rand() % 2;
    }

    if (st->on == st->prev_on) {
        return 1; // no change
    }

    st->prev_on = st->on;

    out->type      = SENSOR_APPLIANCE;
    out->sensor_id = s->sensor_id;
    out->timestamp = time(NULL);
    out->value     = st->on;

    strncpy(out->location, "living_room_tv", sizeof(out->location) - 1);
    out->location[sizeof(out->location) - 1] = '\0';

    return 0;
}

static void appliance_shutdown(sensor_t *s) {
    free(s->state);
    s->state = NULL;
}

sensor_t appliance_sensor = {
    .name      = "appliance",
    .type      = SENSOR_APPLIANCE,
    .sensor_id = 3,
    .init      = appliance_init,
    .poll      = appliance_poll,
    .shutdown  = appliance_shutdown,
};