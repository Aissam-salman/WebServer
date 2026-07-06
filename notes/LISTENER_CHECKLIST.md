# Listener Implementation Checklist

## Concept

A `Listener` = **one socket (one host:port) + pointers to every server that
declared `listen` on that host:port.**

When a connection arrives on that socket, all those servers are *candidates*;
the actual one is picked later from the request's `Host:` header (virtual
hosting). If only one server listens there, the vector has a single entry and
there is no ambiguity.

The relationship inverts what the parser produced:

- Parser stored it as **Server → its Sockets** (a server knows its addresses).
- Listener stores it as **Socket → its Servers** (an address knows who wants it).

Same links, grouped the other way and deduplicated by host:port.

```
Listener {
    Socket               _socket;               // one host:port
    std::vector<Server*> _pointers_to_server;   // everyone listening there
}
```

---

## Decision before starting

The event loop currently lives inside `Server::run()` — a method on a *single*
server. Listeners span **all** servers, so the loop needs an owner that sees the
whole `vector<Server>`. Choose one:

- [x] **Free function** called from `main`, taking `servers_vector`.
      *(Interim, in place now: `gatherListeners`, `setupListeners`, `printListeners`,
      `findListener` are free functions — kept **outside** `Server` so servers don't
      own their listeners and there's no `Server`↔`Listener` reference cycle.)*
- [ ] **Manager class** (`WebServ`) holding the servers vector, the listeners, and
      the client map, with the loop as its method.
      *(Chosen destination — to be built. For now the loop still lives in
      `Server::run(vector<Listener>&)` called on `servers_vector[0]`; it moves into
      `WebServ` once that class exists.)*

Where the listeners + loop live decides where the CP1 function belongs.

---

## Checkpoints

### CP1 — Build `vector<Listener>` from the parsed servers  ✅ DONE
- [x] Walk every server, every socket.
- [x] For each `(host, port)`, find the existing Listener or create a new one.
- [x] Push that server's pointer into the matching Listener.
- [x] Print each unique host:port + the servers attached, to verify grouping.
- Verified against webserv.conf: 0.0.0.0:90 → [localhost, tchoutchou],
  0.0.0.0:110 → [localhost]. Dedup correct.

### CP2 — Open each listener's socket once  ✅ DONE
- [x] Call `setSocket(port)` on each Listener's socket (`bind()` + `listen()`).
      Done in the free `setupListeners(vector<Listener>&)`.
- [x] One fd per unique host:port, **not** per server (dedup happens in CP1).
- [x] Verify one "LISTENING SUCCESS" line per distinct address.
- Note: `Listener::getSocket()` returns `Socket&` (not by value) so `setSocket()`
  binds the stored socket and the fd persists.
- Gotcha found: ports < 1024 (e.g. 90, 110 in webserv.conf) fail `bind()` with
  `EACCES` unless root. Use high ports (8090/8110) for dev, or `sudo`, or
  `setcap 'cap_net_bind_service=+ep' ./WebServ`. Verified bind + listen on 8090/8110.

### CP3 — Drive the event loop from the listeners  🚧 IN PROGRESS
- [x] Replace the hardcoded `socket1` in the loop (SocketTest stub removed).
- [x] Build `poll_fds` from the listener fds (in `Server::run`).
- [ ] Keep `map<int, Listener*>` (listen_fd → listener) so an incoming
      connection knows its candidate servers. **Still TODO** — `loopPollFds`
      currently recognises a listen fd only by "not a client and not a pipe",
      so accepted connections don't yet carry their candidate servers.
- [x] Verify `curl` works on every configured port (HTTP 200 on 8090 & 8110).

### CP4 — Attach candidate servers to each accepted Client
- [ ] On `accept()`, give the new `Client` a handle to its Listener (or its
      server list).
- [ ] A client now carries its routing context.

### CP5 — Select the server via `Host:`, then route + method check
- [ ] After parsing, pick the matching server (default to the first when `Host`
      doesn't match).
- [ ] Do the location match.
- [ ] Do the method-vs-flag check (`location.getMethodFlag() & methodBit`),
      throw `"405"` when not allowed.

---

## Gotcha

`Listener` holds `std::vector<Server*>` — raw pointers into `servers_vector`.
If that vector is resized/reallocated **after** the pointers are taken, they
dangle. So: finish all parsing (vector done growing) → **then** build the
listeners → never push more servers afterward.

---

## Workflow

Implement one checkpoint, show it, get it reviewed, then move to the next.
No code handed over — guidance + frequent review.
