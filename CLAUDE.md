# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A **C++98 sandbox** for exploring POSIX socket APIs and HTTP concepts ahead of the 42 webserv project. The full project architecture and HTTP knowledge base live in `../../library/cpp/webserv/` — read `OVERVIEW.md` there before designing anything structural.

## Build commands

```sh
make                # build both ./WebServ (server) and ./WebServ-client
make server         # build server only
make client         # build client only
make re             # clean rebuild of both
make debug          # rebuild with -g3 -O0 (readable backtraces)
make asan           # rebuild with AddressSanitizer + UBSan
make leaks          # rebuild debug, then run leaks (macOS) / valgrind (Linux)
make watch-server   # auto-rebuild + rerun server on source change (needs fswatch)
make watch-client   # auto-rebuild + rerun client on source change (needs fswatch)
make fclean         # remove objects + binaries
```

Compiler: `c++`, flags: `-Wall -Wextra -Werror -Wswitch -Wpedantic -Wshadow -Wnon-virtual-dtor -Wold-style-cast -std=c++98 -MMD -MP`. Objects land in `objs/`.

## Code conventions

- **C++98 only** — no `auto`, `nullptr`, range-`for`, lambdas, `<thread>`, `<chrono>`, brace-init, `override`, `=default`, `=delete`.
- Every class needs the four OCF members: default ctor, copy ctor, copy-assign, destructor.
- Member variables: `m_` prefix (e.g. `m_name`).
- `utils.hpp` provides: ANSI color macros (`BOLD_CYAN`, `RESET`, etc.), the `endofline` stream manipulator, key constants (`PORT 8080`, `BACK_LOG 128`, `TIMEOUT 1000`, `STD_BUFFER 4096`), and HTTP enums `e_codes` (status codes) and `e_methods` (GET, POST, …). Use these instead of raw escape codes or magic numbers.
- No comments unless the *why* is non-obvious.

## Current state

`server/main.cpp` has a working single-threaded `poll()` loop: it listens on `PORT`, calls `accept()` for new clients, reads data from existing clients via `recv()`, and removes disconnected clients — all driven by `SIGINT`-interruptible `g_running`. `Socket` wraps `socket()`/`setsockopt()`/`bind()`/`listen()`. `Server` and `Location` are OCF-compliant stubs (fields defined, logic not yet wired).

`client/main.cpp` drives a `Client` that calls `connect()` and `sendMessage()` — used to exercise the server loop manually.

`webserv.conf` documents the target nginx-style config format (listen, server_name, client_max_body_size, error_page, location blocks with root/index/methods/autoindex/upload_dir/cgi/return).

`www/` is the document root; `www/errors/` holds the custom error pages referenced in the config.

## Architecture intent (target webserv design)

`main.cpp` experiments drive toward this layered design (see `../../library/cpp/webserv/OVERVIEW.md`):

| Class | Role |
|---|---|
| `ConfigParser` | Parse `.conf` file → in-memory config tree |
| `Server` | Owns the `poll()` loop, listening sockets, connection list |
| `Connection` | Per-client state machine: read → parse → dispatch → write |
| `RequestParser` | Bytes → structured `Request`; must be resumable |
| `Router` | Match request against config → select handler |
| `Response` | Build and buffer the HTTP response |
| `CgiHandler` | Fork + pipe; CGI stdout → response body |

**Core constraint**: single non-blocking `poll()` loop — never block, never call `read`/`write` without poll permission. All fds (listen sockets, client sockets, CGI pipes) register in the same `pollfd` array.
