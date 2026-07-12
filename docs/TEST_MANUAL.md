# WebServ — Test Manual (what exists & what's done)

A catalogue of every test asset in this repo: what it checks, how to run it, and its
**current status**. Companion docs:
- `docs/EVAL.md` — the 42 eval checklist (readiness).
- `docs/TESTS.md` — deep reference on the load tools (`ab` / `siege`).
- **This file** — the *inventory of tests and their status*.

Status legend:
- ✅ **PASS** — asserted green on the last run
- ❌ **FAIL** — asserted red on the last run
- ⚠️ **PASS w/ caveat** — green, but the behaviour behind it is wrong/fragile
- ⏭️ **SKIP** — not run (missing tool / environment)
- 🧑 **MANUAL** — not scriptable; demonstrate live
- ▫️ **NOT RUN** — suite exists but wasn't exercised on the last run

**Last run — 2026-07-12:**
- `eval_tests.sh` → **38 passed, 5 failed, 1 skipped.** (`siege` installed → availability passes; 5 failures all = server[0]-serves-everything routing gap. +7 new non-blocking / slow-client checks, all green.)
- `config_errors` → **38/40 passed, 0 mismatched, 0 crashed, 2 probes (both clean).**
- `k6` / `postman` → not run (tools absent on this machine).

---

## Test assets at a glance

| Asset | Kind | Run it with | Status |
|---|---|---|---|
| `tests/eval_tests.sh` | Automated black-box assertions (curl/nc/siege/ab) | `bash tests/eval_tests.sh` | ✅ run 2026-07-12 (38/5/1) |
| `tests/functional.sh` | Interactive/one-shot curl helper (no assertions) | `bash tests/functional.sh` | 🧑 manual tool |
| `tests/config_errors/` | Config-parser error suite → Markdown report | `bash tests/config_errors/run_error_tests.sh` | ✅ run 2026-07-12 (38/40, 2 probes clean) |
| `tests/k6/stress.js` | k6 concurrent load / stress | `k6 run tests/k6/stress.js` | ⏭️ can't run here (no `k6`) |
| `tests/postman/` | Postman collection + environment | Postman / `newman run …` | ⏭️ can't run here (no `newman`) |

---

## 1. `tests/eval_tests.sh` — automated eval suite

Boots the server on `webserv.conf`, runs black-box assertions, tears it down.
Exit code = number of failed assertions. Override via env: `CONF=`, `PORT=`,
`VHOST_PORT=`, `VHOST=`, `UPLOAD_LOC=`, `CGI_PATH=`.

### Configuration
- [x] ✅ `GET /` on :8090 → 200
- [x] ✅ wrong URL → 404
- [x] ✅ autoindex `/files/` → 200
- [x] ✅ default index (dir → `index.html`) → 200
- [x] ✅ redirect `/old` → 301
- [x] ✅ body over limit (60 MB POST) → 413

### Basic methods
- [x] ✅ unknown method `BREW` → 405, no crash
- [x] ✅ POST on GET-only `/` → 405
- [x] ✅ POST upload → 201
- [x] ✅ GET uploaded file back → 200
- [x] ✅ DELETE uploaded file → 204
- [x] ✅ DELETE again → 404

### CGI
- [x] ✅ CGI GET (`serve.py`) → 200
- [x] ✅ chunked POST to CGI decoded → 200
- [x] ⚠️ bad CGI (missing script) does not hang → returns **200** *(no crash/hang, so the
      assertion is green, but a missing script should be **404** — see EVAL.md CGI note)*
- [x] ✅ server alive after bad CGI

### Port / multi-server
- [x] ✅ same port declared twice → refused ("BINDING FAILURE")

### Virtual hosts (Host routing)
- [x] ✅ Host `localhost` reaches :8110 → 200
- [x] ✅ Host `tchoutchou` reaches :8110 → 200
- [x] ✅ vhost `localhost`: `/old` → `/files`
- [ ] ❌ **vhost `tchoutchou`: `/old` → `/new`** *(got `/files` — server_name routing broken)*
- [ ] ❌ **vhost `tchoutchou`: DELETE `/cgi-bin` → 405** *(got `200` — same root cause)*
- [x] ✅ vhost `localhost`: DELETE `/cgi-bin` allowed
- [x] ✅ unknown Host → default server (`/old` → `/files`)

> Both ❌ are the single `server_name` virtual-hosting gap: every request is answered
> from `servers_vector[0]`'s config regardless of Host. See EVAL.md → Configuration.

### Per-port server routing (one test per port)
Port layout in `webserv.conf`: `:8090` = server1 (`localhost`) unique, `:8110` = shared
by both, `:8130` = server2 (`tchoutchou`) unique. Discriminator: `/old` → `/files`
(server1) vs `/new` (server2).
- [x] ✅ unique `:8090` reachable · shared `:8110` reachable · unique `:8130` reachable
- [x] ✅ shared `:8110` Host `localhost` → server1 (`/old`→`/files`)
- [ ] ❌ **shared `:8110` Host `tchoutchou` → server2** *(got `/files`)*
- [x] ✅ unique `:8090` default → server1 (`/old`→`/files`)
- [x] ✅ unique `:8090` Host `tchoutchou` still server1 *(only server1 listens on 8090)*
- [ ] ❌ **unique `:8130` default → server2 (`/old`→`/new`)** *(got `/files`)*
- [ ] ❌ **unique `:8130` Host `localhost` still server2** *(got `/files`)*

> All three ❌ are the same routing gap, and `:8130` proves it is **broader than Host
> routing**: that port is *only* listened on by server2, yet it is served with server1's
> config. The listener→server mapping is ignored entirely — `servers_vector[0]` answers
> everything. `:8090` passes only because server1 happens to be `servers_vector[0]`.

### Non-blocking I/O (slow clients)
Each check parks one pathological client, then asserts a normal `GET /` on another
connection still returns **200 in < 3s**. If any single slow client could freeze the
`poll()` loop, `responsive()` would hang and the check would FAIL. Env: `SLOW_FILE=`
(default `/uploads/test.png`) selects the large static file for the slow-reader test.
- [x] ✅ baseline responsive before the slow-client tests
- [x] ✅ responsive while a client holds a **half-open request** (partial headers, never terminated)
- [x] ✅ responsive while a client is connected but **idle** (sends nothing)
- [x] ✅ responsive while a client **drip-feeds** a request (1 byte / 200ms — resumable read parsing)
- [x] ✅ responsive with **30 idle connections** open at once
- [x] ✅ responsive while a client **downloads slowly** (2 KB/s, exercises the write side)
- [x] ✅ server alive after all slow-client tests

> ⚠️ **These are *behavioral* tests — they prove nobody gets starved, not that the fds
> are non-blocking.** The two are independent: these checks pass whether or not
> `O_NONBLOCK` is set, because all I/O happens *after* `poll()` reports ready and
> loopback data is already buffered, so even a blocking `read`/`write` wouldn't stall.
> **Whether the flag is actually set is a code-review item that no black-box test can
> catch.** As of 2026-07-12 it *is* set — one `fcntl(fd, F_SETFL, O_NONBLOCK | FD_CLOEXEC)`
> call on the listen, client and CGI-pipe fds (`Socket.cpp:90`, `WebServ.cpp:99,126`);
> see EVAL.md → Mandatory. `nc` is required for checks 2–5; without it they SKIP.

### Stress (siege / ab) & leak sanity
- [ ] ⏭️ `ab` load test — **SKIP** (`ab` not installed on this machine)
- [x] ✅ `siege` availability **100.00%** ≥ 99.5% (`siege -b -c 25 -t 15S`)
- [x] ✅ server alive after a real siege stress (RSS flat 4376K → 4376K)

---

## 2. `tests/functional.sh` — interactive curl helper

Not an assertion suite — a manual tester. Two modes:

```sh
bash tests/functional.sh                          # interactive REPL (type 'exitt' to quit)
bash tests/functional.sh -p 8110 -H tchoutchou /  # one-shot
bash tests/functional.sh -X POST -d 'a=b' /uploads/
bash tests/functional.sh -L /old                  # follow redirects
```

Flags: `-p` port, `-a` addr, `-H` Host header, `-X` method, `-d` inline body,
`-f` body-from-file, `-b` print body, `-v` verbose, `-L` follow redirects.
Use this to eyeball headers/bodies while demonstrating the browser/manual items.

---

## 3. `tests/config_errors/` — parser error suite

`run_error_tests.sh` runs every `*.conf` case through WebServ and writes
`tests/config_errors/PARSING_ERRORS_REPORT.md` (graceful-reject vs. mishandled).
Cases cover: bad extension, unreadable file, missing/extra braces, scope violations,
directives in the wrong block, duplicate location/server_name, invalid method/CGI/host/
port/body-size/autoindex/return/error_page, truncated EOF, unknown directive, etc.

```sh
bash tests/config_errors/run_error_tests.sh   # regenerates the report
```

Status: ✅ **RUN 2026-07-12 → 38/40 passed, 0 mismatched, 0 crashed, 2 probes.**
Every error case produced the expected graceful parse error. The two OOB "crash probes"
(`37_server_truncated_eof`, `38_location_truncated_eof`) **did not crash** — both now
return `Invalid syntax for token …` with a line number, so the previously-flagged
unchecked `_tokens_vector[index+1/+2]` access is not reachable in this build. Full
verdict table in `tests/config_errors/PARSING_ERRORS_REPORT.md`.
*(Confirmed: the runner calls `make server` (the default target, builds `WebServ`) and
resolves configs relative to `tests/config_errors/` — paths are correct.)*

---

## 4. `tests/k6/stress.js` — k6 concurrent load

Concurrent stress against :8090: mixes normal GETs with oversized POSTs (55 MB, must
be 413'd, not crash/hang). Tunable via env `VUS`, `ITERATIONS`, `BASE_URL`.

```sh
./WebServ webserv.conf &          # server must be running first
k6 run tests/k6/stress.js
VUS=50 ITERATIONS=5000 k6 run tests/k6/stress.js
```

Status: ⏭️ **CAN'T RUN HERE** — `k6` is not installed on this machine (checked
2026-07-12). This is the real load test to substitute for the skipped siege/ab
assertions above. See `docs/TESTS.md` for the ab/siege deep dive.

---

## 5. `tests/postman/` — Postman collection

`webserv.postman_collection.json` + `webserv.postman_environment.json`. Import into
Postman, or run headless with newman:

```sh
newman run tests/postman/webserv.postman_collection.json \
  -e tests/postman/webserv.postman_environment.json
```

Status: ⏭️ **CAN'T RUN HERE** — `newman` is not installed (checked 2026-07-12); `node`
is available, so `npx newman run …` would work with network access.

---

## Manual-only items (🧑 — not covered by any script)

These live in EVAL.md and must be demonstrated at the defense:
- Single `poll()` / one-poll-for-read+write / no `errno` after IO — **code review**.
- I/O multiplexing + request→response flow — **verbal explanation**.
- CGI correct working directory; EOF handling on the CGI pipe — **explain live**.
- Browser: load site, inspect Network headers, wrong URL → error page, autoindex
  renders, redirect follows.
- No leaks under valgrind/`leaks`; no zombies after CGI — **run at the eval box**.

---

## What to fix so the suite goes fully green

1. **`server_name` Host routing** — clears the 2 ❌ vhost assertions (the only real
   code failures in `eval_tests.sh`).
2. ~~Install `siege` to verify availability ≥ 99.5%~~ — **done** (100.00%). Optional:
   install `ab` for the ApacheBench cross-check, or run `k6` for heavier load.
3. **Missing CGI script → 404** (currently 200) — upgrades the ⚠️ CGI item.
