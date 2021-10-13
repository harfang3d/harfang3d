.title System of Units

Harfang internally uses the Internal System of Units (SI) or metric system:

- Distances are expressed in meters, see [Mtr], [Cm] and [Mm].
- Angles are expressed in radians, see [Rad] and [Deg].

Time is internally stored in nanoseconds as a 64 bit integer, many functions are available to convert to and from different units of time:

- **Second:** [time_from_sec]/[time_to_sec] and [time_from_sec_f]/[time_to_sec_f]
- **Millisecond (10<sup>-3</sup>s):** [time_from_ms]/[time_to_ms] and [time_from_ms_f]/[time_to_ms_f]
- **Microsecond (10<sup>-6</sup>s):** [time_from_us]/[time_to_us] and [time_from_us_f]/[time_to_us_f]
- **Nanosecond (10<sup>-9</sup>s):** [time_from_ns]

The current system time can be queried using [time_now].