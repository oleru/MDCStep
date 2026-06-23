# MDCStep

Repository for the current stepper motor driver work for Sealight/Torka.

The active work is now centered around Node-RED control of ADM/AdamPower RS485
stepper motor drivers, with focus on:

- reduced motor current/torque for mechanical protection
- software position limits calculated from angles and gear ratio
- driver status/readback diagnostics
- future autostop and recovery handling

## Current active files

| Path | Purpose |
| --- | --- |
| `RaspberryPi/NodeRED/StepmotorSetup/flows.json` | Current Node-RED flow used for motor testing and driver setup |
| `RaspberryPi/NodeRED/StepmotorSetup/UserManual.pdf` | Main ADM/AdamPower communication/register manual |
| `RaspberryPi/NodeRED/StepmotorSetup/ADM57S series RS485 stepper motor controller V1.0.pdf` | ADM57S driver manual |
| `docs/autostop-plan.md` | Notes and plan for autostop, current limiting, hardware limits, and recovery |
| `docs/software-position-limits.md` | Design note for software min/max position limits from angle inputs |

## Project structure

| Path | Purpose |
| --- | --- |
| `Hardware/` | Hardware notes, mechanical/electrical context, and PCB material |
| `Hardware/PCB/` | PCB design files, fabrication exports, schematics, and board notes |
| `Firmware/backplane/` | Future active firmware for the backplane/controller side |
| `Firmware/old/MPLABX/` | Archived MPLABX/Harmony firmware project and source |
| `RaspberryPi/NodeRED/` | Raspberry Pi and Node-RED project files |
| `docs/` | Cross-project design notes and task documentation |

## Node-RED baseline

The active Node-RED tab is:

- `Horizontal Vertical CLEAN`

Current motor mapping:

| Axis | Driver unit id |
| --- | ---: |
| Vertical | `1` |
| Horizontal | `2` |

Current Modbus bus:

```text
/dev/ttyUSB0
115200 8N1
```

The `Motor CLEAN` dashboard contains the current operating UI, including:

- horizontal and vertical ratio settings
- min/max software position limits in degrees
- calculate/write buttons for software limits
- motor current setup
- driver status and diagnostics

## Position and ratio model

Displayed angle is calculated from the driver counter:

```text
angle_deg = 180 * raw_count / ratio
```

Software limits are calculated in the opposite direction:

```text
raw_count = round(angle_deg * ratio / 180)
```

Negative ratio values are supported. This lets us invert displayed direction if
the motor or gear counts opposite of the desired mechanical direction.

See `docs/software-position-limits.md` for details.

## Important driver registers

| Register | Purpose |
| --- | --- |
| `0x000D` | Motor rated current, unit `0.01 A` |
| `0x000E` | Idle current / minimum running current percentage |
| `0x001A` | Real-time motor current, mA |
| `0x0045` | Maximum overload current, unit `0.01 A` |
| `0x006E~0x006F` | Software negative position limit, `INT32` |
| `0x0070~0x0071` | Software positive position limit, `INT32` |
| `0x009B` | Hardware positive/negative limit input setup |
| `0x00C8` | Run/stop command |
| `0x00CA` | Jog command |
| `0x00D2~0x00D3` | Set current motor position |
| `0x00D4` | Enable/release/restart |

## Legacy archive

The original MPLABX/Harmony firmware project is kept in the repository for
reference only:

| Path | Status |
| --- | --- |
| `Firmware/old/MPLABX/MDCStep.X/` | Legacy MPLABX project, archived |
| `Firmware/old/MPLABX/src/` | Legacy firmware source, archived |
| `RaspberryPi/NodeRED/old-flows.json` | Older Node-RED flow/reference |

Do not use the MPLABX project as the active implementation path unless the
project explicitly decides to reactivate it.

## Workflow

Use GitHub as the tracking point for tested checkpoints.

Recommended loop:

1. Change `RaspberryPi/NodeRED/StepmotorSetup/flows.json` or notes in `docs/`.
2. Test in Node-RED.
3. Commit a clear checkpoint when the test is accepted.
4. Push `main` to GitHub.

Recent checkpoints:

- `Status Q` - ratio sign/range handling and improved small-angle display
- `Add software position limit task` - persistent min/max angle inputs and
  write support for software limits
