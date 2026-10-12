<!--
README.md — Publish-facing overview for the lvglpp::playit library.
-->

# lvglpp::playit

Library: `lvglpp::playit` (CMake target: `lvglpp_playit`, alias
`lvglpp::playit`).

`lvglpp::playit` is the C++ port of [`rlvgl-playit`](../rlvgl/playit/),
the serial test driver used to exercise lvglpp / rlvgl widget trees end
to end. The wire protocol is the **canonical source of truth** — see
[`rlvgl/playit/README.md`](../rlvgl/playit/README.md) and the top-level
[`CLAUDE.md`](../CLAUDE.md) § "Runtime Command Protocol".

## Why first-class

The user-facing instruction was to treat playit as a first-class driver.
Reasons:

- The same probe driver and the same wire format already work against
  rlvgl. By mirroring the parser in C++ rather than porting it, every
  existing rlvgl playit fixture, every recorded session, and every
  CI-hardware test transfers to lvglpp targets unchanged.
- Cross-language regressions are caught early: if lvglpp's parser
  diverges from rlvgl's, playit fixtures will fail in obvious ways.
- The "rlvgl ↔ lvglpp" change ordering rule (see CLAUDE.md
  § "Cross-language change ordering") binds here: any new playit
  command lands in rlvgl's protocol doc first, then in both parsers.

## Status

Compiled parser, dispatcher, executor, and LVGL adapters. See
[`STATUS.md`](./STATUS.md).

## LVGL snapshot dumps

`LvglSnapshotReader` implements the existing `FramebufferReader` seam using
upstream `lv_snapshot_take()`. Enable `LV_USE_SNAPSHOT=1` and ARGB8888 software
drawing in the consumer's LVGL configuration (snapshot support is enabled in
the bundled host fallback). Include `lvglpp/playit/lvgl_snapshot_reader.hpp`.

```cpp
// borrows: display and LVGL runtime outlive reader; reader outlives attachment.
lvglpp::playit::LvglSnapshotReader reader{*display.borrow_raw()};
executor.set_framebuffer_reader(&reader);

// On the LVGL application thread, after an actual completed present and
// before mutating the scene for the next frame; never from a flush callback.
if (!reader.capture_presented_frame(completed_present_count)) {
    // Application handles capture failure; the previous snapshot/count remain.
}
(void)executor.poll();

// Before reader destruction:
executor.set_framebuffer_reader(nullptr);
```

Capture is synchronous and allocates through LVGL **outside** `poll()`.
The reader owns each immutable ARGB8888 snapshot through RAII and releases it
with `lv_draw_buf_destroy()`. Pixel reads honor LVGL stride and allocate nothing.
A successful capture adopts the application's completed-present count; it does
not generate a present. Repeated counts retain the existing snapshot. Failures
retain the previous pixels/count, so no stale image is advertised as a new frame.
Old and new buffers briefly coexist: provision LVGL heap for two full snapshots
plus draw-task resources. Capture only when needed by the application.

The existing executor still owns protocol and timing: `DUMP:queued`, initial
priming, one `F` per observed count change, uppercase ARGB hex rows, and `END`.
Skipped counts coalesce; row clipping, replacement requests, and detach behavior
are unchanged. No command, pixel encoding, or rlvgl pin changes.

A snapshot re-renders the **active screen and its children**, rather than reading
physical framebuffer memory. Capture before next-frame changes and serialize all
LVGL access. Separate display layers, screen transitions, physical panel rotation,
and driver post-processing are not captured. Existing compatibility-renderer
examples retain their framebuffer readers; they do not render LVGL object trees.

## License

MIT.
