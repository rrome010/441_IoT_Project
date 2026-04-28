#include "sensor.h"

#include <stdio.h>

void sensor_run(sensor_t *s, event_queue_t *q) {
    if (s->init && s->init(s) != 0) {
        fprintf(stderr, "sensor %s: init failed\n", s->name);
        return;
    }
    event_t ev;
    while (s->poll(s, &ev) == 0) {
        event_queue_push(q, &ev);
    }
    if (s->shutdown) s->shutdown(s);
}
