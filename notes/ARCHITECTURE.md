# WebServ — Project Architecture

A non-blocking HTTP/1.1 server (NGINX-style config) built around a single
`poll()` event loop. This is the map of how the pieces fit together.

---

## 1. Big picture — from config file to served response

```
   webserv.conf
        │
        ▼
  ┌───────────┐   raw text → tokens      ┌───────────┐   tokens → objects
  │   Lexer   │ ───────────────────────► │  Parser   │ ─────────────────────┐
  └───────────┘   std::vector<Token>     └───────────┘                      │
                                                                            ▼
                                                              std::vector<Server>
                                                              (each: sockets, locations,
                                                               error pages, body cap)
                                                                            │
                     ┌──────────────────────────────────────────────────────┘
                     ▼
             gatherListeners(servers)      ← groups servers by unique host:port
                     │  std::vector<Listener>
                     ▼
             setupListeners(listeners)     ← socket()+bind()+listen() on each
                     │  (each Listener now has a live listen fd)
                     ▼
             Server::run(listeners)        ← the event loop (see §3)
                     │
                     ▼
        ┌────────────────────────────┐
        │  poll() → loopPollFds()     │  accept clients, read requests,
        │  per-fd dispatch            │  run CGI, write responses
        └────────────────────────────┘
```

Entry point is `server/main.cpp`, which runs exactly this pipeline.

---

## 2. The Listener model (host:port ↔ servers)

The parser stores **Server → its sockets**. `gatherListeners` inverts and
deduplicates that into **socket → its servers**, so one listening fd is opened
per unique `host:port`, even when several servers share it (virtual hosts).

```
  config:                         listeners (after gatherListeners):

  server "localhost"              ┌───────────────────────────────┐
    listen 0.0.0.0:90     ─┐      │ Listener 0.0.0.0:90            │
    listen 0.0.0.0:110     ├────► │   _socket        : one fd      │
  server "tchoutchou"      │      │   linked servers : [localhost, │
    listen 0.0.0.0:90     ─┘      │                     tchoutchou]│
                                  ├───────────────────────────────┤
                                  │ Listener 0.0.0.0:110           │
                                  │   linked servers : [localhost] │
                                  └───────────────────────────────┘
```

`Listener` lives **outside** `Server` (no ownership cycle). `gather/find/print/
setupListeners` are free functions because they span *all* servers. A future
`WebServ` class is meant to own the listeners + the event loop.

⚠️ `Listener` holds `std::vector<Server*>` into `servers_vector` — finish
parsing (vector done growing) **before** building listeners, or the pointers
dangle.

---

## 3. Runtime — the event loop

```
  Server::run(listeners)
      │  push each listener's listen fd into _poll_fds (events = POLLIN)
      ▼
  while (_running):
      poll(_poll_fds)                     ← blocks until any fd is ready
      loopPollFds():                      ← walk every fd, dispatch by kind
          for each _poll_fds[i] with .revents set:

            ┌── fd in _pipe_to_client? ──────► readCgiPipe()
            │      (CGI stdout pipe)           collect output, on EOF build
            │                                  response + switch client→POLLOUT
            │
            ├── fd not a client & not a pipe?► accept() → acceptNewClient()
            │      (a listening socket)        new Client + its fd into _poll_fds
            │
            └── else (an accepted client):
                   POLLIN            ───────► clientRead()   parse request →
                                              handleReq() or handleCgi()
                   POLLOUT           ───────► clientWrite()  flush response
                   POLLHUP|POLLERR   ───────► closeClient()  tear down
```

Three fd kinds share one `_poll_fds` array; they're told apart purely by map
lookups (`_pipe_to_client`, `_clients`). `SIGINT` flips the static
`Server::_running` to stop the loop.

---

## 4. Request lifecycle for one client

```
  accept() ──► Client(fd, Request)         status = READING
      │
      ▼  POLLIN
  clientRead → handleRecv() fills _buffer_read
      │
      ▼  request complete?
  Request::parseRequest(buffer)            method, resource, headers, body
      │
      ├── resource starts with /cgi-bin/ ? ──► handleCgi()
      │        Cgi::run() forks execve, pipe stdout ──► readCgiPipe (loop)
      │        buildHttpResponse(cgi output)
      │
      └── else ─────────────────────────────► handleReq()
               Response(code).build()        (routing/method-check = CP5, TODO)
      │
      ▼  POLLOUT
  clientWrite → handleSend() drains _buffer_send ──► closeClient when done
```

---

## 5. Components at a glance

| Component            | File(s)                         | Responsibility |
|----------------------|---------------------------------|----------------|
| **Lexer**            | `config/Lexer`                  | Config file → `vector<Token>` (words/separators). |
| **Token**            | `config/Token`                  | One lexical unit + its type (DIRECTIVE, PARAMETER, brackets…). |
| **Parser**           | `config/Parser`                 | Tokens → `vector<Server>`; state machine (GLOBAL/SERVER/LOCATION). |
| **Server**           | `server/Server`                 | One `server {}` block: sockets, locations, error pages, body cap. Also *currently* hosts the event loop (`run`, `loopPollFds`, client handlers). |
| **Listener** (free)  | `server/Server`                 | One host:port + the servers behind it; `gather/find/print/setupListeners`. |
| **Socket**           | `server/Socket`                 | `socket→setsockopt→bind→listen`; owns the listen fd + bound address. |
| **Location**         | `server/Location`               | One `location {}`: root, index, autoindex, allowed methods, redirect. |
| **Client**           | `server/client/Client`          | One connection: recv/send buffers, state, its `Request`, CGI pipe/pid. |
| **Request**          | `server/Request`                | Parse request line + headers + body; detect CGI; build CGI env. |
| **Response**         | `server/Response`               | Build the final HTTP response (status line, headers, body / CGI output). |
| **Cgi**              | `server/cgi/Cgi`                | Fork + `execve` a script, pipe body in / output out (non-blocking). |
| **utils**            | `utils/`                        | Colors, error strings, small helpers. |

---

## 6. Directory layout

```
WebServer/
├── server/
│   ├── main.cpp            entry point: lex → parse → listeners → run
│   ├── Server.{hpp,cpp}    Server + Listener + event loop
│   ├── Socket.{hpp,cpp}    listening socket setup
│   ├── Location.{hpp,cpp}  per-location config
│   ├── Request.{hpp,cpp}   HTTP request parsing
│   ├── Response.{hpp,cpp}  HTTP response building
│   ├── client/Client.*     per-connection state
│   ├── cgi/Cgi.*           CGI execution
│   └── config/             Lexer, Parser, Token, configutils
├── utils/                  utils + error/color macros
├── notes/                  ARCHITECTURE.md, LISTENER_CHECKLIST.md
├── test.sh                 curl wrapper / interactive tester (see below)
├── webserv.conf            sample config
└── Makefile
```

---

## 7. Status & how to test

- **Listeners:** CP1 (gather) ✅, CP2 (bind/listen) ✅, CP3 (loop) 🚧 — see
  [LISTENER_CHECKLIST.md](LISTENER_CHECKLIST.md). Still missing:
  `listen_fd → Listener*` map, so accepted clients don't yet carry candidate
  servers → virtual-host / routing / method checks (CP4–CP5) are next.
- **Test tool:** `./test.sh` (interactive prompt, `exitt` to quit) or one-shot
  `./test.sh -p 8110 -H tchoutchou -X POST -d 'a=b' /uploads/`.
- **Ports < 1024** (like 90/110) need root; use 8090/8110 for dev.
