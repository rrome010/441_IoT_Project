#include "event.h"

#include <pthread.h>
#include <stdlib.h>

struct event_queue {
    event_t        *buf;
    size_t          cap;
    size_t          head;
    size_t          tail;
    size_t          size;
    pthread_mutex_t mu;
    pthread_cond_t  not_empty;
    pthread_cond_t  not_full;
};

event_queue_t *event_queue_create(size_t capacity) {
    if (capacity == 0) return NULL;

    event_queue_t *q = calloc(1, sizeof(*q));
    if (!q) return NULL;

    q->buf = calloc(capacity, sizeof(event_t));
    if (!q->buf) {
        free(q);
        return NULL;
    }

    q->cap = capacity;

    pthread_mutex_init(&q->mu, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);

    return q;
}

void event_queue_destroy(event_queue_t *q) {
    if (!q) return;

    pthread_mutex_destroy(&q->mu);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);

    free(q->buf);
    free(q);
}

int event_queue_push(event_queue_t *q, const event_t *ev) {
    pthread_mutex_lock(&q->mu);

    while (q->size == q->cap) {
        pthread_cond_wait(&q->not_full, &q->mu);
    }

    q->buf[q->tail] = *ev;
    q->tail = (q->tail + 1) % q->cap;
    q->size++;

    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mu);

    return 0;
}

int event_queue_pop(event_queue_t *q, event_t *out) {
    pthread_mutex_lock(&q->mu);

    while (q->size == 0) {
        pthread_cond_wait(&q->not_empty, &q->mu);
    }

    *out = q->buf[q->head];
    q->head = (q->head + 1) % q->cap;
    q->size--;

    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mu);

    return 0;
}

int event_queue_try_pop(event_queue_t *q, event_t *out) {
    pthread_mutex_lock(&q->mu);

    if (q->size == 0) {
        pthread_mutex_unlock(&q->mu);
        return -1;
    }

    *out = q->buf[q->head];
    q->head = (q->head + 1) % q->cap;
    q->size--;

    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mu);

    return 0;
}

const char *sensor_type_name(sensor_type_t t) {
    switch (t) {
        case SENSOR_DOOR:
            return "door";

        case SENSOR_MOTION:
            return "motion";

        case SENSOR_FAUCET:
            return "faucet";

        case SENSOR_APPLIANCE:
            return "appliance";

        case SENSOR_TEMP:
            return "temp";

        default:
            return "unknown";
    }
}