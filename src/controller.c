#include "controller.h"
#include "event.h"
#include "hw_io.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

extern int hw_mode;

/*
 * LED mapping:
 *
 * LED0 = Door sensor status
 * LED1 = Motion sensor status
 * LED2 = Faucet sensor status
 * LED3 = Appliance / TV sensor status
 *
 * LED8 = Hall light actuator
 * LED9 = Alert indicator
 */

typedef struct {
    int door_seen;
    int hallway_motion_seen;
    int faucet_seen;
    int living_room_motion_seen;
    int tv_seen;

    int bathroom_visit_count;
    int tv_session_count;
    int faucet_alert_count;
} activity_state_t;

static activity_state_t activity_state = {0};

static unsigned int led_state = 0;

static void set_led(int bit, int value) {
    if (!hw_mode) {
        return;
    }

    if (value) {
        led_state |= (1u << bit);
    } else {
        led_state &= ~(1u << bit);
    }

    hw_write_leds(led_state);
}

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

static void update_status_leds(const event_t *ev) {
    switch (ev->type) {
        case SENSOR_DOOR:
            set_led(0, ev->value);
            break;

        case SENSOR_MOTION:
            set_led(1, ev->value);
            break;

        case SENSOR_FAUCET:
            set_led(2, ev->value);
            break;

        case SENSOR_APPLIANCE:
            set_led(3, ev->value);
            break;

        default:
            break;
    }
}

static void infer_activity(const event_t *ev) {
    /*
     * Door opened:
     * Start a possible resident movement sequence.
     */
    if (ev->type == SENSOR_DOOR && ev->value == 1) {
        activity_state.door_seen = 1;
    }

    /*
     * Motion detected:
     * Used for hallway movement and possible room transition.
     */
    if (ev->type == SENSOR_MOTION && ev->value == 1) {
        if (strcmp(ev->location, "hallway") == 0) {
            activity_state.hallway_motion_seen = 1;

            if (activity_state.door_seen) {
                printf("  INFERENCE: Resident likely entered the home or moved through the doorway.\n");
            }
        }

        /*
         * In this project, the motion sensor represents hallway/camera activity.
         * If your demo later adds a second motion sensor, this logic can be expanded.
         */
        activity_state.living_room_motion_seen = 1;
    }

    /*
     * Faucet ON:
     * Combine door + motion + faucet into bathroom activity.
     */
    if (ev->type == SENSOR_FAUCET && ev->value == 1) {
        activity_state.faucet_seen = 1;

        if (activity_state.door_seen && activity_state.hallway_motion_seen) {
            activity_state.bathroom_visit_count++;

            printf("  INFERENCE: Resident likely moved from the doorway/hallway to the bathroom and used the sink.\n");
            printf("  SUMMARY: Bathroom visit count = %d\n",
                   activity_state.bathroom_visit_count);

            /*
             * Reset only the sequence flags that produced this inference.
             */
            activity_state.door_seen = 0;
            activity_state.hallway_motion_seen = 0;
            activity_state.faucet_seen = 0;
        } else if (activity_state.hallway_motion_seen) {
            activity_state.bathroom_visit_count++;

            printf("  INFERENCE: Resident likely moved through the hallway and used the bathroom sink.\n");
            printf("  SUMMARY: Bathroom visit count = %d\n",
                   activity_state.bathroom_visit_count);

            activity_state.hallway_motion_seen = 0;
            activity_state.faucet_seen = 0;
        } else {
            printf("  INFERENCE: Water usage detected at the sink.\n");
        }
    }

    /*
     * Faucet OFF:
     * Complete faucet event.
     */
    if (ev->type == SENSOR_FAUCET && ev->value == 0) {
        if (activity_state.faucet_seen) {
            printf("  SUMMARY: Faucet activity completed.\n");
        }

        activity_state.faucet_seen = 0;
    }

    /*
     * Appliance/TV ON:
     * Combine previous motion with TV usage.
     */
    if (ev->type == SENSOR_APPLIANCE && ev->value == 1) {
        activity_state.tv_seen = 1;

        if (activity_state.living_room_motion_seen) {
            activity_state.tv_session_count++;

            printf("  INFERENCE: Resident likely moved to the living room and turned on the TV.\n");
            printf("  SUMMARY: TV session count = %d\n",
                   activity_state.tv_session_count);

            activity_state.living_room_motion_seen = 0;
            activity_state.tv_seen = 0;
        } else {
            printf("  INFERENCE: Appliance/TV use detected.\n");
        }
    }

    /*
     * Appliance/TV OFF:
     * Complete TV event.
     */
    if (ev->type == SENSOR_APPLIANCE && ev->value == 0) {
        if (activity_state.tv_seen) {
            printf("  SUMMARY: TV/appliance session completed.\n");
        }

        activity_state.tv_seen = 0;
    }
}

static void handle_event(const event_t *ev) {
    print_timestamp(ev->timestamp);

    printf("[%s] sensor=%d location=%s state=%s\n",
           sensor_type_name(ev->type),
           ev->sensor_id,
           ev->location,
           value_to_text(ev->type, ev->value));

    /*
     * Raw status LEDs:
     * LED0 = Door
     * LED1 = Motion
     * LED2 = Faucet
     * LED3 = Appliance/TV
     */
    update_status_leds(ev);

    switch (ev->type) {
        case SENSOR_DOOR:
            if (ev->value) {
                printf("  ACTIVITY: Door opened at %s. Possible entry or room transition.\n",
                       ev->location);
            } else {
                printf("  ACTIVITY: Door closed at %s.\n",
                       ev->location);
            }
            break;

        case SENSOR_MOTION:
            if (ev->value) {
                printf("  ACTIVITY: Motion detected in %s.\n",
                       ev->location);
                printf("  ACTION: Turning ON hall light.\n");

                /*
                 * LED8 is an actuator output.
                 * It simulates an automatic smart-home light.
                 */
                set_led(8, 1);
            } else {
                printf("  ACTIVITY: No motion detected in %s.\n",
                       ev->location);
                printf("  ACTION: Turning OFF hall light.\n");

                set_led(8, 0);
            }
            break;

        case SENSOR_FAUCET:
            if (ev->value) {
                printf("  ACTIVITY: Faucet is running at %s.\n",
                       ev->location);
                printf("  ACTION: Monitoring water usage.\n");
                printf("  ALERT: Faucet is currently active.\n");

                /*
                 * LED9 is the alert indicator.
                 */
                set_led(9, 1);
                activity_state.faucet_alert_count++;
            } else {
                printf("  ACTIVITY: Faucet turned OFF at %s.\n",
                       ev->location);
                printf("  ALERT CLEARED: Faucet is off.\n");

                set_led(9, 0);
            }
            break;

        case SENSOR_APPLIANCE:
            if (ev->value) {
                printf("  ACTIVITY: Appliance/TV turned ON at %s.\n",
                       ev->location);
            } else {
                printf("  ACTIVITY: Appliance/TV turned OFF at %s.\n",
                       ev->location);
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