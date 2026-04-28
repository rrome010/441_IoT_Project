# smart_home

ECE441 group project: a C application for the Terasic DE10-Standard (Cyclone V SoC, ARM Cortex-A9, Linux 4.5) that monitors household sensors, infers resident activity, and reacts to it.

## Assignment recap

- IoT-style smart home system, runs on the **HPS** (Linux side) — no VHDL/HDL required.
- Required peripherals: door, motion / camera, faucet, TV/appliance.
- Must analyze resident activity and **combine multiple sensors to infer behavior**.
- Final 3-page report due **2026-05-12**.

## Architecture

```
  sensors/*           event_t queue            controller              outputs
  +---------+         +-----------+         +-------------+         +--------+
  | door    | --push--|           |--pop--> | state_update|         | stdout |
  | motion  | --push--| mutex +   |         | rules table | --emit->| (LEDs /|
  | faucet  | --push--| condvars  |         |             |         |  relays|
  | appl.   | --push--|           |         |             |         |  later)|
  +---------+         +-----------+         +-------------+         +--------+
   1 pthread          bounded ring                                   TODO: real
   per sensor         buffer, blocks                                 GPIO out
                      on full/empty
```

- **Sensors** are independent producers. Each runs in its own pthread, polls its source, and pushes an `event_t` into the shared queue.
- **Event queue** is a bounded ring buffer with a mutex + two condvars (`not_empty`, `not_full`). Producers block when full; the controller blocks when empty.
- **Controller** is a single consumer. On every event it (1) folds the event into a `home_state_t` summary, (2) logs it, (3) runs every rule in the rule table. Rules emit actions.
- **Outputs** are stdout-only for now. A future `output.c` will mmap an FPGA register and drive LEDs / relays.

## Directory layout

```
smart_home/
├── Makefile
├── include/
│   ├── event.h          # event_t, sensor_type_t, queue API
│   ├── sensor.h         # sensor_t interface, sensor_run()
│   └── controller.h
└── src/
    ├── main.c           # spawns sensor threads, runs controller in main
    ├── event_queue.c    # bounded thread-safe queue
    ├── sensor.c         # generic sensor_run() loop
    ├── controller.c     # home_state_t + rule table
    └── sensors/
        └── door.c       # working stub; toggles every 3s
```

## Build & run

Need `gcc` and `make` (or `mingw32-make` on MSYS2 ucrt64).

```bash
cd smart_home
make           # or: mingw32-make
./smart_home
```

Expected output (one line every 3 seconds, until you Ctrl+C):

```
[14:02:31] door#0 @ front_door   value=1   [mode=UNKNOWN]
[14:02:34] door#0 @ front_door   value=0   [mode=UNKNOWN]
```

> **Heads up:** the scaffold has not been built end-to-end yet — first person to clone, run `make` and confirm clean build please.

## Adding a sensor (one per teammate)

The per-person task. One file in `src/sensors/` plus two lines elsewhere.

### 1. Copy `src/sensors/door.c` to `src/sensors/<name>.c` and adapt

```c
#include "sensor.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    /* whatever per-sensor state you need */
} my_state_t;

static int my_init(sensor_t *s) {
    s->state = calloc(1, sizeof(my_state_t));
    return s->state ? 0 : -1;
}

static int my_poll(sensor_t *s, event_t *out) {
    /* Block until your sensor has new data.
       Real hw: read from a memory-mapped FPGA register.
       Stub:    sleep() then synthesize a fake event.
       Fill in `out`, return 0. Return -1 to signal shutdown. */
}

static void my_shutdown(sensor_t *s) {
    free(s->state);
}

sensor_t my_sensor = {
    .name      = "motion",
    .type      = SENSOR_MOTION,    /* pick from event.h */
    .sensor_id = 0,
    .init      = my_init,
    .poll      = my_poll,
    .shutdown  = my_shutdown,
};
```

### 2. Wire it into `src/main.c`

```c
extern sensor_t my_sensor;
...
sensor_t *sensors[] = { &door_sensor, &my_sensor };
```

### 3. Add the source to the Makefile's `SRC` list

```make
SRC := \
    src/main.c \
    ...
    src/sensors/my_sensor.c
```

That's it. `controller.c` will see your events and run them through every rule automatically — no controller changes needed unless you want to add new inference logic.

## How the controller infers behavior

`controller.c` keeps a small `home_state_t` summarizing recent activity (mode, last motion timestamp, last door open/close, faucet-on-since, etc.).

For every event:
1. **`state_update`** folds the event into the state — no decisions, just bookkeeping.
2. The event is logged with the current mode appended.
3. Every function in `RULES[]` runs against the new event + state.

Current rules:

- `rule_arrival` — motion + door opened ≤ 30 s ago + not already HOME → mode = HOME, "turn on lights".
- `rule_forgotten_faucet` — faucet on > 5 min + no motion ≥ 5 min → warn (once per faucet session).
- `rule_appliance_while_away` — appliance turns on while AWAY → alert.

To add a rule: write `static void rule_xxx(home_state_t *st, const event_t *ev)`, add it to `RULES[]`. Done.

### Known gap: time-based rules

"Mode → AWAY after N minutes of no motion" can't fire from event arrivals alone — `event_queue_pop` blocks indefinitely. Fix is a small `event_queue_pop_timed()` (using `pthread_cond_timedwait`) plus a "tick" pass on timeout. Tagged TODO in `controller.c`.

## What's open

- `src/sensors/motion.c` — needed before any inference fires
- `src/sensors/faucet.c`
- `src/sensors/appliance.c`
- Real GPIO via `/dev/mem` mmap (replaces the `sleep()` stubs once we have hardware)
- `event_queue_pop_timed` + the AWAY-transition tick rule
- Output module (LEDs/relays) once we know what's actually wired up
- 3-page report

## Hardware notes (for when stubs become real sensors)

GPIO expansion on the DE10-Standard runs through the **FPGA fabric**, not direct ARM GPIO. The Terasic Golden Hardware Reference Design (GHRD) ships PIO blocks pre-wired to the GPIO headers and exposed across the **lightweight HPS-to-FPGA bridge** at `0xFF20_0000`. So even without writing VHDL, our sensor reads still travel through the fabric.

The pattern each `*_init` will use:

```c
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>

#define LWHPS_BASE  0xFF200000
#define LWHPS_SPAN  0x1000
#define DOOR_PIO    0x40    /* whatever Platform Designer assigned */

static int door_init(sensor_t *s) {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) return -1;
    void *base = mmap(NULL, LWHPS_SPAN, PROT_READ | PROT_WRITE,
                      MAP_SHARED, fd, LWHPS_BASE);
    close(fd);
    if (base == MAP_FAILED) return -1;
    s->state = base;
    return 0;
}

static int door_poll(sensor_t *s, event_t *out) {
    volatile uint32_t *regs = s->state;
    /* TODO: replace polling with UIO-driven blocking read once IRQ is wired */
    sleep(1);
    int v = regs[DOOR_PIO / 4] & 1;
    /* fill out as before */
    return 0;
}

