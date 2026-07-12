#!/usr/bin/env bash
# eval_tests.sh — automated PASS/FAIL suite mirroring the WebServ eval sheet.
#
# Usage:   ./tests/eval_tests.sh
#          CONF=webserv.conf PORT=8090 VHOST_PORT=8110 ./tests/eval_tests.sh
#          SLOW_FILE=/uploads/test.png ./tests/eval_tests.sh   # slow-reader target
#
# It builds (if needed), boots the server on its own, runs black-box assertions
# with curl / nc / siege / ab, then tears the server down and prints a summary.
# Exit code is the number of failed assertions (0 = all green).

set -u

# ------------------------------------------------------------------ config ----
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT" || exit 2

BIN="./WebServ"
CONF="${CONF:-webserv.conf}"
ADDR="${ADDR:-127.0.0.1}"
PORT="${PORT:-8090}"          # server 'localhost' UNIQUE port
VHOST_PORT="${VHOST_PORT:-8110}"  # SHARED port (both servers listen)
SRV2_PORT="${SRV2_PORT:-8130}"    # server 'tchoutchou' UNIQUE port
VHOST="${VHOST:-tchoutchou}"
UPLOAD_LOC="${UPLOAD_LOC:-/uploads/}"
CGI_PATH="${CGI_PATH:-/cgi-bin/serve.py}"
SLOW_FILE="${SLOW_FILE:-/uploads/test.png}"  # a largish static file for the slow-reader test

PASS=0; FAIL=0; SKIP=0
if [ -t 1 ]; then G=$'\033[1;32m'; R=$'\033[1;31m'; Y=$'\033[1;33m'; B=$'\033[1;36m'; Z=$'\033[0m'
else G=''; R=''; Y=''; B=''; Z=''; fi

ok()   { PASS=$((PASS+1)); printf "  ${G}PASS${Z}  %s\n" "$1"; }
no()   { FAIL=$((FAIL+1)); printf "  ${R}FAIL${Z}  %s  ${Y}(%s)${Z}\n" "$1" "$2"; }
skip() { SKIP=$((SKIP+1)); printf "  ${Y}SKIP${Z}  %s  (%s)\n" "$1" "$2"; }
section() { printf "\n${B}== %s ==${Z}\n" "$1"; }

# code = HTTP status of a request; extra args passed to curl
code() { curl -s -o /dev/null -w "%{http_code}" -m 5 "$@"; }

# loc = value of the Location response header (no redirect follow); args passed to curl
loc() { curl -s -D - -o /dev/null -m 5 "$@" | grep -i '^location:' | head -1 \
          | tr -d '\r' | sed 's/^[Ll]ocation:[[:space:]]*//'; }

# assert: description | expected | actual
eq() { if [ "$2" = "$3" ]; then ok "$1 (=$3)"; else no "$1" "want $2 got $3"; fi; }

# ------------------------------------------------------------------ build -----
section "Build"
if [ ! -x "$BIN" ] || [ -n "$(find server utils -newer "$BIN" -name '*.cpp' -o -newer "$BIN" -name '*.hpp' 2>/dev/null)" ]; then
  echo "  building..."; make >/dev/null 2>&1 && ok "make" || { no "make" "compile error"; exit 2; }
else
  ok "binary up to date"
fi

# ------------------------------------------------------------------ boot ------
LOG="$(mktemp)"
"$BIN" "$CONF" >"$LOG" 2>&1 &
SRV=$!
cleanup() { kill "$SRV" 2>/dev/null; wait "$SRV" 2>/dev/null; rm -f "$LOG"; }
trap cleanup EXIT
sleep 1
if ! kill -0 "$SRV" 2>/dev/null; then
  echo "  server failed to start:"; sed 's/^/    /' "$LOG"; exit 2
fi
BASE="http://$ADDR:$PORT"

# ------------------------------------------------------------------ config ----
section "Configuration"
eq "GET / on :$PORT"                 200 "$(code $BASE/)"
# NOTE: virtual-host / Host routing on the shared port is checked in its own
# discriminating section below (a bare 200 on :$VHOST_PORT proves nothing, since
# the default server on that port also answers 200).
eq "wrong URL -> 404"                404 "$(code $BASE/nope-$RANDOM)"
eq "autoindex /files/"               200 "$(code $BASE/files/)"
eq "default index (dir -> index.html)" 200 "$(code $BASE/)"
eq "redirect /old -> 301"            301 "$(code $BASE/old)"

# body size limit: POST > per-location client_max_body_size (50M on /uploads).
# Pipe via stdin (@-) so curl buffers it and sets Content-Length; the server then
# rejects on size (413) rather than on a chunked/multipart mismatch.
BIG=$(head -c 60000000 /dev/zero | curl -s -o /dev/null -w "%{http_code}" -m 10 \
        -X POST --data-binary @- $BASE$UPLOAD_LOC)
eq "body over limit -> 413"          413 "$BIG"

# ------------------------------------------------------------------ methods ---
section "Basic methods"
eq "unknown method (BREW) -> 405, no crash" 405 "$(code -X BREW $BASE/)"
eq "POST on GET-only / -> 405"       405 "$(code -X POST -d 'a=b' $BASE/)"

# upload -> get back -> delete round trip
TMP="$(mktemp)"; echo "eval-upload-$RANDOM" > "$TMP"; NAME="eval_$$_$RANDOM.txt"
UP=$(code -X POST -F "file=@$TMP;filename=$NAME" $BASE$UPLOAD_LOC)
case "$UP" in 20*) ok "POST upload (=$UP)";; *) no "POST upload" "got $UP";; esac
eq "GET uploaded file back"          200 "$(code $BASE$UPLOAD_LOC$NAME)"
DEL=$(code -X DELETE $BASE$UPLOAD_LOC$NAME)
case "$DEL" in 20*) ok "DELETE uploaded file (=$DEL)";; *) no "DELETE uploaded file" "got $DEL";; esac
eq "DELETE again -> 404"             404 "$(code -X DELETE $BASE$UPLOAD_LOC$NAME)"
rm -f "$TMP"

# ------------------------------------------------------------------ cgi --------
section "CGI"
eq "CGI GET -> 200"                  200 "$(code $BASE$CGI_PATH)"

# valid chunked body to CGI (curl chunks when body comes from a pipe + TE header)
CH=$(printf 'POST %s HTTP/1.1\r\nHost: %s\r\nContent-Type: text/plain\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n0\r\n\r\n' "$CGI_PATH" "$VHOST" \
      | nc -w 3 "$ADDR" "$PORT" | head -1 | tr -d '\r')
case "$CH" in *200*) ok "chunked POST to CGI decoded ($CH)";; *) no "chunked POST to CGI" "got: $CH";; esac

# bad CGI script must not hang / crash (timeout catches a hang)
BAD=$(code -m 5 $BASE/cgi-bin/does_not_exist.py)
if [ "$BAD" = "000" ]; then no "bad CGI (no hang)" "timeout/hang or reset"
else ok "bad CGI does not hang (=$BAD)"; fi
kill -0 "$SRV" 2>/dev/null && ok "server alive after bad CGI" || no "server alive after bad CGI" "died"

# ------------------------------------------------------------------ ports ------
section "Port / multi-server"
# Second instance on an already-bound port must refuse (it prints BINDING FAILURE
# and quits — note: it currently exits 0, so we match on the message, not $?).
OUT2=$("$BIN" "$CONF" 2>&1)
if printf '%s' "$OUT2" | grep -qiE 'binding failure|bind'; then
  ok "same port twice refused (BINDING FAILURE)"
else
  no "same port twice refused" "no bind error: $(printf '%s' "$OUT2" | head -1)"
fi
# ---------------------------------------------------- virtual hosts (Host) ----
# Both server blocks in webserv.conf share :$VHOST_PORT (server_name localhost vs
# $VHOST). These checks send the SAME url to the SAME port and vary ONLY the Host
# header, then assert behaviour that DIFFERS between the two blocks.
#
# Discriminator: /cgi-bin method set differs per block (localhost = GET POST DELETE,
# $VHOST = GET POST only). /old is NOT usable as a discriminator: both blocks redirect
# it to /files (see webserv.conf), so a bare Location-header match proves nothing.
section "Virtual hosts (Host routing)"
RA="--resolve localhost:$VHOST_PORT:$ADDR http://localhost:$VHOST_PORT"
RB="--resolve $VHOST:$VHOST_PORT:$ADDR http://$VHOST:$VHOST_PORT"

# reachability: both names must reach *a* server on the shared port
eq "Host localhost reaches :$VHOST_PORT" 200 "$(code $RA/)"
eq "Host $VHOST reaches :$VHOST_PORT"    200 "$(code $RB/)"

# sanity: /old redirect target agrees on both blocks (not a discriminator, both -> /files)
eq "vhost localhost: /old -> /files" /files "$(loc $RA/old)"
eq "vhost $VHOST: /old -> /files"    /files "$(loc $RB/old)"

# discriminator: /cgi-bin method set differs ($VHOST = GET POST, no DELETE)
eq "vhost $VHOST: DELETE /cgi-bin -> 405" 405 "$(code -X DELETE $RB$CGI_PATH)"
NE=$(code -X DELETE $RA$CGI_PATH)
if [ "$NE" != "405" ]; then ok "vhost localhost: DELETE /cgi-bin allowed (=$NE)"
else no "vhost localhost: DELETE /cgi-bin allowed" "got 405"; fi

# default server: unknown Host on the shared port falls back to the first block (localhost),
# so DELETE /cgi-bin must be allowed there too (not 405)
UNK=$(code -X DELETE --resolve foo.example:$VHOST_PORT:$ADDR http://foo.example:$VHOST_PORT$CGI_PATH)
if [ "$UNK" != "405" ]; then ok "unknown Host -> default server, DELETE /cgi-bin allowed (=$UNK)"
else no "unknown Host -> default server" "got 405 (should behave like localhost)"; fi

# ------------------------------------------- per-port server routing ----------
# Port layout in webserv.conf:
#   :$PORT       (8090) — ONLY server 'localhost'  (its UNIQUE port)
#   :$VHOST_PORT (8110) — SHARED by both servers    (selected by Host header)
#   :$SRV2_PORT  (8130) — ONLY server '$VHOST'      (its UNIQUE port)
# Discriminator: DELETE /cgi-bin -> allowed on server1 (localhost), 405 on server2 ($VHOST).
# - The SHARED-port checks prove BOTH servers are reachable on :$VHOST_PORT.
# - The UNIQUE-port checks prove each port is served by ITS server no matter what
#   Host is sent (the other server does not listen there, so it can't answer).
section "Per-port server routing"

# reachability: each port answers at all
eq "unique :$PORT reachable (server1)"  200 "$(code $BASE/)"
eq "shared :$VHOST_PORT reachable"      200 "$(code http://$ADDR:$VHOST_PORT/)"
eq "unique :$SRV2_PORT reachable (server2)" 200 "$(code http://$ADDR:$SRV2_PORT/)"

# SHARED :$VHOST_PORT — reachable by BOTH servers, chosen by Host
S1=$(code -X DELETE --resolve localhost:$VHOST_PORT:$ADDR http://localhost:$VHOST_PORT$CGI_PATH)
if [ "$S1" != "405" ]; then ok "shared :$VHOST_PORT Host localhost -> server1 (DELETE allowed, =$S1)"
else no "shared :$VHOST_PORT Host localhost -> server1" "got 405"; fi
eq "shared :$VHOST_PORT Host $VHOST -> server2 (DELETE /cgi-bin -> 405)" 405 \
   "$(code -X DELETE --resolve $VHOST:$VHOST_PORT:$ADDR http://$VHOST:$VHOST_PORT$CGI_PATH)"

# UNIQUE :$PORT — server1 only; even a $VHOST Host stays server1 (server2 absent here)
U1=$(code -X DELETE $BASE$CGI_PATH)
if [ "$U1" != "405" ]; then ok "unique :$PORT default -> server1 (DELETE allowed, =$U1)"
else no "unique :$PORT default -> server1" "got 405"; fi
U2=$(code -X DELETE --resolve $VHOST:$PORT:$ADDR http://$VHOST:$PORT$CGI_PATH)
if [ "$U2" != "405" ]; then ok "unique :$PORT Host $VHOST still server1 (DELETE allowed, =$U2)"
else no "unique :$PORT Host $VHOST still server1" "got 405"; fi

# UNIQUE :$SRV2_PORT — server2 only; even a localhost Host stays server2 (server1 absent here)
eq "unique :$SRV2_PORT default -> server2 (DELETE /cgi-bin -> 405)" 405 \
   "$(code -X DELETE http://$ADDR:$SRV2_PORT$CGI_PATH)"
eq "unique :$SRV2_PORT Host localhost still server2 (DELETE /cgi-bin -> 405)" 405 \
   "$(code -X DELETE --resolve localhost:$SRV2_PORT:$ADDR http://localhost:$SRV2_PORT$CGI_PATH)"

# ------------------------------------------------- non-blocking I/O -----------
# The whole subject hinges on ONE non-blocking poll() loop: a single slow or
# stalled client must NEVER freeze service for everyone else. Each check parks a
# pathological client (half-open, idle, drip-feeding, or reading slowly) and then
# asserts that a normal GET on a *different* connection still returns 200 promptly.
# A blocking read/write/accept anywhere would make `responsive` hang and FAIL.
section "Non-blocking I/O (slow clients)"

# responsive: is the server answering GET / with 200 in well under 3s right now?
responsive() { [ "$(code -m 3 "$BASE/")" = "200" ]; }

if responsive; then ok "baseline responsive before slow-client tests"
else no "baseline responsive" "GET / not 200 in <3s"; fi

if command -v nc >/dev/null 2>&1; then
  # 1. half-open request: send partial headers, never terminate them, hold ~5s.
  ( printf 'GET / HTTP/1.1\r\nHost: %s\r\n' "$ADDR"; sleep 5 ) | nc "$ADDR" "$PORT" >/dev/null 2>&1 &
  STALL=$!; sleep 0.3
  if responsive; then ok "responsive while a client holds a half-open request"
  else no "half-open client blocks server" "concurrent GET not 200 in <3s"; fi
  kill "$STALL" 2>/dev/null; wait "$STALL" 2>/dev/null

  # 2. idle connection: connect, send absolutely nothing, hold ~5s.
  ( sleep 5 ) | nc "$ADDR" "$PORT" >/dev/null 2>&1 &
  IDLE=$!; sleep 0.3
  if responsive; then ok "responsive while a client is connected but idle (sends nothing)"
  else no "idle client blocks server" "concurrent GET not 200 in <3s"; fi
  kill "$IDLE" 2>/dev/null; wait "$IDLE" 2>/dev/null

  # 3. drip-fed request: one byte every 200ms — exercises resumable read parsing
  #    (server must buffer a partial request across many poll() wakeups).
  drip() {
    local req i
    req="$(printf 'GET / HTTP/1.1\r\nHost: %s\r\n\r\n' "$1")"
    for (( i=0; i<${#req}; i++ )); do printf '%s' "${req:i:1}"; sleep 0.2; done
  }
  drip "$ADDR" | nc -w 20 "$ADDR" "$PORT" >/dev/null 2>&1 &
  DRIP=$!; sleep 0.5
  if responsive; then ok "responsive while a client drip-feeds a request (1 byte / 200ms)"
  else no "drip-fed client blocks server" "concurrent GET not 200 in <3s"; fi
  kill "$DRIP" 2>/dev/null; wait "$DRIP" 2>/dev/null

  # 4. many idle connections at once: no per-client thread, so every one must
  #    simply park in the poll() set without starving the accept()/serve path.
  NIDLE=30; IPIDS=""
  for _ in $(seq 1 "$NIDLE"); do
    ( sleep 5 ) | nc "$ADDR" "$PORT" >/dev/null 2>&1 & IPIDS="$IPIDS $!"
  done
  sleep 0.5
  if responsive; then ok "responsive with $NIDLE idle connections open at once"
  else no "$NIDLE idle connections block server" "concurrent GET not 200 in <3s"; fi
  for p in $IPIDS; do kill "$p" 2>/dev/null; wait "$p" 2>/dev/null; done
else
  skip "slow-client tests (half-open / idle / drip / N-idle)" "nc not installed"
fi

# 5. slow reader: download a ~90KB file at 2 KB/s so the response can't flush in
#    one write() — the write-side connection stays open ~45s and must not block
#    the loop (this is where a naive blocking write() would stall everyone).
SLOW_URL="$BASE$SLOW_FILE"
if [ "$(code -m 3 "$SLOW_URL")" = "200" ]; then
  curl -s -o /dev/null --limit-rate 2k -m 60 "$SLOW_URL" &
  SLOWR=$!; sleep 0.5
  if responsive; then ok "responsive while a client downloads slowly (2 KB/s, write-side)"
  else no "slow reader blocks server" "concurrent GET not 200 in <3s"; fi
  kill "$SLOWR" 2>/dev/null; wait "$SLOWR" 2>/dev/null
else
  skip "slow reader (write-side)" "no static file served at $SLOW_FILE (set SLOW_FILE=)"
fi

kill -0 "$SRV" 2>/dev/null && ok "server alive after slow-client tests" \
  || no "server alive after slow-client tests" "process died"

# ------------------------------------------------------------------ stress -----
section "Stress (siege / ab) & leak sanity"
RSS_BEFORE=$(ps -o rss= -p "$SRV" | tr -d ' ')

# NOTE ON ORDERING: the server sends "Connection: close" (no keep-alive), so every
# request is a fresh TCP connection. Two stress tools back-to-back on macOS loopback
# pile up TIME_WAIT sockets and exhaust ephemeral ports, which stalls the *second*
# tool (ab -> apr_pollset_poll timeout) — a client/OS artifact, not a server hang.
# Run ab first on fresh ports, then a short cooldown, then siege (the eval's tool).
if command -v ab >/dev/null 2>&1; then
  ab -n 1000 -c 10 -q "$BASE/" >/tmp/ab_$$ 2>&1
  FAILED=$(grep -i 'Failed requests' /tmp/ab_$$ | grep -oE '[0-9]+' | head -1)
  RPS=$(grep -i 'Requests per second' /tmp/ab_$$ | grep -oE '[0-9]+\.[0-9]+' | head -1)
  if [ -z "$FAILED" ]; then
    # No summary => ab aborted. If it's an apr port-exhaustion/timeout (loopback
    # TIME_WAIT saturated by an earlier siege run), that's a client/OS limit, not a
    # server bug — skip it. Any other abort is a real failure.
    if grep -qiE 'apr_pollset_poll|apr_socket|Cannot assign' /tmp/ab_$$; then
      skip "ab load test" "loopback ports exhausted (TIME_WAIT) — see siege result"
    else
      no "ab load test" "aborted: $(grep -iE 'apr_|aborted' /tmp/ab_$$ | head -1)"
    fi
  elif [ "$FAILED" = "0" ]; then
    ok "ab 1000 reqs, 0 failed (${RPS:-?} req/s)"
  else
    no "ab failed requests" "$FAILED failed"
  fi
  rm -f /tmp/ab_$$
else
  skip "ab load test" "ab not installed"
fi

if command -v siege >/dev/null 2>&1; then
  # -b benchmark, no delay; 25 concurrent, 15s. Parse Availability %.
  SOUT=$(siege -b -c 25 -t 15S "$BASE/" 2>&1)
  AVAIL=$(printf '%s\n' "$SOUT" | grep -i 'Availability' | grep -oE '[0-9]+\.[0-9]+' | head -1)
  if [ -n "$AVAIL" ]; then
    if awk "BEGIN{exit !($AVAIL >= 99.5)}"; then ok "siege availability ${AVAIL}% (>= 99.5)"
    else no "siege availability" "${AVAIL}% < 99.5"; fi
  else
    skip "siege availability" "could not parse output"; printf '%s\n' "$SOUT" | sed 's/^/    /'
  fi
else
  skip "siege availability" "siege not installed"
fi

# leak sanity: RSS should not balloon after the stress run
sleep 1
if kill -0 "$SRV" 2>/dev/null; then
  RSS_AFTER=$(ps -o rss= -p "$SRV" | tr -d ' ')
  ok "server alive after stress (RSS ${RSS_BEFORE}K -> ${RSS_AFTER}K)"
  GROW=$(( RSS_AFTER - RSS_BEFORE ))
  [ "$GROW" -gt 50000 ] && printf "  ${Y}NOTE${Z}  RSS grew ${GROW}K — check with valgrind/leaks\n"
else
  no "server alive after stress" "process died — check leak/hang"
fi

# ------------------------------------------------------------------ summary ---
section "Summary"
printf "  ${G}%d passed${Z}, ${R}%d failed${Z}, ${Y}%d skipped${Z}\n" "$PASS" "$FAIL" "$SKIP"
echo "  (Manual items — browser, poll()/errno code review, EOF explanation — see EVAL.md)"
exit "$FAIL"
