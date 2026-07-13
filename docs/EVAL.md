# WebServ — Evaluation Sheet

Word-for-word transcription of the 42 **WebServ** evaluation sheet, as a checklist.

Status legend:
- ✅ **PASS** — verified (black-box test or code inspection)
- ⚠️ **CHECK** — works but has a caveat / needs manual confirmation at defense
- ❌ **FAIL** — known gap to fix
- 🧑 **MANUAL** — must be demonstrated live (browser / verbal explanation), not scriptable

**Last automated run:** 2026-07-12 — `tests/eval_tests.sh` → **38 passed, 5 failed, 1 skipped.**
All 5 failures are the **same routing gap**: requests are served from the first server
block regardless of `Host` / listening port, so the second server (`tchoutchou`) config
never applies. The 1 skip is environment-only (`ab` not installed); `siege` availability
passes at 100.00%, RSS stable (no leak).

---

## Mandatory Part

### Check the code and ask questions

- [ ] 🧑 Launch the installation of siege with homebrew.
- [ ] 🧑 Ask explanations about the basics of an HTTP server.
- [ ] 🧑 Ask what function the group used for I/O Multiplexing.
- [ ] 🧑 Ask for an explanation of how does select() (or equivalent) work.
- [ ] 🧑 Ask if they use only one select() (or equivalent) and how they've managed the server to accept and the client to read/write.
- [ ] ✅ The select() (or equivalent) should be in the main loop and should check file descriptors for read and write AT THE SAME TIME. If not, the grade is 0 and the evaluation process ends now.
- [ ] ✅ There should be only one read or one write per client per select() (or equivalent). Ask the group to show you the code from the select() (or equivalent) to the read and write of a client.
- [ ] ✅ Search for all read/recv/write/send on a socket and check that, if an error is returned, the client is removed.
- [ ] ✅ Search for all read/recv/write/send and check if the returned value is correctly checked (checking only -1 or 0 values is not enough, both should be checked).
- [ ] ✅ If errno is checked after read/recv/write/send, the grade is 0 and the evaluation process ends now.
- [ ] ⚠️ Writing or reading ANY file descriptor without going through the select() (or equivalent) is strictly FORBIDDEN. *(Caveat: the CGI request body is written to the CGI stdin pipe in `Cgi::dadaExec` without going through poll — be ready to justify or move it into the poll loop.)*
- [ ] ✅ The project must compile without any re-link issue. If not, use the 'Invalid compilation' flag.
- [ ] 🧑 If any point is unclear or is not correct, the evaluation stops.

---

## Configuration

In the configuration file, check whether you can do the following and test the result:

- [ ] ✅ Search for the HTTP response status codes list on the internet. During this evaluation, if any status codes is wrong, don't give any related points.
- [ ] ❌ Setup multiple servers with different ports. *(Second server on its own port is reachable but serves the first server's config — routing gap.)*
- [ ] ❌ Setup multiple servers with different hostnames (use something like: curl --resolve example.com:80:127.0.0.1 http://example.com/). *(Host-based routing to the second server fails.)*
- [ ] ✅ Setup default error page (try to change the error 404).
- [ ] ✅ Limit the client body (use: curl -X POST -H "Content-Type: plain/text" --data "BODY IS HERE write something shorter or longer than body limit").
- [ ] ✅ Setup routes in a server to different directories.
- [ ] ✅ Setup a default file to search for if you ask for a directory.
- [ ] ⚠️ Setup a list of methods accepted for a certain route (e.g., try to delete something with and without permission). *(Works on the first server; the second server's per-route method rules aren't applied due to the routing gap.)*

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
- [ ] ✅ You need to test with files containing errors to see if the error handling works properly. You can use a script containing an infinite loop or an error; you are free to do whatever tests you want within the limits of acceptability that remain at your discretion. The group being evaluated should help you with this.

The server should never crash and an error should be visible in case of a problem.

---

## Check with a browser

- [ ] 🧑 Use the reference browser of the team. Open the network part of it, and try to connect to the server using it.
- [ ] 🧑 Look at the request header and response header.
- [ ] 🧑 It should be compatible to serve a fully static website.
- [ ] 🧑 Try a wrong URL on the server.
- [ ] 🧑 Try to list a directory.
- [ ] 🧑 Try a redirected URL.
- [ ] 🧑 Try anything you want to.

---

## Port issues

- [ ] ❌ In the configuration file setup multiple ports and use different websites. Use the browser to ensure that the configuration works as expected and shows the right website. *(Routing gap: the second website is not shown on its port/host.)*
- [ ] ✅ In the configuration, try to setup the same port multiple times. It should not work.
- [ ] 🧑 Launch multiple servers at the same time with different configurations but with common ports. Does it work? If it does, ask why the server should work if one of the configurations isn't functional. Keep going.

---

## Siege & stress test

- [ ] ✅ Use Siege to run some stress tests.
- [ ] ✅ Availability should be above 99.5% for a simple GET on an empty page with a siege -b on that page. *(100.00%.)*
- [ ] ✅ Verify there is no memory leak (Monitor the process memory usage. It should not go up indefinitely). *(RSS 4496K → 4496K.)*
- [ ] ✅ Check if there is no hanging connection.
- [ ] ✅ You should be able to use siege indefinitely without having to restart the server (take a look at siege -b).
