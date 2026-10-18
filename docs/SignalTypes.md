# Common Real-World Signal Types

| Signal type | Typical embedded examples |
| --- | --- |
| Constant / DC | Stable temperature, pressure, battery voltage |
| Step | Switch operation, load connection, sudden setpoint change |
| Ramp | Heating/cooling, tank filling, motor acceleration |
| Sine wave | AC voltage/current, vibration, rotating machinery |
| Multi-sine / harmonic | Power-system harmonics, complex vibration |
| Square wave | Clock, GPIO, encoder output |
| Pulse / pulse train | Flow meter, proximity sensor, event counter |
| PWM | Motor control, LED control, actuator command |
| Triangle / sawtooth | Position sweep, periodic charge/discharge behavior |
| Exponential rise/fall | Capacitor charging, thermal response, pressure decay |
| Damped oscillation | Mechanical shock, control-loop settling, relay transient |
| Chirp / frequency sweep | System identification and resonance testing |
| Periodic profile | Machine cycle, breathing pattern, repetitive load |
| Random / stochastic | Ambient vibration, fluctuating load, environmental variation |
| Recorded profile / replay | CSV or logged sensor data from real equipment |
| Discrete state/event | ON/OFF, alarm, contact open/closed, operating mode |
| Digital counter/position | Rotary encoder, pulse accumulator, tachometer |
| Message-based data | CAN, Modbus, UART, SPI, I²C or Ethernet samples |

Real sensors commonly expose continuously varying voltage/current outputs or digital values transported through interfaces such as I²C, SPI, and 1-Wire.

## Common Disturbance and Fault Types

### Noise and Interference

| Disturbance | Simulation behavior |
| --- | --- |
| White/random noise | Add zero-mean random values |
| Gaussian noise | Random values following a normal distribution |
| Low-frequency noise | Slowly varying random fluctuations |
| 50/60 Hz mains hum | Add a fixed-frequency sine wave |
| Harmonic interference | Add multiples of a fundamental frequency |
| Power-supply ripple | Periodic ripple superimposed on the signal |
| EMI/RFI burst | Short high-frequency disturbance |
| Crosstalk | Inject a scaled version of another channel |
| Impulse/spike noise | Occasional sharp positive or negative spikes |
| Mechanical vibration | Multiple frequency components with changing amplitude |

Thermal effects, EMI, ADC quantization, ground loops, power-supply noise and switching equipment are frequent sources of measurement corruption.

### Sensor and Measurement Errors

| Disturbance | Simulation behavior |
| --- | --- |
| Constant bias/offset | `output = input + bias` |
| Gain error | `output = input × gain` |
| Drift | Bias changes gradually with time |
| Temperature-dependent drift | Error follows a temperature channel |
| Nonlinearity | Polynomial or lookup-table distortion |
| Hysteresis | Output depends on whether input is rising or falling |
| Deadband | Small input changes produce no output change |
| Saturation/clipping | Limit output to minimum and maximum values |
| Quantization | Round values to ADC resolution |
| Resolution loss | Dynamically reduce effective ADC bits |
| Calibration error | Combination of offset, gain and nonlinearity |
| Cross-axis sensitivity | One physical axis affects another |
| Aging | Very slow change in gain, bias or response time |

Noise is random variation, bias is a relatively fixed displacement, and drift is a slower time-dependent change. Keeping these separate in your model will make test scenarios much more realistic.

### Timing and Sampling Problems

- Sample-time jitter
- Variable latency
- Fixed transport delay
- Dropped samples
- Duplicate samples
- Out-of-order samples
- Irregular sampling rate
- Clock drift between channels
- Aliasing
- Stale value / frozen sensor
- Timestamp corruption

### Connection and Hardware Faults

- Open circuit
- Short to ground
- Short to supply
- Intermittent connection
- Stuck-at-zero
- Stuck-at-last-value
- Stuck-at-minimum/maximum
- Complete signal loss
- Brownout-related corruption
- Startup invalid period
- Sensor reset or reinitialization

### Communication Disturbances

- Packet loss
- CRC/checksum error
- Bit corruption
- Truncated frame
- Malformed payload
- Timeout
- Bus-off or disconnected node
- Repeated old packet
- Incorrect scaling or endianness
- Sequence-counter jump
- Temporary burst followed by recovery

## Recommended Minimum Viable Set

For the first practical version, I would implement:

### Waveforms

- Constant
- Step
- Ramp
- Sine
- Square/pulse
- Triangle
- Exponential
- Random signal
- Recorded-data replay

### Disturbances

- Additive Gaussian noise
- Bias
- Linear drift
- Spike/outlier
- 50/60 Hz interference
- Clipping
- Quantization
- Delay and timing jitter
- Dropout
- Frozen/stuck value
- Open/short-circuit fault
- Communication loss/corruption

## Suggested Internal Pipeline

A useful internal pipeline would be:

```text
Ideal signal
    -> physical dynamics
    -> sensor transfer function
    -> bias / gain / drift / nonlinearity
    -> electrical noise and interference
    -> ADC sampling and quantization
    -> timing and communication faults
    -> simulated application input
```

This structure will let you combine effects, for example: slow temperature ramp + sensor lag + offset drift + Gaussian noise + occasional dropout, which is much closer to what embedded firmware sees in the field.
