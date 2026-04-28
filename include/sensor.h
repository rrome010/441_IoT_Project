#ifndef SMART_HOME_SENSOR_H
#define SMART_HOME_SENSOR_H

#include "event.h"

typedef struct sensor sensor_t;

struct sensor {
    const char    *name;
    sensor_type_t  type;
    int            sensor_id;
    void          *state;

    int  (*init)(sensor_t *s);
    int  (*poll)(sensor_t *s, event_t *out);
    void (*shutdown)(sensor_t *s);
};

void sensor_run(sensor_t *s, event_queue_t *q);

#endif
