# Autostop plan for ADM stepper drivers

## Goal

Protect the mechanical gear and endstops first. The motor is strong enough to
damage the mechanism, so normal operation should use reduced current/torque and
the driver should stop motion as close to the hardware as possible.

Node-RED should supervise, report, tune limits, and allow recovery. It should
not be the only layer that prevents mechanical damage.

## Current baseline

The current Node-RED baseline is:

- `StepmotorSetup/flows.json`
- Active flow: `Horizontal Vertical CLEAN`
- Vertical driver: Modbus unit id `1`
- Horizontal driver: Modbus unit id `2`
- Modbus bus: `/dev/ttyUSB0`, `115200 8N1`

Relevant registers already used by the flow:

| Register | Decimal | Current use |
| --- | ---: | --- |
| `0x000D` | 13 | Motor current, labelled `[0.01A]` in dashboard |
| `0x0004` | 4 | Position polling, 2 words |
| `0x009A` | 154 | Run speed |
| `0x00A4` | 164 | Clear error flags |
| `0x00C8` | 200 | Run/stop command |
| `0x00CA` | 202 | Jog command from joystick path |
| `0x00D0`/`0x00D1` | 208/209 | Run to absolute position |
| `0x00D2` | 210 | Set current absolute position |
| `0x00D4` | 212 | Enable/release |

## Position ratio and displayed degrees

The active Node-RED flow converts driver counter value to displayed degrees:

```text
angle_deg = 180 * raw_driver_count / ratio
```

Driver position is a signed 32-bit value, so the useful counter range is:

```text
-2147483648 .. +2147483647
```

The ratio inputs should therefore allow large values and both signs:

```text
-2147483647 .. +2147483647
```

Negative ratio is intentional. It lets us invert displayed angle if the motor or
gear counts in the opposite direction from the expected mechanical direction.

JavaScript `Number` precision is not the limiting factor here. All 32-bit driver
counter values are represented exactly. The practical issue is display
resolution: with a large ratio, a small raw movement produces a very small angle.
The flow should show extra decimals for small angles so a valid movement does
not appear as `0.0°`.

Current `0x00C8` command values from the manual:

| Value | Meaning |
| ---: | --- |
| `0` | Decelerate stop |
| `1` | Run forward |
| `256` | Emergency stop |
| `257` | Run reverse |

## Driver-side protection to test first

### 1. Reduce normal motor current and torque

This is the main protection track for the current mechanical problem.

`0x009B` is a position/DI limit register. It is useful, but it does not reduce
motor force before the endstop is hit. To avoid damaging gears and endstops, the
first tuning lever is motor current.

Use register `0x000D` to reduce the rated motor current. The flow already has
dashboard buttons:

- Horizontal currently writes `100`, interpreted by the flow as `1.00 A`.
- Vertical currently writes `250`, interpreted by the flow as `2.50 A`.

Start with deliberately low values and increase only until motion is reliable
under normal load. This makes a missed software stop less destructive.

Manual details for `0x000D`:

| Field | Value |
| --- | --- |
| Register | `0x000D` |
| Type | `UINT16` |
| Access | read/write, memory |
| Unit | `0.01 A` |
| Range | `10..650` |
| Example | `100` = `1.00 A`, `250` = `2.50 A`, `500` = `5.00 A` |

Node-RED payload through the current `Common Write Builder`:

```json
{
  "axis": "horizontal",
  "address": 13,
  "value": 80
}
```

This writes `0.80 A` to the horizontal driver.

Suggested first bench values:

| Axis | Start value | Current |
| --- | ---: | ---: |
| Horizontal | `50` | `0.50 A` |
| Vertical | `80` | `0.80 A` |

These are intentionally conservative. Increase in small steps until motion is
stable enough for normal load. Do not start by matching the motor nameplate
current.

### 1.1 Idle and minimum running current: `0x000E`

`0x000E` controls idle current, and on closed-loop drivers also minimum running
current percentage.

Manual details:

| Mode | Bits | Meaning |
| --- | --- | --- |
| Open loop | `7..0` | Idle current percentage |
| Closed loop | `15..8` | Minimum running current percentage |
| Closed loop | `7..0` | Idle current percentage |

Example from manual:

```text
0x1919 = min running current 25%, idle current 25%
0x1932 = min running current 25%, idle current 50%
```

For our purpose:

- Keep idle current low enough to avoid heating and unnecessary holding force.
- Keep minimum running current low, but not so low that motion becomes unstable.
- Treat `0x000D` as the main torque limit and `0x000E` as fine tuning.

Node-RED payload example for closed-loop 25% min running and 25% idle:

```json
{
  "axis": "vertical",
  "address": 14,
  "value": 6425
}
```

`6425` decimal is `0x1919`.

### 1.2 Current monitoring registers

These are readback/diagnostic values, not protection settings:

| Register | Meaning | Unit |
| --- | --- | --- |
| `0x001A` | Real-time running current | `mA` |
| `0x0045` | Maximum overload current since last reset | `0.01 A` |

The current diagnostic flow already reads:

- `0x001A` as `actual_current`
- `0x0045` as `max_overload_current`

Use these while tuning:

1. Reset peak registers before a test.
2. Run the axis gently.
3. Read snapshot.
4. Check actual and peak current.
5. Lower `0x000D` until the motor is weak enough to be mechanically safe but
   still usable.

### 1.3 What this driver does not clearly provide

From the manual, there does not appear to be a simple "stop when current exceeds
X" register for normal speed/position mode.

What we do have:

- `0x000D`: reduce available motor current/torque.
- `0x000E`: reduce idle/minimum current behavior.
- `0x001A`: read actual running current.
- `0x0045`: read peak overload current.
- `0x00A3`: alarm status, including overcurrent-related alarm history/status.
- `0x009E`/`0x00CB`: torque modes for closed-loop special functions such as
  collision homing, grab object, constant torque run/hold.

`0x009E` may become interesting later for controlled collision homing or a
torque-limited special motion, but it is not the first choice for ordinary
operator joystick movement. First get `0x000D`/`0x000E` tuned.

Open questions for bench test:

- Does `0x000D` apply immediately while enabled?
- Does it persist after power cycle, or does it need a save command?
- What is the lowest current that still moves each axis reliably?
- How low can `0x000E` go before holding/running becomes unstable?
- Does the driver alarm or simply current-limit when mechanically blocked?

### 2. Configure hardware position limit inputs in the driver

The manual describes hardware positive/negative limit setup at `0x009B`.

Expected behavior from the manual:

- Motion stops immediately on hitting a configured hardware limit.
- At a limit, only motion away from that limit is accepted.
- Hardware limits take priority over software limits.

This is the preferred autostop layer because it does not depend on Node-RED
timing, polling interval, dashboard state, or host software.

### 2.1 Understanding register `0x009B`

`0x009B` is one 16-bit word that configures both negative and positive hardware
limits.

Bit layout:

| Bits | Name | Meaning |
| --- | --- | --- |
| `15..13` | Set/cancel negative limit | `000` = cancel, `001` = set |
| `12` | Negative limit active level | `0` = low level active, `1` = high level active |
| `11..8` | Negative limit input port | Input number, `X0..X15` = `0..15` |
| `7..5` | Set/cancel positive limit | `000` = cancel, `001` = set |
| `4` | Positive limit active level | `0` = low level active, `1` = high level active |
| `3..0` | Positive limit input port | Input number, `X0..X15` = `0..15` |

Formula:

```text
value =
  (neg_set        << 13) |
  (neg_level      << 12) |
  (neg_port       <<  8) |
  (pos_set        <<  5) |
  (pos_level      <<  4) |
  (pos_port       <<  0)
```

Where:

- `neg_set` and `pos_set` are normally `1` to enable the limit.
- `neg_level` and `pos_level` are `0` for low-active input and `1` for
  high-active input.
- `neg_port` and `pos_port` are the physical driver input numbers.

The manual example says: NPN sensor, `X0` as negative limit, `X1` as positive
limit.

```text
negative set       = 001 << 13 = 0x2000
negative high      =   1 << 12 = 0x1000
negative port X0   =   0 <<  8 = 0x0000
positive set       = 001 <<  5 = 0x0020
positive high      =   1 <<  4 = 0x0010
positive port X1   =   1       = 0x0001
total                         = 0x3031 = 12337
```

For low-active sensors on `X0` negative and `X1` positive:

```text
0x2000 + 0x0000 + 0x0000 + 0x0020 + 0x0000 + 0x0001 = 0x2021 = 8225
```

For high-active sensors on `X1` negative and `X2` positive:

```text
0x2000 + 0x1000 + 0x0100 + 0x0020 + 0x0010 + 0x0002 = 0x3132 = 12594
```

Node-RED write payload through the current `Common Write Builder`:

```json
{
  "axis": "vertical",
  "address": 155,
  "value": 12337
}
```

`155` decimal is `0x009B`.

Readback payload:

```json
{
  "fc": 3,
  "unitid": 1,
  "address": 155,
  "quantity": 1
}
```

Expected use:

1. Write the calculated value to `0x009B`.
2. Read `0x009B` back and confirm the same value.
3. Move slowly toward the negative physical limit.
4. Confirm the driver stops without any Node-RED stop command.
5. Try to continue into the same limit. The driver should reject that direction.
6. Jog slowly away from the limit. This should be accepted.
7. Repeat for the positive limit.

Important: first bench-test the mapping between driver direction and physical
axis direction. Do not assume `positive` means up/right until tested. If the
first test stops on the wrong side, swap the positive/negative limit assignment
or motor direction configuration.

Bench test sequence:

1. Wire temporary limit switches/sensors to driver inputs.
2. Read current `0x009B`.
3. Configure negative and positive limits for one axis.
4. Move slowly toward each limit.
5. Confirm the driver stops without Node-RED sending a stop.
6. Confirm reverse motion is still allowed.
7. Confirm same behavior after power cycle, or document required init writes.

### 3. Consider E-stop input for hard inhibit

The manual describes E-stop setup at `0x00AD`.

Use this for a global hard stop condition if we have a suitable input. Hardware
limits should protect axis travel; E-stop should protect broader unsafe states.

## Node-RED supervision layer

Node-RED should add visibility and recovery:

1. Read driver status regularly, not just absolute position.
2. Decode and display:
   - alarm
   - running state
   - software negative/positive limit
   - input states `X0-X7`
   - enable state
3. Latch a software safety state when a limit or alarm appears.
4. Block further motion commands into the active limit direction.
5. Allow only:
   - stop
   - release/enable as needed
   - slow reverse jog away from the active limit
   - explicit reset/clear action

The best insertion point in the current flow is before `Common Write Builder`,
because all normal motor write commands pass through that node.

## Stop policy

Use a tiered stop policy:

| Situation | Preferred action |
| --- | --- |
| Normal operator releases joystick | `0x00C8 = 0` or jog stop, decelerate stop |
| Limit input becomes active | Driver hardware limit stops motion |
| Node-RED detects unsafe motion toward active limit | Block command and send `0x00C8 = 256` |
| Global unsafe condition / E-stop | Driver E-stop input, plus `0x00C8 = 256` as software backup |
| Recovery from limit | Permit only slow motion away from the active limit |

## Tuning values to expose in dashboard

Add these to the Node-RED dashboard or diagnostic snapshot:

| Register | Purpose |
| --- | --- |
| `0x000D` | Motor current |
| `0x0097` | Stop speed |
| `0x0098` | Acceleration time |
| `0x0099` | Deceleration time |
| `0x009A` | Run speed |
| `0x009B` | Hardware limit setup |
| `0x00AD` | E-stop setup |
| `0x00D4` | Enable/release |

For protecting hardware, prioritize lower current and a fast stop. Keep speed
low until limit behavior is proven repeatable.

## Recommended implementation order

1. Commit `StepmotorSetup/flows.json` as the Node-RED baseline.
2. Add dashboard controls/readback for current, stop/decel, hardware limit setup,
   and status inputs.
3. Bench-test reduced current on both axes.
4. Bench-test driver hardware limits on one axis at low speed/current.
5. Add a Node-RED safety gate before `Common Write Builder`.
6. Add a recovery mode that only permits slow movement away from the active
   limit.
7. Document final wiring and tested register values per axis.

## Notes

The old MPLABX project and `src/` firmware are archive material for this phase.
They should not be used as the active implementation path unless explicitly
reactivated later.
