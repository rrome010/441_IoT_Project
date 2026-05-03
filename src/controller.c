#include "controller.h"
#include "event.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

typedef struct {
    int door_recently_opened;
    int hallway_motion_seen;
    int bathroom_faucet_seen;
    int living_room_motion_seen;
} activity_state_t;

static activity_state_t activity_state = {0};

static void print_timestamp(time_t timestamp) {
    char buffer[32];
    struct tm *tm_info = localtime(&timestamp);

    if (tm_info) {
        strftime(buffer, sizeof(buffer), "%H:%M:%S", tm_info);
        printf("[%s] ", buffer);
    }
}

static const char *value_to_text(sensor_type_t type, int value) {
    switch (type) {
        case SENSOR_DOOR:
            return value ? "OPEN" : "CLOSED";
        case SENSOR_MOTION:
            return value ? "MOTION DETECTED" : "NO MOTION";
        case SENSOR_FAUCET:
            return value ? "ON" : "OFF";
        case SENSOR_APPLIANCE:
            return value ? "ON" : "OFF";
        case SENSOR_TEMP:
            return "TEMPERATURE READING";
        default:
            return "UNKNOWN";
    }
}

static void infer_activity(const event_t *ev) {
    if (ev->type == SENSOR_DOOR && ev->value == 1) {
        activity_state.door_recently_opened = 1;
    }

    if (ev->type == SENSOR_MOTION && ev->value == 1) {
        if (strcmp(ev->location, "hallway") == 0) {
            activity_state.hallway_motion_seen = 1;

            if (activity_state.door_recently_opened) {
                printf("  INFERENCE: Resident likely entered the home or moved through the doorway.\n");
            }
        }

        if (strcmp(ev->location, "living_room") == 0 ||
            strcmp(ev->location, "living_room_tv") == 0) {
            activity_state.living_room_motion_seen = 1;
        }
    }

    if (ev->type == SENSOR_FAUCET && ev->value == 1) {
        activity_state.bathroom_faucet_seen = 1;

        if (activity_state.hallway_motion_seen) {
            printf("  INFERENCE: Resident likely moved from hallway to bathroom and used the sink.\n");
        } else {
            printf("  INFERENCE: Resident is likely using water at the sink.\n");
        }
    }

    if (ev->type == SENSOR_APPLIANCE && ev->value == 1) {
        if (activity_state.living_room_motion_seen) {
            printf("  INFERENCE: Resident likely entered the living room and turned on the TV.\n");
        } else {
            printf("  INFERENCE: Appliance/TV use detected.\n");
        }
    }

    /*
     * Reset simple inference state after a meaningful activity chain.
     * This prevents the same old events from causing repeated conclusions forever.
     */
    if (activity_state.hallway_motion_seen &&
        activity_state.bathroom_faucet_seen &&
        ev->type == SENSOR_FAUCET &&
        ev->value == 0) {
        printf("  SUMMARY: Bathroom sink activity completed.\n");

        activity_state.door_recently_opened = 0;
        activity_state.hallway_motion_seen = 0;
        activity_state.bathroom_faucet_seen = 0;
    }
}

static void handle_event(const event_t *ev) {
    print_timestamp(ev->timestamp);

    printf("[%s] sensor=%d location=%s state=%s\n",
           sensor_type_name(ev->type),
           ev->sensor_id,
           ev->location,
           value_to_text(ev->type, ev->value));

    switch (ev->type) {
        case SENSOR_DOOR:
            if (ev->value) {
                printf("  ACTIVITY: Door opened at %s. Possible entry or room transition.\n",
                       ev->location);
            } else {
                printf("  ACTIVITY: Door closed at %s.\n", ev->location);
            }
            break;

        case SENSOR_MOTION:
            if (ev->value) {
                printf("  ACTIVITY: Motion detected in %s.\n", ev->location);
                printf("  ACTION: Turning ON light near %s.\n", ev->location);
            } else {
                printf("  ACTIVITY: No motion detected in %s.\n", ev->location);
            }
            break;

        case SENSOR_FAUCET:
            if (ev->value) {
                printf("  ACTIVITY: Faucet is running at %s.\n", ev->location);
                printf("  ACTION: Monitoring water usage.\n");
            } else {
                printf("  ACTIVITY: Faucet turned OFF at %s.\n", ev->location);
            }
            break;

        case SENSOR_APPLIANCE:
            if (ev->value) {
                printf("  ACTIVITY: Appliance/TV turned ON at %s.\n", ev->location);
            } else {
                printf("  ACTIVITY: Appliance/TV turned OFF at %s.\n", ev->location);
            }
            break;

        case SENSOR_TEMP:
            printf("  ACTIVITY: Temperature sensor reading received from %s.\n",
                   ev->location);
            break;

        default:
            printf("  WARNING: Unknown sensor event received.\n");
            break;
    }

    infer_activity(ev);

    printf("\n");
}

void controller_run(event_queue_t *q) {
    event_t ev;

    while (1) {
        if (event_queue_pop(q, &ev) == 0) {
            handle_event(&ev);
        }
    }
}