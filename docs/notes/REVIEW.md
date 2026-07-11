# WebServ — Health Check (2026-07-06)

**Verdict: it's taking form. This is a real webserv, not going nuts.**

The architecture is sound and recognizable, and the hard design calls are the
right ones. What needs work is concentrated in the request/response *parsing*
layer (a few concrete bugs) and the fact that routing isn't wired yet — both
normal for this stage.

---

## What's solid ✅

- **Overall shape.** Clean separation: config (Lexer/Parser/Token) → `Server` +
  `Listener` → `Socket` → single `poll()` loop → `Client` → `Request` →
  `Response`/`Cgi`. Each layer has one job.
- **The event loop is the correct design.** One non-blocking `poll()` over a
  mixed fd set (listeners + clients + CGI pipes), dispatched by kind. The index
  loop that tolerates mid-iteration erase/append is a deliberate, correct choice.
- **Listener / virtual-host model is right.** `gatherListeners` dedups by
  host:port and links all sharing servers; `Listener` lives outside `Server` to
  avoid a reference cycle. Verified working on `webserv.conf`.
- **Non-blocking instincts are good.** `handleSend` tracks a send offset for
  partial writes; CGI is driven through the loop via a pipe fd, not blocking reads.
- **The team is already reasoning about the right hard problems** — the `WARN`/
  `FIX` comments flag exactly the classic webserv traps (pipe-buffer deadlock,
  fd leaks, unhandled `read()==-1`).

---

## Concrete bugs to fix 🐞

1. **`Request::parseRequest` header split (Request.cpp ~L56).** Inside the header
   loop, the code splits with `pos` — a leftover from the earlier CRLF-strip loop,
   which ends at `npos` — instead of the colon index `pose` it just computed. So
   `key`/`value` aren't separated. **Fix:** use `pose` (and `pose + 1`).

2. **`Response::parseCgi_output` CR check (Response.cpp L38).**
   `line.size() - 1 == '\r'` compares a *length* to the char code 13. It should
   index the last char: `line[line.size() - 1] == '\r'`. CGI header CR-stripping
   is currently broken.

3. **`readCgiPipe` ignores `read() == -1`** (Server.cpp) — on error the pipe fd is
   never closed/removed from `_poll_fds`. Already flagged in a `WARN`.

4. **CGI body write is synchronous** (`Cgi::dadaExec`) — writing a body larger
   than the pipe buffer (~64 KB) before the script reads will stall the whole
   event loop. Already flagged in a `WARN`. Needs to go through the loop too.

5. **fd leak on partial CGI setup** (`pipe_and_fork`) — if the 2nd `pipe()` or
   `fork()` fails, earlier fds leak before the throw. Already flagged in a `FIX`.

---

## Not-yet-wired (expected) 🚧

- **Routing.** `handleReq` always builds `Response(200)`, ignoring the parsed
  method/path/location. This is CP5.
- **Host-based server selection.** Needs the `listen_fd → Listener*` map (CP3/CP4)
  so an accepted client knows its candidate servers.
- **`Response::build` is static** — doesn't serve files from disk or apply the
  config's custom error pages yet.
- **Hardcoded values** — `acceptNewClient` builds the Request with fixed
  `("8080","0.0.0.0",…)`; `setupListeners` no longer hardcodes, but the client
  side still does.

## Tidy-ups 🧹

- `Client::process()` duplicates the read+parse+respond path (with a leftover
  `[DEBUG]` cerr) but the loop uses `clientRead` instead — delete one to keep a
  single source of truth.
- `DEBUG_REQUEST` is `#define`d in `main.cpp`, so `Request.cpp` never sees it —
  move it to a shared header or a `-D` compile flag.

---

## Suggested order of attack

1. Fix the two parsing bugs (#1, #2) — they silently corrupt every request/CGI response.
2. Finish CP3: the `listen_fd → Listener*` map → then CP4/CP5 routing.
3. Harden CGI: async body write (#4), handle `read()==-1` (#3), close-on-error (#5).
4. Wire `Response::build` to files + config error pages.

See the interactive map (`notes/webserv-map.html`) — click any box for the
under-the-hood detail; boxes with amber/red notes are the items above.
