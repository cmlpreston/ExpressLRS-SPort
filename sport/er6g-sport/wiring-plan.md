> Migrated design/research notes from the transmitter prototype.
> Current source/build/upload locations are in [the fork guide](../README.md).
> Earlier C3 or three-pin buffer assignments are alternatives, not current firmware wiring.

# ER6G receiver-integrated S.Port interface — planning only

Draft, 2026-10-14. No wiring, firmware changes, receiver settings or uploads
were performed for this plan. This supersedes the old C3-specific pin proposal
only for the receiver-integrated approach.

## Research finding: normal S.Port signalling is approximately 3.3V

Independent first-hand development sources found on 2026-10-14 support a
3.3V S.Port signal independent of receiver/BEC supply. This changes the planning
assessment: a level converter is not established as necessary, and direct
ER6G single-pin connection is the preferred candidate. It is still not a formal
maximum-voltage guarantee for the exact ADV/GPS/cell-monitor installation.

Evidence:

- [ROBOTmaker S.Port interface](https://www.robotmaker.eu/home/quadcopters-360-degree-proximity-sensing-project-overview/frsky-s-port-interface)
  explicitly describes 3.3V inverted single-wire signalling at 57600 baud.
  Its capture project names both X8R and X4R and reports a direct FTDI connection.
  This is a developer's implementation report, not a manufacturer electrical spec.
- [SportECU implementation](https://github.com/raphaelcoeffic/sport_ecu)
  reports testing on Teensy LC and directly connecting its TX pin to S.Port.
  [PJRC specifications](https://www.pjrc.com/teensy/teensyLC.html) establish that
  normal LC GPIO inputs are 3.3V-only, not 5V tolerant. Its separate 5V ECU
  interface needs protection. Successful direct S.Port operation is strong
  practical supporting evidence, not proof about every FrSky sensor reply.
- [RX8R developer measurement](https://embdev.net/topic/489894)
  reports receiver S.Port peaks a little above 3V while the project is powered
  at 5V. Treat the first-hand receiver measurement as corroboration; the thread
  also contains an unexplained/initially problematic Arduino transmit amplitude.

No reviewed source establishes that ordinary receiver S.Port signal HIGH follows
BEC voltage. Transmitter module-bay signals, sensor supply ratings, RSSI outputs,
FrSky D telemetry and F.Port specifications are not substituted for receiver
S.Port electrical evidence. The FrSky F.Port document found during research is
about a different protocol and is not used to establish S.Port voltage.

Proceed with direct ER6G as a design candidate, subject to a final waveform
check of the actual chain for sensor reply levels and release behaviour. ER4
ESP8285 UART routing remains a separate question. No component purchase,
firmware change, receiver setting change or flash was performed in this research.

## X4R identification and signal-level evidence

The user identified the existing receiver as **FrSky X4R**, and gave sensor
markings **FrSky ADV** and **FRKGPS**. ADV is a family label, not confirmation
of the exact FAS300/cell-monitor revision; FRKGPS does not establish GPS V2.
The user recalls S.Port signalling may follow BEC voltage. Treat this as a
hypothesis to check, not proof that the signal equals the power rail.

The reviewed X4R manufacturer manual specifies 4–10V receiver operating supply
and separately identifies an RSSI PWM output as 0–3.3V, but does not give a
guaranteed S.Port signal-HIGH voltage. Do not apply the RSSI specification to
S.Port. The FAS300 ADV manual specifies 4–10V **S.Port supplying voltage**, not
signal HIGH. These sources therefore do not establish direct GPIO compatibility.

The smallest next check is a scope capture of S.Port SIGNAL relative to GND on
the working X4R chain, covering both receiver polls and sensor replies. Also
record S.Port VCC with a meter. If signal HIGH follows the LiFe supply or is
5V, direct ESP32/ESP8285 GPIO wiring is ruled out and a level interface is
required. Firmware inversion does not lower electrical voltage. A trace within
3.3V-compatible limits makes direct ER6G wiring a candidate, still subject to
idle bias, transients and power-sequence review; it does not resolve ER4 routing.
No powered experiment or firmware change was performed.

References:
- [X4R manufacturer manual](https://flymod.net/download/manual_frsky_x4rsb_eng)
- [FAS300 ADV manufacturer manual](https://www.flyingtech.co.uk/wp-content/uploads/2023/11/FAS300-ADV-Manual.pdf)

## Confirmed existing installation

The user confirmed the receiver pack is **2S LiFe**, not LiPo. The telemetry
chain currently operates with a FrSky S.Port receiver; the sensors appear to
receive power from the receiver supply through its S.Port connector. Receiver
model and whether that port passes the supply unchanged remain unconfirmed.

Use that working installation as the reference. Do not assume a 5V regulator
is needed for these sensors: first verify S.Port VCC against the receiver pack
with a meter. Their existing operation establishes practical compatibility
with the current arrangement, not a guaranteed voltage specification for every
module or direct compatibility with ESP32/ESP8285 signal pins.

Next planning evidence: identify the FrSky receiver and sensor models; measure
S.Port VCC/GND and the signal HIGH/LOW/idle waveform while the existing FrSky
receiver polls normally. Distinguish the receiver's polls from sensor replies,
and observe the released bus level. This can be done without changing firmware
or moving the sensors to the experimental ELRS receiver. Do not tie the two
receivers' signal pins together: the current FrSky receiver is already the bus
master. Maintain the no-flash, planning-only constraint.

## Latest preference: direct sensor connection

The user prefers no additional interface components; the project may not be
worthwhile if external electronics are required. Evaluate direct compatibility
before selecting buffers. The buffer below is a conservative optional design,
not an established requirement of S.Port on this hardware.

A direct ER6G signal connection is a candidate only after checking that all
sensor-chain HIGH/LOW levels, idle bias, transients and unpowered behaviour are
compatible with a 3.3V ESP32 GPIO. The existing single-pin driver provides
software-controlled inversion and release; it does not provide voltage
protection. Sensor supply voltage alone does not identify signal voltage.
No guaranteed signal-level specification for the exact sensors has been found.
The proposed direct topology is a compatible free ER6G Serial2 output SIGNAL
plus common ground to the S.Port chain; receiver supply to sensor VCC is
conditional on every sensor's verified full-charge supply rating.

The ER4 still requires an ESP8285 driver/pin-routing feasibility review even if
voltage levels match. Do not assume that removing the buffer creates an ESP32
GPIO matrix or frees its occupied UART TX pin.

The user described 2S as about 6.6V. Confirm chemistry from the pack label:
6.6V is normally nominal 2S LiFe, whereas conventional 2S LiPo is 7.4V nominal
and 8.4V fully charged. Do not infer the actual pack type from the wording alone.

## Updated bench hardware: ER4

The user subsequently selected a spare **ER4**, two servos on physical outputs
1/2 for steering/throttle, and a **2S receiver pack**. Pack chemistry and servo
voltage ratings have not been specified. No hardware was connected or changed.

The following ER6G circuit/pin plan must not be applied to the ER4. The checked
hardware target identifies ER4 as **ESP8285**, with outputs `[0,1,3,9]`:
output 1 GPIO0, output 2 GPIO1/UART0 TX, output 3 GPIO3/UART0 RX, output 4 GPIO9.
It has no ESP32 Serial2 GPIO matrix, and the existing SerialSport adapter is
ESP32-only. Keeping servos on 1/2 occupies the normal UART TX pin and leaves
only two header signals; the proposed TX/RX/enable circuit needs three.
This is a hardware/driver redesign, not an environment-name substitution.

If throttle may move to physical output 4 while remaining RF channel 2,
outputs 2/3 become candidates for UART0 TX/RX. That frees a UART but still does
not provide a third enable pin on the four output headers. An ER4-integrated
solution then needs a qualified two-pin interface or verified extra GPIO access
and an ESP8285 adapter. Neither is designed or validated yet. Retain the
receiver-integrated objective while deciding whether remapping is acceptable;
do not silently reintroduce the C3 or declare ER4 support complete.

ER4 manufacturer specifies a 4.5–8.4V receiver supply. A conventional 2S LiPo
reaches 8.4V, but receiver headers pass supply voltage to servos. Direct 2S
power is conditional on both servos being rated for the full-charge voltage.
For unspecified bench servos, plan a suitable regulated 5V receiver/servo rail
with enough current for both servo peaks, and separately review sensor power.
Interface logic remains 3.3V; the 2S pack is never a logic-level rail. When an
ESC with a BEC is added later, choose one receiver supply rather than paralleling
its BEC with the receiver pack.

Sources: [ER4 specification](https://radiomasterrc.com/products/er4-2-4ghz-elrs-pwm-receiver)
and the local checked ExpressLRS hardware repository at commit
`51a697b3220f069baf3adf2c5353b710df70bbe0`, target
`radiomaster.rx_2400.er4` in `targets.json`.

## Decision

Use a non-inverting, tri-state buffer interface with separate receiver TX, RX
and active-low TX-enable pins. The existing experimental driver is single-pin;
it needs a later hardware-adapter change before it can operate this circuit.
The tested parser, polling policy and telemetry encoders can remain shared.

The user's four-channel BSS138 I2C level-shifter module is **not approved as a
plug-in S.Port interface**. Its description implies the usual MOSFET plus
pull-up topology; its exact schematic/resistor values have not been inspected.
That topology biases an undriven bus high. Our inverted S.Port UART expects
idle-low, and must release the wire for sensor replies. The module neither
inverts the waveform nor provides an explicit transmit output-enable. It may
be adaptable with bias/driver changes and measured edge timing, but that would
be a separate interface design, not simply LV=3.3V/HV=5V wiring. It is not
necessary to prove it impossible to choose a more controllable buffer circuit.

## Provisional receiver allocation

The checked ER6G hardware definition lists physical outputs in order as
GPIO `[14, 1, 3, 15, 2, 4]`. Connector number is not RF channel number.

| Physical ER6G output | GPIO | Proposed use, conditional on being free |
| --- | --- | --- |
| 1 | 14 | Existing steering, if that is its current role |
| 2 | 1 | S.Port UART1 TX through buffer input |
| 3 | 3 | S.Port UART1 RX from receive buffer |
| 4 | 15 | Existing ESC, if that is its current role |
| 5 | 2 | Keep available; avoid adding a new strap-pin bias |
| 6 | 4 | Buffer /TX_ENABLE, with external pull-up |

Outputs 2/3 would become Serial2 TX/RX. Output 6 needs a new dedicated enable
role that disables its PWM driver; the current overlay does not implement that
role. Merely writing GPIO4 while the servo driver owns it is not acceptable.
If these outputs are occupied, revise the allocation before implementation.
Primary UART0/debug must not own GPIO1/3 during operation. Boot UART activity
on GPIO1 is isolated from the sensor bus by the disabled TX buffer.

GPIO4 is not one of the classic ESP32 boot-strapping pins. This is a reason to
prefer it for enable over GPIO2/15; still verify the actual receiver revision.
No receiver solder-pad access is proposed. Use signal and ground connections
from the output headers; confirm orientation from the board/manual markings.
The manual prohibits using output 6 as a receiver **power input**. For the
interface, its positive pin should be omitted from the harness altogether.

## Proposed 3.3V interface

Use **SN74LV125AN**, TI's 14-pin through-hole LV125A, with a DIP socket. It is
not interchangeable with a generic 74HC125. At 3.3V supply the LV125A accepts
input signals up to 5.5V; that permits receive-side down translation. Its
outputs drive at its own 3.3V rail, not at the sensor supply voltage.

```text
ER6G output 2 SIGNAL --> 1A [TX buffer] 1Y --> 220 ohm --> S.Port SIGNAL
ER6G output 6 SIGNAL --> /1OE                     |
                        |                        |
                 10k to regulated 3.3V           +--> 2A [RX buffer] 2Y
                                                      |
                                                 ER6G output 3 SIGNAL

/2OE --> GND           buffer VCC --> regulated 3.3V
all grounds common    100nF across VCC/GND close to IC
```

Top-view IC connections for this exact package:

| IC pin | Connection |
| --- | --- |
| 1, /1OE | ER6G output 6 signal; 10k pull-up to interface 3.3V |
| 2, 1A | ER6G output 2 signal; provisional 47k pull-down to ground |
| 3, 1Y | 220-ohm resistor to S.Port signal |
| 4, /2OE | Ground |
| 5, 2A | S.Port signal |
| 6, 2Y | ER6G output 3 signal |
| 7 | Ground |
| 8, 11 | Unused outputs, leave open |
| 9, 12 | Unused inputs, ground |
| 10, 13 | Unused enables, interface 3.3V |
| 14 | Interface 3.3V; 100nF to pin 7 |

Provide a position for a **47k S.Port signal pull-down**, but decide whether
and how to populate it after measuring the existing chain's bias. Both the
220-ohm series value and bias are starting values, not validated cable design.
The resistor limits transient/contention current; it is not a voltage regulator.
Receiver pin INPUT alone will not release an externally enabled buffer:
firmware must drive /1OE high after the last poll stop bit.

In the revised driver RX sees the local poll echo. Echo handling must remain
explicit: discard poll echo and seed the parser once, or parse echo once.
Enable goes high before startup, shutdown, WiFi entry and pin reconfiguration;
TX data/inversion must be established before enabling it. Test startup for a
momentary ordinary-UART idle-high glitch while output remains disabled.

### Necessary condition: sensors must accept 3.3V poll signalling

The LV125A protects the receive input within its specified range. It does
**not** prove that every FrSky sensor accepts 3.3V HIGH on S.Port. Check actual
models, bus voltage and replies to 3.3V polls before approving this design.
A 5V sensor supply does not establish a 5V data signal or a required 5V HIGH.

If the chain needs 5V transmit HIGH, a possible revised TX stage is
**SN74AHCT125N at regulated 5V**, which accepts TTL-level inputs. Keep the
LV125A receive buffer at 3.3V. The AHCT output-enable must also be controlled
without exposing an ESP32 GPIO to a 5V pull-up: use a suitable enable translator
(e.g. a transistor stage) and explicitly review reset polarity. Do not substitute
this second chip into the 3.3V diagram or simply power the LV125A at 5V: its
input-HIGH requirement then exceeds a guaranteed 3.3V ESP32 signal.
Only consider this alternative after establishing that a 5V bus drive is safe
for all connected sensors.

## Supply arrangement

Receiver header positive is the receiver/BEC supply, **not a 3.3V logic rail**.
Do not connect it to the buffer VCC or BSS138 LV terminal. For bench planning,
use regulated 5V for the identified sensor chain and a small regulated 3.3V
rail for the buffer. Select regulators and their capacitors against the actual
BEC/test-pack maximum voltage, not nominal voltage. A buck alone cannot reliably
produce 5V when its input falls to 5V or below.

```text
Verified BEC/test supply --> ER6G power input on output 1–5
                       --> suitable regulator --> sensor 5V rail
                       --> suitable regulator --> interface 3.3V rail
All grounds common; receiver positive and sensor 5V are not paralleled.
```

A verified 5V BEC may supply compatible sensors directly and feed a 3.3V
regulator. A higher-voltage BEC/test pack needs appropriate conversion.
The GPS version, FAS300 revision and unidentified 4S cell monitor still need
model/manual confirmation for their supply requirements. Keep the cell
monitor's balance input separate from its S.Port supply wiring.

Interface power must follow receiver power so the enabled RX-buffer output
cannot back-power an unpowered receiver GPIO. For initial work power down the
whole assembly together; if independent supplies are necessary, add a designed
RX isolation/enable arrangement. LV125A Ioff behaviour helps when the buffer
itself is unpowered; it does not protect a receiver powered off while the buffer
continues driving it.

## Initial parts list and planning checks

Likely parts, before final voltage/model confirmation:

- 1 x SN74LV125AN and 14-pin DIP socket.
- 1 x 100nF ceramic decoupling capacitor; regulator-specific capacitors.
- 1 x 220-ohm resistor; 1 x 10k resistor; 2 x 47k resistors/bias positions.
- Suitable regulated 3.3V supply, and 5V supply if the BEC cannot supply the sensors.
- Stripboard, signal/ground-only receiver harnesses and S.Port connectors.

Before making this a wiring instruction:

1. Record existing physical output assignments, failsafe settings, BEC voltage,
   exact receiver firmware version and actual sensor model markings.
2. Verify header orientation and supplies with a meter; identify module pull-up
   resistors if considering the BSS138 alternative.
3. With a suitable known S.Port master, measure sensor signal idle/HIGH/LOW and
   edge timing using a scope. A logic analyzer alone does not measure voltage.
4. Confirm that 3.3V polls are understood, the buffer releases before replies,
   and reset/power sequencing cannot drive the bus unexpectedly.
5. Only then implement the separate TX/RX/enable hardware adapter and test it.
   Flashing, receiver remapping and powered vehicle testing are outside this plan.

## Sources

- [ER6G manufacturer's manual](https://www.flyingtech.co.uk/wp-content/uploads/2023/11/ER6G_Receiver_User_Manual.pdf)
- [Checked ER6G GPIO layout](https://github.com/ExpressLRS/targets/blob/51a697b3220f069baf3adf2c5353b710df70bbe0/RX/Vario%202400.json)
- [LV125A voltage limits, enable operation and pinout](https://www.ti.com/lit/ds/symlink/sn74lv125a.pdf)
- [AHCT125 alternative specification](https://www.ti.com/lit/ds/symlink/sn74ahct125.pdf)
- [MOSFET/I2C level-shifter operation](https://cdn-shop.adafruit.com/datasheets/an97055.pdf)
- [S.Port inversion overview](https://github.com/iNavFlight/inav/blob/master/docs/Telemetry.md)
