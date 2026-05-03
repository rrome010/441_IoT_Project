#include "demo.h"
#include "event.h"

#include <string.h>
#include <time.h>
#include <unistd.h>

static void make_event(event_t *ev,
                       sensor_type_t type,
                       int sensor_id,
                       int value,
                       const char *location) {
    ev->type = type;
    ev->sensor_id = sensor_id;
    ev->timestamp = time(NULL);
    ev->value = value;

    strncpy(ev->location, location, sizeof(ev->location) - 1);
    ev->location[sizeof(ev->location) - 1] = '\0';
}

void demo_run(event_queue_t *q) {
    event_t ev;

    make_event(&ev, SENSOR_DOOR, 0, 1, "front_door");
    event_queue_push(q, &ev);
    sleep(1);

    make_event(&ev, SENSOR_MOTION, 1, 1, "hallway");
    event_queue_push(q, &ev);
    sleep(1);

    make_event(&ev, SENSOR_FAUCET, 2, 1, "bathroom_sink");
    event_queue_push(q, &ev);
    sleep(1);

    make_event(&ev, SENSOR_FAUCET, 2, 0, "bathroom_sink");
    event_queue_push(q, &ev);
    sleep(1);

    make_event(&ev, SENSOR_MOTION, 1, 1, "living_room");
    event_queue_push(q, &ev);
    sleep(1);

    make_event(&ev, SENSOR_APPLIANCE, 3, 1, "living_room_tv");
    event_queue_push(q, &ev);
    sleep(1);

    make_event(&ev, SENSOR_APPLIANCE, 3, 0, "living_room_tv");
    event_queue_push(q, &ev);
}