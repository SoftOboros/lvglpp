# 00 — Transport + Executor

Chapter status: **draft, ratified 2026-04-27**.
Phase code: **PLAYIT-07**.

## §0 Authority

- `Transport` byte-level interface: `rlvgl/playit/src/transport.rs`
  (v0.2.0 @ 79f730d). Canonical.
- Line accumulation + dispatch loop: `rlvgl/playit/src/executor.rs`
  poll loop (canonical for the per-frame contract).
- Wire format: PLAYIT-01 (parse) + PLAYIT-04b (format).

## §1 Purpose

Turn the in-memory Dispatcher into something a probe can talk to.
Two pieces:

- `Transport` — byte-level read/write abstraction. Backends: stdio
  (host), TCP, UART, USART, SDMMC-pretend, etc.
- `Executor` — accumulates incoming bytes into newline-terminated
  lines, dispatches each line via `parse_command` →
  `Dispatcher::dispatch` → `format_response`, writes the result
  back through the transport.

## §3 Canonical glossary

- **`Transport`** — Abstract base.
  ```
  class Transport {
   public:
    virtual ~Transport() = default;
    [[nodiscard]] virtual std::optional<std::uint8_t> read_byte() noexcept = 0;
    virtual void write_bytes(std::span<const std::uint8_t>) noexcept = 0;
  };
  ```
  `read_byte()` returns `nullopt` when no data is available right
  now (non-blocking semantics; mirrors rlvgl's `Option<u8>`).
- **`StdioTransport`** — Host-only concrete `Transport` reading
  from `stdin`, writing to `stdout`. Sets `stdin` to non-blocking
  in the constructor and restores the prior flags in the
  destructor. `#error`s under `LVGLPP_EMBEDDED_POSTURE`.
- **`Executor`** — Line-accumulator + dispatch loop.
  - Holds `Transport& transport`, `Dispatcher& dispatcher`, a
    fixed-size line buffer, and a fixed-size response buffer.
  - `poll()` drains all currently-available bytes; for each
    complete line (terminated by `\n`, ignoring `\r`), it parses,
    dispatches, formats, and writes the response. Returns the
    number of commands processed this call.

## §4 Source-of-truth map

| Concept | Owner | Mirror sites |
| --- | --- | --- |
| `Transport` shape | rlvgl `transport.rs` (canonical) | `lvglpp::playit::Transport`. |
| Line accumulation rules (`\n` / `\r\n` / size cap) | this chapter §5.2 — **Standards Action** | parity with rlvgl executor's accumulator. |
| Stdio transport non-blocking semantics | this chapter §5.3 | host-side. |

## §5 Frozen decisions

### §5.1 `Transport` interface — **Standards Action**

Two pure-virtual methods. Adding a method requires a chapter
amendment + matching rlvgl change.

- `read_byte()` — non-blocking single-byte read. `nullopt` ≡ "no
  data right now". Implementations MUST NOT block.
- `write_bytes(span)` — best-effort write of the full span.
  Implementations MAY block briefly (e.g. flushing a TTY) but
  MUST NOT block indefinitely on a slow consumer.

### §5.2 Line accumulation — **Standards Action**

- Accumulator is a fixed-capacity buffer
  (`LVGLPP_PLAYIT_LINE_BUF_BYTES`, default `256`).
- Bytes are appended until a `'\n'` arrives. A trailing `'\r'`
  immediately before `'\n'` is stripped (CRLF tolerance).
- Lines that would exceed the capacity are silently truncated;
  the overflow tail up to the next `'\n'` is discarded.
  (Mirrors rlvgl's reasonable-line-length assumption.)
- Empty lines after CRLF stripping are dispatched as
  `parse_command("")` → `nullopt`; the executor responds with
  `Response::Error{"empty command"}` so a probe always sees a
  reply.

### §5.3 `StdioTransport` non-blocking — **Specification Required**

- Constructor sets `STDIN_FILENO` to non-blocking via
  `fcntl(O_NONBLOCK)` and stashes the prior flags.
- Destructor restores the prior flags.
- `read_byte()` returns `nullopt` on `EAGAIN` / `EWOULDBLOCK` /
  zero-byte read.
- `write_bytes()` writes via `::write(STDOUT_FILENO, …)` in a
  loop; partial writes are retried.

### §5.4 `Executor` per-poll behavior — **Standards Action**

`Executor::poll()` MUST:

1. Read up to `LVGLPP_PLAYIT_POLL_MAX_BYTES` (default `4096`)
   bytes per call.
2. For each complete line: parse, dispatch, format the Response,
   write back through the transport.
3. Return the number of complete lines processed.

It MUST NOT:

- Block on the transport.
- Allocate.
- Throw.

## §10 Reconciliation vs. adjacent primitives

- **`PlayitExecutor` (rlvgl).** rlvgl's executor owns transport +
  dispatcher + recorder + dump-state in one struct. lvglpp splits
  these: `Dispatcher` (PLAYIT-04) is independent of transport;
  `Executor` is a thin pump. This composes better with embedded
  targets that already have an event loop (caller drives `poll()`
  on each tick).
- **PLAYIT-04 Dispatcher.** Dispatcher does NOT know about bytes;
  Executor does NOT know about widgets. Clean separation.

## §11 Non-goals

- TCP / UART / USART backends. Each lands in its own follow-up
  sub-phase when a real target needs it.
- Connection lifecycle (reconnect, keepalive). The host stdio
  transport is single-shot.
- TLS / framing other than `\n`. Out of scope.
- Multi-line commands. Every command MUST fit on one line.

## §12 Acceptance checklist

- [ ] `lvglpp::playit::Transport` abstract base per §5.1.
- [ ] `lvglpp::playit::StdioTransport` host-only concrete per §5.3.
      `#error`s under `LVGLPP_EMBEDDED_POSTURE`.
- [ ] `lvglpp::playit::Executor` per §5.4. `poll()` is `noexcept`
      and allocation-free.
- [ ] Round-trip test: feed wire bytes through a
      `MemoryTransport` (test-only `Transport`), watch the
      Executor parse + dispatch + write a Response back.
      Assertions: every command produces the expected line,
      malformed input produces `ERR: empty command`, partial lines
      are accumulated across `poll()` calls.
- [ ] Compiles cleanly under `LVGLPP_EMBEDDED_POSTURE=ON` (the
      Transport base + Executor — StdioTransport excluded).
- [ ] `playit/STATUS.md` change log records PLAYIT-07 landing.

## §13 Files cited

- `rlvgl/playit/src/transport.rs`,
  `rlvgl/playit/src/executor.rs` (v0.2.0 @ 79f730d).
- `lvglpp/docs/playit-tagged/00-tagged-queries.md`,
  `lvglpp/docs/playit-tagged/01-response-formatter.md`.

## §14 Unblocks

- The `examples/host_sdl_label/` demo can now be driven by a
  rlvgl playit host: `cat fixture.txt | lvglpp_example_host_sdl_label`.
- **PLAT-NN UART transport** for embedded targets — the same
  Executor + Dispatcher pair plug into a board's UART driver.

## §15 Change log

- 2026-04-27 — Chapter ratified at draft level. Transport interface
  (§5.1), line accumulation (§5.2), StdioTransport non-blocking
  (§5.3), Executor per-poll behavior (§5.4) all frozen.
- 2026-04-27 — PLAYIT-07 execution landed.
  `playit/include/lvglpp/playit/{transport,executor,stdio_transport}.hpp`
  + `playit/src/{executor,stdio_transport}.cpp`. `stdio_transport.cpp`
  is excluded from `lvglpp_playit` under embedded posture per CMake
  conditional. Test target `lvglpp_playit_executor` (8 fixtures)
  green; embedded posture clean (Transport base + Executor
  compile freestanding-friendly).
