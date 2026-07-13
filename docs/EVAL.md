# WebServ — Evaluation Sheet

Word-for-word transcription of the 42 **WebServ** evaluation sheet, as a checklist.

Status legend:
- ✅ **PASS** — verified (black-box test or code inspection)
- ⚠️ **CHECK** — works but has a caveat / needs manual confirmation at defense
- ❌ **FAIL** — known gap to fix
- 🧑 **MANUAL** — must be demonstrated live (browser / verbal explanation), not scriptable

**Last automated run:** 2026-07-13 — `tests/eval_tests.sh` → **44 passed, 0 failed, 0 skipped.**
The earlier routing gap is **fixed**: `Host`/port now select the right server block, so the
second server (`tchoutchou`) and its per-route rules apply correctly. Two CGI defects were
also fixed: a failed CGI (execve/parse error → empty stdout) now returns **500** instead of a
bogus `200`/empty body, and a peer that half-closes its write side mid-request (a legitimate
FIN, e.g. `printf ... | nc`) no longer tears the connection down before the CGI response is
sent. `ab` and `siege` both run (siege availability 100.00%, RSS stable, no leak).

---

## Mandatory Part

### Check the code and ask questions

- [ ] 🧑 Launch the installation of siege with homebrew.
- [ ] 🧑 Ask explanations about the basics of an HTTP server.
- [x] ✅ Ask what function the group used for I/O Multiplexing. *(`poll()` — the single call is `WebServ.cpp:392`, the only `poll(` in `src/`.)*
- [ ] 🧑 Ask for an explanation of how does select() (or equivalent) work.
- [ ] 🧑 Ask if they use only one select() (or equivalent) and how they've managed the server to accept and the client to read/write.
- [ ] ✅ The select() (or equivalent) should be in the main loop and should check file descriptors for read and write AT THE SAME TIME. If not, the grade is 0 and the evaluation process ends now.
- [ ] ✅ There should be only one read or one write per client per select() (or equivalent). Ask the group to show you the code from the select() (or equivalent) to the read and write of a client.
- [ ] ✅ Search for all read/recv/write/send on a socket and check that, if an error is returned, the client is removed.
- [ ] ✅ Search for all read/recv/write/send and check if the returned value is correctly checked (checking only -1 or 0 values is not enough, both should be checked).
- [ ] ✅ If errno is checked after read/recv/write/send, the grade is 0 and the evaluation process ends now.
- [ ] ✅ Writing or reading ANY file descriptor without going through the select() (or equivalent) is strictly FORBIDDEN. *(The CGI stdin pipe is registered in the poll set with POLLOUT and written only from `Client::handleSendCgi` on a poll wake-up — `writeCgiPipe()` in the loop; the stdout pipe is likewise POLLIN-driven. No fd is read/written outside poll.)*
- [ ] ✅ The project must compile without any re-link issue. If not, use the 'Invalid compilation' flag.
- [ ] 🧑 If any point is unclear or is not correct, the evaluation stops.

---

## Configuration

In the configuration file, check whether you can do the following and test the result:

- [ ] ✅ Search for the HTTP response status codes list on the internet. During this evaluation, if any status codes is wrong, don't give any related points.
- [ ] ✅ Setup multiple servers with different ports. *(Each server on its own port serves its own config — verified by `tests/eval_tests.sh` per-port routing block.)*
- [ ] ✅ Setup multiple servers with different hostnames (use something like: curl --resolve example.com:80:127.0.0.1 http://example.com/). *(Host-based routing selects the matching server block, incl. on a shared port.)*
- [ ] ✅ Setup default error page (try to change the error 404).
- [ ] ✅ Limit the client body (use: curl -X POST -H "Content-Type: plain/text" --data "BODY IS HERE write something shorter or longer than body limit").
- [ ] ✅ Setup routes in a server to different directories.
- [ ] ✅ Setup a default file to search for if you ask for a directory.
- [ ] ✅ Setup a list of methods accepted for a certain route (e.g., try to delete something with and without permission). *(Per-route method rules apply on both servers; e.g. `DELETE /cgi-bin` → 405 on `tchoutchou`, allowed on `localhost`.)*

---

## Basic checks

Using telnet, curl, prepared files, demonstrate that the following features work properly:

- [ ] ✅ GET, POST and DELETE requests should work.
- [ ] ✅ UNKNOWN requests should not result in a crash.
- [ ] ✅ For every test you should receive the appropriate status code.
- [ ] ✅ Upload some file to the server and get it back.

---

## Check CGI

Pay attention to the following:

- [ ] ✅ The server is working fine using a CGI.
- [ ] ⚠️ The CGI should be run in the correct directory for relative path file access. *(Confirm live at defense.)*
- [ ] ✅ With the help of the students you should check that everything is working properly. You have to test the CGI with the "GET" and "POST" methods.
- [ ] ✅ You need to test with files containing errors to see if the error handling works properly. You can use a script containing an infinite loop or an error; you are free to do whatever tests you want within the limits of acceptability that remain at your discretion. The group being evaluated should help you with this. *(A CGI that fails to run or crashes before emitting output now returns **500**, not a masked `200`/empty body.)*

The server should never crash and an error should be visible in case of a problem. *(Verified: broken CGI → 500, server stays alive.)*

---

## Check with a browser

*Driven live in Chrome against `:8090` on 2026-07-13. Re-run at defense to show it on the evaluator's own browser.*

- [x] ✅ Use the reference browser of the team. Open the network part of it, and try to connect to the server using it. *(Connected in Chrome; DevTools network captured every request.)*
- [x] ✅ Look at the request header and response header. *(Well-formed: `200 OK` carries `Content-Type: text/html`, `Content-Length`, `Date`, `Server: webserv/1.0`.)*
- [x] ✅ It should be compatible to serve a fully static website. *(`/` renders `index.html` fully.)*
- [x] ✅ Try a wrong URL on the server. *(`/does-not-exist` → custom `404 Not Found` page.)*
- [x] ✅ Try to list a directory. *(`/files/` → autoindex "Index of /files/" with `../` and `hello.txt`.)*
- [x] ✅ Try a redirected URL. *(`/old` → `301` with `Location: /files`; browser follows to the listing.)*
- [ ] 🧑 Try anything you want to. *(Evaluator's free-play — do live.)*

---

## Port issues

- [ ] ✅ In the configuration file setup multiple ports and use different websites. Use the browser to ensure that the configuration works as expected and shows the right website. *(Routing fixed: each port/host now serves its own server block. Confirm the visual result live in the browser at defense.)*
- [ ] ✅ In the configuration, try to setup the same port multiple times. It should not work.
- [ ] 🧑 Launch multiple servers at the same time with different configurations but with common ports. Does it work? If it does, ask why the server should work if one of the configurations isn't functional. Keep going.

---

## Siege & stress test

- [ ] ✅ Use Siege to run some stress tests.
- [ ] ✅ Availability should be above 99.5% for a simple GET on an empty page with a siege -b on that page. *(100.00%.)*
- [x] ✅ Verify there is no memory leak (Monitor the process memory usage. It should not go up indefinitely). *(`leaks <pid>` on the live process: **0 leaks / 0 total leaked bytes** both idle and after ~1100 mixed requests — heap flat at 69 KB. Also fd-leak clean: after ~3500 mixed requests the fd table is identical to baseline (only the 3 listener sockets, no stray client sockets or CGI pipes). NB: `make leaks` / `leaks --atExit` is unreliable here — on macOS it suspends the forked CGI child so a failed-`execve` child can't `_exit`, which fakes an orphan+hang. The release binary returns 500 in ~2 ms with no orphan; measure with `leaks <pid>` on the running process instead.)*
- [ ] ✅ Check if there is no hanging connection.
- [ ] ✅ You should be able to use siege indefinitely without having to restart the server (take a look at siege -b).
