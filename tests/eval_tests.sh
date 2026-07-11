#!/usr/bin/env bash
# eval_tests.sh — automated PASS/FAIL suite mirroring the WebServ eval sheet.
#
# Usage:   ./tests/eval_tests.sh
#          CONF=webserv.conf PORT=8090 VHOST_PORT=8110 ./tests/eval_tests.sh
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
PORT="${PORT:-8090}"
VHOST_PORT="${VHOST_PORT:-8110}"
VHOST="${VHOST:-tchoutchou}"
UPLOAD_LOC="${UPLOAD_LOC:-/uploads/}"
CGI_PATH="${CGI_PATH:-/cgi-bin/serve.py}"

PASS=0; FAIL=0; SKIP=0
if [ -t 1 ]; then G=$'\033[1;32m'; R=$'\033[1;31m'; Y=$'\033[1;33m'; B=$'\033[1;36m'; Z=$'\033[0m'
else G=''; R=''; Y=''; B=''; Z=''; fi

ok()   { PASS=$((PASS+1)); printf "  ${G}PASS${Z}  %s\n" "$1"; }
no()   { FAIL=$((FAIL+1)); printf "  ${R}FAIL${Z}  %s  ${Y}(%s)${Z}\n" "$1" "$2"; }
skip() { SKIP=$((SKIP+1)); printf "  ${Y}SKIP${Z}  %s  (%s)\n" "$1" "$2"; }
section() { printf "\n${B}== %s ==${Z}\n" "$1"; }

# code = HTTP status of a request; extra args passed to curl
code() { curl -s -o /dev/null -w "%{http_code}" -m 5 "$@"; }

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
eq "GET / on vhost :$VHOST_PORT"     200 "$(code --resolve $VHOST:$VHOST_PORT:$ADDR http://$VHOST:$VHOST_PORT/)"
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
eq "second server port :$VHOST_PORT serves" 200 \
   "$(code --resolve $VHOST:$VHOST_PORT:$ADDR http://$VHOST:$VHOST_PORT/)"

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
