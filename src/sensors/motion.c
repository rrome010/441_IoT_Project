#include "sensor.h"
#include "hw_io.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

extern int random_mode;
extern int hw_mode;

typedef struct {
    int motion_detected;
    int prev_motion_detected;
} motion_state_t;

static int motion_init(sensor_t *s) {
    motion_state_t *st = calloc(1, sizeof(*st));
    if (!st) return -1;

    if (hw_mode) {
        st->motion_detected      = hw_read_switch(1);
        st->prev_motion_detected = st->motion_detected;
    } else {
        st->prev_motion_detected = -1;
    }
    s->state = st;
    return 0;
}

static int motion_poll(sensor_t *s, event_t *out) {
    motion_state_t *st = s->state;

    if (hw_mode) {
        st->motion_detected = hw_read_switch(1);
    }
    else if (random_mode) {
        st->motion_detected = rand() % 2;
    }

    if (st->motion_detected == st->prev_motion_detected) {
        return 1; // no change
    }

    st->prev_motion_detected = st->motion_detected;

    out->type      = SENSOR_MOTION;
    out->sensor_id = s->sensor_id;
    out->timestamp = time(NULL);
    out->value     = st->motion_detected;

    strncpy(out->location, "hallway", sizeof(out->location) - 1);
    out->location[sizeof(out->location) - 1] = '\0';

    return 0;
}

static void motion_shutdown(sensor_t *s) {
    free(s->state);
    s->state = NULL;
}

sensor_t motion_sensor = {
    .name      = "motion",
    .type      = SENSOR_MOTION,
    .sensor_id = 1,
    .init      = motion_init,
    .poll      = motion_poll,
    .shutdown  = motion_shutdown,
};