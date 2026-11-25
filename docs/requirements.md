# Requirements & Verification

**System:** FreeRTOS safety ECU on STM32F407, started and updated by the [STM32 UDS bootloader](https://github.com/GamzeAkcay/stm32-uds-bootloader). Monitors rollover with the on-board LIS3DSH accelerometer, latches a safe state on faults, and offers UDS diagnostics over CAN.

Status legend: **TBD** = not yet verified. Every requirement gets a measured or tested result before it is marked done.
Verification methods: **M** = measurement (logic analyzer / timestamps), **H** = automated HIL test (Python on the tester node), **U** = unit test (host), **R** = review / inspection.

## 1. Boot and update

| ID | Requirement | Method | Result |
|---|---|---|---|
| REQ-B01 | The application runs correctly when started by the bootloader from `0x08008000` (vector table relocated, FreeRTOS scheduler starts) | H | ✅ Verified manually: 5/5 resets via bootloader (HIL test pending) |
| REQ-B02 | `0x22 F189` returns the application software version | H | TBD |
| REQ-B03 | On `0x10 02` the application hands control to the bootloader, which stays in programming mode; a new image can then be written and started over CAN without touching the board | H | TBD |
| REQ-B04 | If the application does not confirm a healthy boot in 3 consecutive starts, the bootloader stops jumping to it and waits in programming mode | H | TBD |
| REQ-B05 | The application confirms a healthy boot after running for 5 s with all tasks alive | H | TBD |

## 2. Timing

| ID | Requirement | Method | Result |
|---|---|---|---|
| REQ-T01 | The accelerometer is sampled every 10 ms, jitter ≤ 50 µs (measured at a GPIO toggled by the sampling task) | M | TBD |
| REQ-T02 | A heartbeat frame is sent on CAN every 100 ms ± 1 ms | M, H | TBD |
| REQ-T03 | After a fault is confirmed, the safe-state output is set within 5 ms | M | TBD |
| REQ-T04 | The application answers UDS requests within 50 ms (P2 server time) | H | TBD |
| REQ-T05 | After `0x10 02`, the bootloader is ready to accept a download within 500 ms | H | TBD |

## 3. Fault handling

| ID | Requirement | Method | Result |
|---|---|---|---|
| REQ-F01 | Tilt above ~45° continuously for 500 ms raises a rollover DTC and enters the safe state | U, H | TBD (verified by hand) |
| REQ-F02 | Acceleration magnitude above a shock threshold raises a shock DTC *(planned for a later stage)* | U, H | TBD |
| REQ-F03 | The safe state is latched; it is cleared only by `0x14` and only if no fault condition is still present | U, H | TBD |
| REQ-F04 | Stored DTCs can be read with `0x19` | H | TBD |
| REQ-F05 | If any monitored task misses its liveness check-in, the independent watchdog resets the MCU within 500 ms; the reset cause can be read over UDS after reboot | H | TBD |
| REQ-F06 | Test builds offer fault-injection routines (`0x31`) that simulate each fault without touching the hardware | R, H | TBD |

## 4. Robustness

| ID | Requirement | Method | Result |
|---|---|---|---|
| REQ-R01 | Every task keeps ≥ 25 % unused stack (stack high-water mark) under worst-case test load | M | TBD |
| REQ-R02 | No heap allocation after the scheduler has started | R | TBD |
| REQ-R03 | Stack overflow and malloc failure are trapped by hooks and indicated (red LED) | R, M | Done (hooks implemented) |
| REQ-R04 | Data shared between tasks is accessed only through queues, task notifications or a mutex | R | TBD |
| REQ-R05 | Application code is C++ without exceptions or RTTI; all objects are statically allocated | R | TBD |

## 5. CAN interface (500 kbit/s, standard IDs)

| ID | Direction | Content |
|---|---|---|
| 0x7E0 | tester → ECU | UDS requests (ISO-TP), same IDs as the bootloader |
| 0x7E8 | ECU → tester | UDS responses (ISO-TP) |
| 0x100 | ECU → tester | Heartbeat: state, active fault flags, uptime |

### UDS services in the application

| Service | Use |
|---|---|
| `0x10 02` | Enter programming session → hand over to the bootloader |
| `0x14` | Clear DTCs (only if the fault condition is gone) |
| `0x19` | Read DTCs |
| `0x22` | Read data: `F189` software version, `FD00` last reset cause, `FD01` boot counter |
| `0x31` | Routine control (test builds only): fault injection |

## 6. Open items (decided by measurement, not assumption)

- Noise level of the tilt angle at rest, which sets the minimum useful debounce and hysteresis.
- Where the "stay in bootloader" marker and the boot counter are stored so that they survive a reset (backup register or `.noinit` RAM).
- Watchdog timeout vs. the longest legitimate task blocking time.
