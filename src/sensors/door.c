#include "sensor.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    int open;
} door_state_t;

static int door_init(sensor_t *s) {
    door_state_t *st = calloc(1, sizeof(*st));
    if (!st) return -1;
    s->state = st;
    return 0;
}

static int door_poll(sensor_t *s, event_t *out) {
    door_state_t *st = s->state;
    sleep(3);
    st->open = !st->open;
    out->type      = SENSOR_DOOR;
    out->sensor_id = s->sensor_id;
    out->timestamp = time(NULL);
    out->value     = st->open;
    strncpy(out->location, "front_door", sizeof(out->location) - 1);
    out->location[sizeof(out->location) - 1] = '\0';
    return 0;
}

static void door_shutdown(sensor_t *s) {
    free(s->state);
    s->state = NULL;
}

sensor_t door_sensor = {
    .name      = "door",
    .type      = SENSOR_DOOR,
    .sensor_id = 0,
    .init      = door_init,
    .poll      = door_poll,
    .shutdown  = door_shutdown,
};
