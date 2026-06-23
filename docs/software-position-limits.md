# Software position limits from angles

## Purpose

We want to let the operator enter minimum and maximum allowed position as
degrees, while Node-RED converts those angles to the driver counter values used
by the motor driver.

This is separate from hardware DI limits.

## Registers

Use the software position limit registers:

| Register | Type | Meaning |
| --- | --- | --- |
| `0x006E~0x006F` | `INT32` | Software negative limit |
| `0x0070~0x0071` | `INT32` | Software positive limit |
| `0x0006~0x0007` | `UINT32` | Status register, includes software limit flags |
| `0x00D2~0x00D3` | `INT32` | Set current motor position |

Manual behavior:

- Negative software limit: motor cannot run to an absolute position smaller
  than this value.
- Positive software limit: motor cannot run to an absolute position larger than
  this value.
- Status register bit `13` indicates software negative limit.
- Status register bit `14` indicates software positive limit.

Important: if hardware limits are configured with `0x009B`, the manual says
software limits are invalid. Therefore, this feature should be tested with
hardware limits disabled, or we must clearly document which protection layer is
active.

## Existing ratio model

The active Node-RED flow displays position as degrees using:

```text
angle_deg = 180 * raw_count / ratio
```

Therefore, to write a limit from degrees:

```text
raw_count = round(angle_deg * ratio / 180)
```

The ratio may be negative. Negative ratio is useful if the gear or motor counts
opposite of the expected direction.

## Handling min/max angle

Do not assume that `minAngle` always becomes the negative driver limit. If ratio
is negative, the signs flip.

Correct approach:

```js
const rawA = Math.round(minAngleDeg * ratio / 180);
const rawB = Math.round(maxAngleDeg * ratio / 180);

const softwareNegativeLimit = Math.min(rawA, rawB);
const softwarePositiveLimit = Math.max(rawA, rawB);
```

This works for both positive and negative ratio.

## Examples

With ratio `100000`:

| Angle | Raw count |
| ---: | ---: |
| `-90°` | `-50000` |
| `0°` | `0` |
| `+90°` | `50000` |

With ratio `-100000`:

| Angle | Raw count |
| ---: | ---: |
| `-90°` | `50000` |
| `0°` | `0` |
| `+90°` | `-50000` |

For requested limits `minAngle=-30°`, `maxAngle=60°`, `ratio=-100000`:

```text
rawA = round(-30 * -100000 / 180) = 16667
rawB = round( 60 * -100000 / 180) = -33333

softwareNegativeLimit = -33333
softwarePositiveLimit = 16667
```

## Word order for driver writes

The driver writes 32-bit values as two Modbus registers.

Based on the manual examples and current flow parsing, the word order is:

```text
[lowWord, highWord]
```

Example from the manual:

```text
-10000 = 0xFFFFD8F0
sent as D8 F0 FF FF
words = [0xD8F0, 0xFFFF]
```

Node-RED helper:

```js
function int32ToWords(value) {
    const v = value | 0;
    return [
        v & 0xFFFF,
        (v >> 16) & 0xFFFF
    ];
}
```

Write negative limit:

```json
{
  "axis": "vertical",
  "address": 110,
  "value": [lowWord, highWord],
  "quantity": 2
}
```

`110` decimal is `0x006E`.

Write positive limit:

```json
{
  "axis": "vertical",
  "address": 112,
  "value": [lowWord, highWord],
  "quantity": 2
}
```

`112` decimal is `0x0070`.

## Recommended Node-RED workflow

1. Add UI inputs per axis:
   - minimum angle in degrees
   - maximum angle in degrees
   - calculate only
   - write limits
   - read back limits
2. On calculate:
   - read active ratio for the axis
   - calculate `rawA` and `rawB`
   - sort to software negative/positive limit
   - verify both are within signed 32-bit range
   - show the calculated raw values
3. On write:
   - write `0x006E~0x006F` with software negative limit
   - write `0x0070~0x0071` with software positive limit
   - read back both values
4. On test:
   - use low current
   - use low speed
   - move toward each limit
   - verify the driver stops at the software limit
   - verify status bits `13` and `14`

## Safety notes

- Start with low motor current and low speed.
- Keep hardware endstops physically available during testing.
- Test with a generous software limit range first.
- Confirm that `0x009B` hardware limits are not overriding software limits.
- Always show both angle and raw count values in the dashboard, so the group can
  see what is actually written to the driver.
