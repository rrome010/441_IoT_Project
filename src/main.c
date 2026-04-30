#include "controller.h"
#include "event.h"
#include "sensor.h"

#include <pthread.h>
#include <stdio.h>

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

int main(void) {
    event_queue_t *q = event_queue_create(64);
    if (!q) {
        fprintf(stderr, "queue alloc failed\n");
        return 1;
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
            return 1;
        }
    }

    controller_run(q);

    for (int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
    }

    event_queue_destroy(q);
    return 0;
}