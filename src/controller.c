#include "controller.h"

#include <stdio.h>
#include <time.h>

#define ARRIVAL_WINDOW_S   30
#define FAUCET_WARN_S      (5 * 60)
#define MOTION_IDLE_S      (5 * 60)

typedef enum {
    MODE_UNKNOWN,
    MODE_HOME,
    MODE_AWAY,
} home_mode_t;

typedef struct {
    home_mode_t mode;
    time_t      last_motion_ts;
    time_t      last_door_open_ts;
    time_t      last_door_close_ts;
    int         door_open;
    time_t      faucet_on_since;   /* 0 if off */
    int         faucet_warned;
    int         appliance_on;
} home_state_t;

static const char *mode_name(home_mode_t m) {
    switch (m) {
    case MODE_HOME: return "HOME";
    case MODE_AWAY: return "AWAY";
    default:        return "UNKNOWN";
    }
}

static void emit_action(const event_t *trigger, const char *action) {
    char ts[32];
    struct tm *tm = localtime(&trigger->timestamp);
    strftime(ts, sizeof(ts), "%H:%M:%S", tm);
    printf("[%s] >>> %s\n", ts, action);
}

static void state_update(home_state_t *st, const event_t *ev) {
    switch (ev->type) {
    case SENSOR_DOOR:
        st->door_open = ev->value;
        if (ev->value) st->last_door_open_ts  = ev->timestamp;
        else           st->last_door_close_ts = ev->timestamp;
        break;
    case SENSOR_MOTION:
        if (ev->value) st->last_motion_ts = ev->timestamp;
        break;
    case SENSOR_FAUCET:
        if (ev->value && st->faucet_on_since == 0) {
            st->faucet_on_since = ev->timestamp;
            st->faucet_warned   = 0;
        } else if (!ev->value) {
            st->faucet_on_since = 0;
            st->faucet_warned   = 0;
        }
        break;
    case SENSOR_APPLIANCE:
        st->appliance_on = ev->value;
        break;
    default:
        break;
    }
}

typedef void (*rule_fn)(home_state_t *, const event_t *);

static void rule_arrival(home_state_t *st, const event_t *ev) {
    if (ev->type != SENSOR_MOTION || !ev->value) return;
    if (st->last_door_open_ts == 0) return;
    if (ev->timestamp - st->last_door_open_ts > ARRIVAL_WINDOW_S) return;
    if (st->mode == MODE_HOME) return;
    st->mode = MODE_HOME;
    emit_action(ev, "ARRIVAL — turn on lights");
}

static void rule_forgotten_faucet(home_state_t *st, const event_t *ev) {
    if (st->faucet_on_since == 0 || st->faucet_warned) return;
    if (ev->timestamp - st->faucet_on_since < FAUCET_WARN_S) return;
    if (st->last_motion_ts != 0 &&
        ev->timestamp - st->last_motion_ts < MOTION_IDLE_S) return;
    emit_action(ev, "WARN: faucet running with no motion >5 min");
    st->faucet_warned = 1;
}

static void rule_appliance_while_away(home_state_t *st, const event_t *ev) {
    if (ev->type != SENSOR_APPLIANCE || !ev->value) return;
    if (st->mode != MODE_AWAY) return;
    emit_action(ev, "ALERT: appliance turned on while away");
}

static const rule_fn RULES[] = {
    rule_arrival,
    rule_forgotten_faucet,
    rule_appliance_while_away,
};

void controller_run(event_queue_t *q) {
    home_state_t st = { .mode = MODE_UNKNOWN };
    event_t ev;
    while (event_queue_pop(q, &ev) == 0) {
        state_update(&st, &ev);

        char ts[32];
        struct tm *tm = localtime(&ev.timestamp);
        strftime(ts, sizeof(ts), "%H:%M:%S", tm);
        printf("[%s] %s#%d @ %-12s value=%d   [mode=%s]\n",
               ts, sensor_type_name(ev.type), ev.sensor_id,
               ev.location, ev.value, mode_name(st.mode));

        for (size_t i = 0; i < sizeof(RULES) / sizeof(RULES[0]); i++)
            RULES[i](&st, &ev);

        fflush(stdout);
    }
    /* TODO: time-based rules (e.g., transition to AWAY after N min of no
       motion) need a periodic wake-up. Add event_queue_pop_timed() and run
       a tick pass on timeout. */
}
