#ifndef SMART_HOME_EVENT_H
#define SMART_HOME_EVENT_H

#include <stddef.h>
#include <time.h>

typedef enum {
    SENSOR_DOOR,
    SENSOR_MOTION,
    SENSOR_FAUCET,
    SENSOR_APPLIANCE,
    SENSOR_TEMP,
    SENSOR_TYPE_COUNT,
} sensor_type_t;

typedef struct {
    sensor_type_t type;
    int           sensor_id;
    time_t        timestamp;
    int           value;
    char          location[32];
} event_t;

const char *sensor_type_name(sensor_type_t t);

typedef struct event_queue event_queue_t;

event_queue_t *event_queue_create(size_t capacity);
void           event_queue_destroy(event_queue_t *q);
int            event_queue_push(event_queue_t *q, const event_t *ev);
int            event_queue_pop(event_queue_t *q, event_t *out);

#endif
