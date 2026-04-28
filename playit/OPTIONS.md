<!--
OPTIONS.md — Build-flag reference for the lvglpp::playit library.
-->

# lvglpp::playit Options

`lvglpp::playit` is currently an INTERFACE library and defines no
options of its own. It inherits `LVGLPP_EMBEDDED_POSTURE` from the
project-wide CMake scope; the parser MUST compile cleanly under
embedded posture (no exceptions, no dynamic allocation in the hot
parse path) because it runs on every board target.

## Planned options

| Planned option | Default | Effect |
| --- | --- | --- |
| `LVGLPP_PLAYIT_RECORDER` | `OFF` | Enable the `RS` / `RE` / `RD` event-recorder commands. Adds a small ring buffer; gated off by default to keep the embedded footprint small. |
| `LVGLPP_PLAYIT_FRAMEBUFFER_DUMP` | `OFF` | Enable the `D<x>,<y>,<w>,<h>` framebuffer-dump command. Requires the renderer to support readback. |
| `LVGLPP_PLAYIT_TAG_QUERY` | `OFF` | Enable `T@<tag>:`, `QB:`, `QE:`, `QC:` tagged-widget queries. Requires `lvglpp::core::Widget` tag support. |
