#include "controller.h"
#include "demo.h"
#include "event.h"
#include "hw_io.h"
#include "sensor.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>

int random_mode = 0;
int demo_mode   = 0;
int hw_mode     = 1;

extern sensor_t door_sensor;
extern sensor_t motion_sensor;
extern sensor_t faucet_sensor;
extern sensor_t appliance_sensor;

typedef struct {
    sensor_t      *s;
    event_queue_t *q;
} sensor_args_t;

static void *sensor_thread(void *arg) {
    sensor_args_t *a = arg;
    sensor_run(a->s, a->q);
    return NULL;
}

static void *controller_thread(void *arg) {
    event_queue_t *q = arg;
    controller_run(q);
    return NULL;
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--random") == 0) {
            random_mode = 1;
            demo_mode   = 0;
            hw_mode     = 0;
        }
        else if (strcmp(argv[i], "--demo") == 0) {
            demo_mode   = 1;
            random_mode = 0;
            hw_mode     = 0;
        }
    }

    if (hw_mode) {
        if (hw_init() != 0) {
            fprintf(stderr, "hardware init failed\n");
            return 1;
        }
    }

    event_queue_t *q = event_queue_create(64);
    if (!q) {
        fprintf(stderr, "queue alloc failed\n");
        if (hw_mode) hw_shutdown();
        return 1;
    }

    if (demo_mode) {
        pthread_t controller;

        if (pthread_create(&controller, NULL, controller_thread, q) != 0) {
            fprintf(stderr, "failed to create controller thread\n");
            event_queue_destroy(q);
            return 1;
        }

        demo_run(q);

        pthread_join(controller, NULL);
        event_queue_destroy(q);
        return 0;
    }

    sensor_t *sensors[] = {
        &door_sensor,
        &motion_sensor,
        &faucet_sensor,
        &appliance_sensor
    };

    enum { N = sizeof(sensors) / sizeof(sensors[0]) };

    pthread_t     threads[N];
    sensor_args_t args[N];

    for (int i = 0; i < N; i++) {
        args[i].s = sensors[i];
        args[i].q = q;

        if (pthread_create(&threads[i], NULL, sensor_thread, &args[i]) != 0) {
            fprintf(stderr, "failed to create sensor thread\n");
            event_queue_destroy(q);
            if (hw_mode) hw_shutdown();
            return 1;
        }
    }

    controller_run(q);

    for (int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
    }

    event_queue_destroy(q);

    if (hw_mode) {
        hw_shutdown();
    }

    return 0;
}