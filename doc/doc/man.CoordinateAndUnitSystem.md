.title Coordinates and Units System

Harfang uses a left-handed coordinate system with the X axis pointing right, the Y axis pointing up and the Z axis pointing away from the viewer.
For units, it uses the International System of Units (SI) or metric system:

- Distances are expressed in meters, see [Mtr], [Cm] and [Mm].
- Angles are expressed in radians, see [Rad] and [Deg].

Time is internally stored in nanoseconds as a 64 bit integer, many functions are available to convert to and from different units of time:

- **Second:** [time_from_sec]/[time_to_sec] and [time_from_sec_f]/[time_to_sec_f]
- **Millisecond (10<sup>-3</sup>s):** [time_from_ms]/[time_to_ms] and [time_from_ms_f]/[time_to_ms_f]
- **Microsecond (10<sup>-6</sup>s):** [time_from_us]/[time_to_us] and [time_from_us_f]/[time_to_us_f]
- **Nanosecond (10<sup>-9</sup>s):** [time_from_ns]

*Note:* Storing time as integer is extremely important! As values in nanoseconds get very large, very fast, floating point representations will quickly loose precision due to quantization. If you really need time as a floating point value, you should perform fixed-point arithmetics on integer representation then convert the result to floating point as the very last step.

The current system time can be queried using [time_now].
