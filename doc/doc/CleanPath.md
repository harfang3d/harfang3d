Cleanup a local filesystem path according to the host platform conventions.

- Remove redundant folder separators.
- Remove redundant `.` and `..` folder entries.
- Ensure forward slash (`/`) folder separators on Unix and back slash (`\`) folder separators on Windows.