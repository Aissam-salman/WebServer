# WebServ — Evaluation Checklist

Checklist derived from the 42 **WebServ** evaluation sheet. Each item records what
the evaluator will do and the **current status** of this project.

**Bonus scope decision:** we only implement **multiple CGI**. We do **not** implement
cookies / session management.

Status legend:
- ✅ **PASS** — verified (black-box test or code inspection)
- ⚠️ **CHECK** — works but has a caveat / needs manual confirmation at defense
- ❌ **FAIL** — known gap to fix
- 🧑 **MANUAL** — must be demonstrated live (browser / verbal explanation), not scriptable

Automated coverage lives in `tests/eval_tests.sh` (run it against a running build).

**Last automated run:** 2026-07-12 — **31 passed, 5 failed, 1 skipped.** All 5 failures
are the same routing gap (see Configuration): every request is served from
`servers_vector[0]`. The one skip is environment-only (`ab` not installed); `siege`
availability passes at 100.00%.

---

## Preamble / Guidelines

- [ ] 🧑 Only what is in the git repository is graded — confirm the eval config + www
      tree are committed.
- [ ] 🧑 No crash / no undefined behaviour / no memory leak during the whole defense.
- [ ] 🧑 If anything segfaults or leaks during a mandatory test → the item fails.

---

## Mandatory — Code review & questions (defense, mostly hard-fail)

These are **not** covered by curl. They pass or fail on the code + your explanation.

- [x] ✅ **A single `poll()` (or equivalent) in the main loop.** One `poll()` call at
      `server/Server.cpp:405`.
- [x] ✅ **One poll handles both read and write.** Per-fd `.events` flips between
      `POLLIN` and `POLLOUT` (`Server.cpp:189,242,252,307`); the single poll services
      every fd each tick.
- [x] ✅ **errno is NOT checked after `read`/`recv`/`write`/`send`.** `handleRecv`
      (`Client.cpp:52`) and `handleSend` (`Client.cpp:128`) branch only on the return
      value (`n < 0` → throw, `n == 0` → close). No `errno`/`EAGAIN` peeking on socket
      IO. *(errno elsewhere is on `open`/`remove`/`pipe` — file ops, acceptable, but be
      ready to justify it.)*
- [x] ✅ **Client is removed / bounced on a socket read/write error.** `handleRecv`/
      `handleSend` throw `"500"` on `n < 0`; caught in `Server.cpp:299` →
      `responseError`; `n == 0` → `DONE` → `closeClient` (`erase` at `Server.cpp:228`).
- [x] ✅ **Non-blocking sockets.** `O_NONBLOCK` is now set on **every** fd that enters
      the poll() set — the listen fd (`src/core/Socket.cpp:90`), each accepted client fd
      (`src/core/WebServ.cpp:99`) and the CGI pipe read end (`src/core/WebServ.cpp:126`).
      Subject-compliant form: fcntl is used **only** as `F_SETFL` with `O_NONBLOCK |
      FD_CLOEXEC` OR'd into one call — no `F_GETFL`, no `F_SETFD` (both outside the
      allowed set). *Caveat to be ready for:* passing `FD_CLOEXEC` to `F_SETFL` sets
      non-blocking for real but only nominally requests close-on-exec (Linux `F_SETFL`
      ignores it); the allowed-flags rule forces this, and CGI still works because the
      pipe ends we care about are `dup2`'d explicitly.
      *Behavioral coverage:* `eval_tests.sh` → **Non-blocking I/O (slow clients)** parks
      half-open / idle / drip-fed / slow-reader clients and proves none starve the loop
      (7/7 green). Those are behavioral — the *flag itself* is a code-review item, now met.
- [ ] 🧑 Be able to explain I/O multiplexing and the basic request→response flow.
- [ ] 🧑 Re-read the subject before the eval continues (evaluator instruction).

## Mandatory — Configuration

- [x] ✅ Multiple servers on **different ports** (8090 + 8110) — all listeners are
      bound and polled (`gatherListeners` + `run` register every listener socket).
- [ ] ❌ Multiple servers with **different hostnames** (`server_name` virtual hosting).
      **Broken:** the loop runs as `servers_vector[0].run(...)`, so `this` is always
      server[0], and `handleReq` uses `this->_locations_vector` (`Server.cpp:248`). The
      accepted client is never tagged with its listener/server (see the NOTE at
      `Server.cpp:355`), and the `Host` header is never matched against a listener's
      linked servers. **Every request, on any port/Host, is served with server[0]'s
      config.** Proof: `GET /old` on the `tchoutchou` vhost returns `Location: /files`
      (server[0]'s rule) instead of `/new` (server[1]'s). **→ tag each client with its
      `Listener`, pick the linked server whose `server_name` matches `Host` (else the
      first), and pass that server's config (locations, error pages, body size, CGI)
      to `StaticHandler`.** *Confirmed by `tests/eval_tests.sh` (run 2026-07-12), both
      the "Virtual hosts" and "Per-port server routing" sections. The bug is broader
      than Host routing — even the **unique port `:8130`, which only server2
      (`tchoutchou`) listens on, is served with server1's config**: `GET /old` on
      `:8130` returns `/files` instead of `/new`. Same on the shared `:8110` with
      `Host: tchoutchou`. Port `:8090` (server1's unique port) is correct only because
      server1 is `servers_vector[0]`. Failing assertions: `shared :8110 Host tchoutchou`,
      `unique :8130 default`, `unique :8130 Host localhost`, plus the two original
      vhost checks.*
- [x] ⚠️ **Default error page** — custom pages configured (`error_page 404 …`); the
      referenced `/errors/404.html` file is absent so the server serves a generated
      default 404. That is acceptable nginx-like behaviour (a default error page *is*
      shown). Add the file if you want the custom page to actually render.
- [x] ✅ **Client body size limit** — `client_max_body_size`; POST over the limit → 413.
- [x] ✅ Routes to **different directories** (`/`, `/files`, `/uploads`, `/cgi-bin`).
- [x] ✅ **Default index file** for a directory request (`index index.html`).
- [x] ✅ **Per-route method list** — GET-only on `/` returns 405 for POST/DELETE.

## Mandatory — Basic checks (GET / POST / DELETE / unknown)

- [x] ✅ **GET** works → 200.
- [x] ✅ **POST** works (multipart upload) → 201.
- [x] ✅ **DELETE** works → 204, then 404 on the now-missing file.
- [x] ✅ **UNKNOWN method** does not crash → 405 (e.g. `BREW`).
- [x] ✅ **Upload a file and get it back** — POST to `/uploads/`, then GET the file → 200.
- [x] ✅ Wrong URL → correct status code (404).
- [x] ✅ Method not allowed → 405.

## Mandatory — CGI

- [x] ✅ Server works with **at least one CGI** (python3 `serve.py`) → 200.
- [x] ✅ **Chunked request body** to the CGI is decoded (`isChunked`/`decodeChunk`,
      `Request.cpp:31-64`); verified 200 with a hand-crafted chunked POST.
- [x] ✅ **Unchunked** (Content-Length) body works.
- [ ] 🧑 CGI runs in the **correct working directory** for relative-path file access —
      confirm at defense (open a file relatively from the CGI).
- [ ] ⚠️ **Bad CGI / wrong file / CGI error does not crash / no infinite loop** —
      exercised in the suite (missing script does not hang, server stays alive).
      **Caveat:** a request to a non-existent CGI script currently returns
      **`200 OK` with an empty body** instead of `404`. It doesn't crash/hang (so the
      eval item holds), but the status is wrong — worth fixing so a missing script
      yields 404. Also confirm no zombie process is left after CGI runs.
- [ ] 🧑 Explain **how EOF is handled** on the CGI pipe.
- [ ] ⚠️ **php-cgi** interpreter is configured but **not installed on this machine** —
      install `php-cgi` (or drop the `.php` mapping) before an eval that tests PHP.

## Mandatory — Browser

- [ ] 🧑 Load the site in the reference browser; open the Network panel; inspect
      request / response headers.
- [ ] 🧑 Wrong URL in browser → error page.
- [ ] 🧑 Directory listing (autoindex) renders.
- [ ] 🧑 Redirected URL follows correctly (`/old` → 301).

## Mandatory — Port / multi-server

- [x] ✅ **Same port declared twice** → server refuses to start ("BINDING FAILURE").
- [x] ✅ Multiple ports serving content — both 8090 and 8110 respond.
- [ ] 🧑 Two servers sharing config still work (demonstrate live).

## Mandatory — Stress test (siege / ab)

- [x] ✅ **Availability ≥ 99.5%** for a simple GET on an empty page (`siege -b`).
      2026-07-12 run measured **100.00%** (`siege -b -c 25 -t 15S`). *(`ab` is still not
      installed, so the ApacheBench cross-check is skipped, but the siege availability
      assertion — the one the eval watches — passes.)*
- [x] ⚠️ **No memory leak** — RSS stayed flat across a real siege stress on the
      2026-07-12 run (4376K → 4376K, no growth). This is still a sanity check only —
      run under valgrind/`leaks` for the real verdict at the eval.
- [x] ✅ **No hanging connections** — server stays responsive and alive after the
      stress run. *(Note: an `ab -n 1000` load also passes 0-failed standalone; in the
      suite it's skipped when a prior siege run has saturated macOS loopback TIME_WAIT
      ports — a client-side limit, not a server hang. The server sends
      `Connection: close`, so it does not keep connections open.)*

---

## Bonus

- [x] ✅ **Multiple CGI** systems supported (multiple `cgi <ext> <interp>` mappings).
- [ ] ⛔ **Cookies / session** — intentionally **not implemented** (out of scope).

---

## Summary of things to fix before the eval

1. **`server_name` virtual hosting is broken** — every request is served with
   server[0]'s config regardless of port/Host. Route by `Host` against each
   listener's linked servers (see the Configuration section for the fix).
2. ~~**`O_NONBLOCK` on all sockets** (`fcntl`)~~ — **done.** Set via one
   `F_SETFL, O_NONBLOCK | FD_CLOEXEC` call on the listen, client and CGI-pipe fds
   (`Socket.cpp:90`, `WebServ.cpp:99,126`); the old forbidden `F_SETFD` lines are gone.
3. **Missing CGI script returns `200`** instead of `404` — fix the status.
3. **Bind failure exits `0`** — the server prints "BINDING FAILURE" but should exit
   non-zero so a launch failure is detectable.
4. **`www/errors/404.html`** — add it if the custom 404 page must render (optional).
5. **Install `php-cgi`** or remove the `.php` CGI mapping (environment, not code).
5b. **Install `siege` (or `ab`)** — the stress/availability assertions are skipped and
   unverified on this machine without them (environment, not code).
6. Confirm **no leaks** under valgrind/`leaks` and **no zombies** after CGI runs.
