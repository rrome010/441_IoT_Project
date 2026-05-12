# smart_home

ECE 441 group project: a C application for the Terasic DE10-Standard Cyclone V SoC that simulates a smart home IoT system.

The system monitors household sensor inputs, infers resident activity, and reacts using console output and DE10 LEDs.

## Assignment recap

- IoT-style smart home system running on the HPS/Linux side.
- Required peripherals:
  - Door sensor
  - Motion/camera sensor
  - Faucet sensor
  - TV/appliance sensor
- System must combine sensor data to infer resident behavior.
- System should produce actions, alerts, and activity summaries.

## Architecture

```text
sensors / switches        controller logic             outputs
+----------------+        +----------------+           +----------------+
| door sensor    | -----> | state tracking | --------> | stdout log     |
| motion sensor  | -----> | activity rules | --------> | LED indicators |
| faucet sensor  | -----> | alert checks   | --------> | summary stats  |
| TV/appliance   | -----> | timing logic   |           |                |
+----------------+        +----------------+           +----------------+
```

The smart home controller reads sensor states, tracks timing information, and determines whether the resident is entering, moving through the home, using the faucet, watching TV, or causing an alert condition.

## Hardware mapping

### Inputs

| Input | Function |
|------|----------|
| SW0 | Door sensor |
| SW1 | Motion sensor |
| SW2 | Faucet sensor |
| SW3 | TV/appliance sensor |
| KEY0 | Reset / optional trigger |

### Outputs

| LED | Meaning |
|-----|---------|
| LED0 | Door active |
| LED1 | Motion detected |
| LED2 | Faucet running |
| LED3 | TV/appliance active |
| LED6 | Water alert |
| LED8 | Automatic hallway light |


## System behavior

The system operates using both direct sensor feedback and inferred activity.

### Direct sensor feedback

```text
SW0 -> LED0 -> Door active
SW1 -> LED1 -> Motion detected
SW2 -> LED2 -> Faucet running
SW3 -> LED3 -> TV/appliance active
```

### Inferred outputs

LED6 and LED8 are not direct sensor indicators.  
They are derived from controller logic.

```text
Motion detected -> LED8 hallway light turns ON
Faucet active too long -> LED6 alert turns ON
```

## Activity rules

The controller uses simple activity rules to infer behavior from multiple inputs.

```text
IF motion is detected
THEN turn on hallway light
```

```text
IF faucet remains active longer than the allowed time
THEN trigger alert condition
```

```text
IF door opens AND motion follows
THEN infer resident movement through the home
```

```text
IF motion is detected AND TV/appliance is active
THEN track a TV/appliance session
```

## Project structure

```text
441_IoT_Project/
├── Makefile
├── README.md
├── include/
│   ├── controller.h
│   ├── event.h
│   ├── hw_io.h
│   └── sensor.h
└── src/
    ├── controller.c
    ├── demo.c
    ├── event_queue.c
    ├── hw_io.c
    ├── main.c
    ├── sensor.c
    └── sensors/
        ├── appliance.c
        ├── door.c
        ├── faucet.c
        └── motion.c
```

## Build instructions

Run these commands from the project root directory:

```bash
make clean
make
```

## Run instructions

Run the main smart home program:

```bash
sudo ./smart_home              # hardware mode (default): reads SW0-3, drives LEDs
sudo ./smart_home --random     # randomly-generated sensor inputs (no hardware)
sudo ./smart_home --demo       # predefined scripted sequence
```

`sudo` is needed because hardware mode opens `/dev/mem` to map the FPGA
lightweight bridge.

## Suggested sequence

Use the switches to simulate a resident moving through the smart home.

```text
1. Turn on SW0 to simulate the door opening.
2. Turn on SW1 to simulate motion.
3. Turn on SW2 to simulate faucet usage.
4. Leave SW2 on long enough to trigger an alert.
5. Turn on SW3 to simulate TV/appliance usage.
6. Observe LED outputs and console summary.
```

Expected LED behavior:

```text
SW0 ON -> LED0 ON
SW1 ON -> LED1 ON and LED8 ON
SW2 ON -> LED2 ON
SW3 ON -> LED3 ON
Faucet active too long -> LED6 ON
```

## Design notes

- The program runs on the Linux/HPS side of the DE10-Standard.
- Hardware I/O is accessed through memory-mapped I/O.
- Switches are used as simulated smart home sensors.
- LEDs are used as visible actuators and status indicators.
- The controller polls sensor values and updates system state.
- Timing logic is used to detect extended activity.
- Activity summaries are printed to the console.

