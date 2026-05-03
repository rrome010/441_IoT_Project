# smart_home

ECE441 group project: a C application for the Terasic DE10-Standard (Cyclone V SoC, ARM Cortex-A9, Linux 4.5) that monitors household sensors, infers resident activity, and reacts to it.

## Assignment recap

- IoT-style smart home system, runs on the **HPS** (Linux side) — no VHDL/HDL required.
- Required peripherals: door, motion / camera, faucet, TV/appliance.
- Must analyze resident activity and **combine multiple sensors to infer behavior**.
- Final 3-page report due **2026-05-12**.

## Architecture
sensors/* event_t queue controller outputs
+---------+ +-----------+ +-------------+ +--------+
| door | --push--| |--pop--> | state_update| | stdout |
| motion | --push--| mutex + | | rules table | --emit->| (LEDs /|
| faucet | --push--| condvars | | inference | | relays|
| appl. | --push--| | | | | later)|
+---------+ +-----------+ +-------------+ +--------+
1 pthread bounded ring TODO: real
per sensor buffer, blocks GPIO out
on full/empty

- **Sensors** are independent producers. Each runs in its own pthread, polls its source, and pushes an `event_t` into the shared queue.
- **Event queue** is a bounded ring buffer with a mutex + two condvars (`not_empty`, `not_full`). Producers block when full; the controller blocks when empty.
- **Controller** is a single consumer. On every event it (1) folds the event into a `home_state_t` summary, (2) logs it, (3) runs inference rules that emit actions.
- **Outputs** are stdout-only for now. A future module will mmap FPGA registers to drive LEDs / relays.

## Modes of operation

The system now supports multiple runtime modes:

### Default (Hardware/Input Mode)

```bash
./smart_home
Intended for DE10-Standard switch/button input
Currently uses placeholder values until hardware is connected
Random Simulation Mode
./smart_home --random
Sensors generate randomized events
Useful for testing system behavior
Demo Mode
./smart_home --demo
Runs a fixed scripted sequence of events
Ensures predictable output for presentation

Example demo flow:
Door opens
Motion detected (hallway)
Faucet ON
Faucet OFF
Motion detected (living room)
TV ON
Directory layout
smart_home/
├── Makefile
├── include/
│   ├── event.h
│   ├── sensor.h
│   ├── controller.h
│   └── demo.h
└── src/
    ├── main.c
    ├── event_queue.c
    ├── sensor.c
    ├── controller.c
    ├── demo.c
    └── sensors/
        ├── door.c
        ├── motion.c
        ├── faucet.c
        └── appliance.c

Build & run

Need gcc and make.
cd smart_home
make
./smart_home
Current system behavior
Each sensor runs in its own thread and continuously produces events
Events are pushed into a shared queue and processed by the controller
The controller prints readable logs and performs activity inference

Example output:
[14:02:31] door#0 @ front_door   value=1
  ACTIVITY: Door opened → possible entry

[14:02:34] motion#1 @ hallway   value=1
  ACTION: Light ON
  Sensors implemented
src/sensors/door.c
src/sensors/motion.c
src/sensors/faucet.c
src/sensors/appliance.c

Each follows the same sensor_t interface:

init
poll
shutdown
How the controller infers behavior

The controller keeps a small internal state and evaluates rules per event.

Examples:

Motion → turn on light
Faucet ON → resident using sink
Motion + Faucet → resident moved to bathroom and used sink
Motion (living room) + Appliance ON → resident watching TV
Door OPEN → entry / movement event

To add a rule:

write a function in controller.c
add it to the rule set
no changes needed to sensors or queue
Adding a sensor
Copy src/sensors/door.c
Implement init, poll, shutdown
Register in main.c
Add file to Makefile
What's completed
Multi-threaded sensor system
Thread-safe event queue
Controller with inference logic
Four working sensors
Runtime mode switching (hardware / random / demo)
What's open
Real GPIO input via /dev/mem
Interrupt-based sensor reads (instead of polling)
Output module (LEDs / relays)
Time-based inference rules (e.g., AWAY mode)
Final report
Hardware notes

GPIO on the DE10-Standard is exposed through the FPGA fabric via the lightweight HPS-to-FPGA bridge (0xFF200000).

Future implementation:
/dev/mem → mmap → PIO registers
Example mapping:
SW0 → Door
SW1 → Motion
SW2 → Faucet
SW3 → Appliance
Summary

This project implements a modular IoT smart home system using:

multi-threaded sensors
event-driven architecture
centralized controller
activity inference
multiple runtime modes

The design supports both simulation and real hardware integration.