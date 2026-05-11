#include "sensor.h"

#include <signal.h>
#include <stdio.h>
#include <unistd.h>

extern volatile sig_atomic_t shutdown_requested;

void sensor_run(sensor_t *s, event_queue_t *q) {
    if (s->init && s->init(s) != 0) {
        fprintf(stderr, "sensor %s: init failed\n", s->name);
        return;
    }

    event_t ev;

    while (!shutdown_requested) {
        int result = s->poll(s, &ev);

        if (result == 0) {
            event_queue_push(q, &ev);
        }
        else if (result < 0) {
            break;
        }

        usleep(1000000); // 1 s polling delay
    }

    if (s->shutdown) {
        s->shutdown(s);
    }
}