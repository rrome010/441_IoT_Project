#include "sensor.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    int on;
    int ticks_on;
} appliance_state_t;

static int appliance_init(sensor_t *s) {
    appliance_state_t *st = calloc(1, sizeof(*st));
    if (!st) return -1;

    s->state = st;
    return 0;
}

static int appliance_poll(sensor_t *s, event_t *out) {
    appliance_state_t *st = s->state;

    sleep(5);

    /*
     * Simulated appliance/TV behavior:
     * - Randomly turns on/off
     * - Tracks how long it has been on using ticks
     */
    st->on = rand() % 2;

    if (st->on) {
        st->ticks_on++;
    } else {
        st->ticks_on = 0;
    }

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
    .name      = "appliance_timer",
    .type      = SENSOR_APPLIANCE,
    .sensor_id = 3,
    .init      = appliance_init,
    .poll      = appliance_poll,
    .shutdown  = appliance_shutdown,
};