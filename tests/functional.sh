#!/usr/bin/env bash
# test.sh — curl wrapper + interactive REPL for testing the webserver.
#
# Two ways to use it:
#   1. One-shot:      ./test.sh [options] [PATH]
#   2. Interactive:   ./test.sh          (no args -> prompt; type 'exitt' to quit)
#
# In the prompt you type the SAME options as one-shot, e.g.:
#   webserv> /
#   webserv> -p 8110 -H tchoutchou /
#   webserv> -X POST -d 'hello=world' /uploads/
#   webserv> exitt        # leave
#
# Options:
#   -p PORT     port to hit           (default: 8090)
#   -a ADDR     address/IP            (default: 127.0.0.1)
#   -H HOST     value of Host: header (default: none)
#   -X METHOD   GET | POST | DELETE…  (default: GET)
#   -d DATA     request body, inline  (implies POST if -X not given)
#   -f FILE     request body, from a file
#   -b          also print the response BODY
#   -v          full verbose curl exchange
#   -L          follow redirects
#   -h          show this help

set -u

DEFAULT_ADDR="127.0.0.1"
DEFAULT_PORT="8090"

# ---- colours (skip if not a terminal) ----
if [ -t 1 ]; then
  C=$'\033[1;36m'; G=$'\033[1;32m'; Y=$'\033[1;33m'; D=$'\033[1;30m'; R=$'\033[0m'
else
  C=''; G=''; Y=''; D=''; R=''
fi

usage() {
  cat <<EOF
options:
  -p PORT     port           (default: $DEFAULT_PORT)
  -a ADDR     address/IP     (default: $DEFAULT_ADDR)
  -H HOST     Host: header   (virtual host)
  -X METHOD   GET|POST|DELETE (default: GET)
  -d DATA     body, inline   (implies POST)
  -f FILE     body, from file
  -b          print body too
  -v          verbose curl
  -L          follow redirects
examples:
  /                         -H tchoutchou /
  -p 8110 /                 -X DELETE /uploads/f.txt
  -X POST -d 'a=b' /up/     -L /old
in the prompt: 'help' shows this, 'exitt' quits.
EOF
}

# Sends one request. Takes the option/PATH args as "$@". Returns curl's exit code.
do_request() {
  local OPTIND opt
  local ADDR="$DEFAULT_ADDR" PORT="$DEFAULT_PORT" HOST="" METHOD="" DATA="" FILE=""
  local SHOW_BODY=0 VERBOSE=0 FOLLOW=0 PATH_ARG URL rc body_out
  local -a args

  while getopts ":p:a:H:X:d:f:bvLh" opt; do
    case "$opt" in
      p) PORT="$OPTARG" ;;
      a) ADDR="$OPTARG" ;;
      H) HOST="$OPTARG" ;;
      X) METHOD="$OPTARG" ;;
      d) DATA="$OPTARG" ;;
      f) FILE="$OPTARG" ;;
      b) SHOW_BODY=1 ;;
      v) VERBOSE=1 ;;
      L) FOLLOW=1 ;;
      h) usage; return 0 ;;
      \?) echo "unknown option: -$OPTARG" >&2; return 2 ;;
      :)  echo "option -$OPTARG needs an argument" >&2; return 2 ;;
    esac
  done
  shift $((OPTIND - 1))

  PATH_ARG="${1:-/}"
  [ "${PATH_ARG#/}" = "$PATH_ARG" ] && PATH_ARG="/$PATH_ARG"   # ensure leading /

  if [ -z "$METHOD" ]; then
    if [ -n "$DATA" ] || [ -n "$FILE" ]; then METHOD="POST"; else METHOD="GET"; fi
  fi

  URL="http://${ADDR}:${PORT}${PATH_ARG}"

  args=(-s -S -X "$METHOD" -m 5)
  [ -n "$HOST" ] && args+=(-H "Host: $HOST")
  [ -n "$DATA" ] && args+=(--data-binary "$DATA")
  [ -n "$FILE" ] && args+=(--data-binary "@$FILE")
  [ "$FOLLOW" -eq 1 ] && args+=(-L)
  [ "$VERBOSE" -eq 1 ] && args+=(-v)

  # summary line: what am I calling
  printf "%s→ %-6s %s%s" "$C" "$METHOD" "$URL" "$R"
  [ -n "$HOST" ] && printf "   %s(Host: %s)%s" "$Y" "$HOST" "$R"
  [ -n "$DATA" ] && printf "   %s[body: %s]%s" "$Y" "$DATA" "$R"
  [ -n "$FILE" ] && printf "   %s[body <- %s]%s" "$Y" "$FILE" "$R"
  printf "\n"

  if [ "$VERBOSE" -eq 1 ]; then
    curl "${args[@]}" "$URL"; return $?
  fi

  if [ "$SHOW_BODY" -eq 1 ]; then body_out="/dev/stdout"; else body_out="/dev/null"; fi

  printf "%s" "$G"
  curl "${args[@]}" -D - -o "$body_out" \
    -w "\n--- status %{http_code} | %{size_download} bytes | %{time_total}s | server=%{remote_ip}:%{remote_port} ---\n" \
    "$URL"
  rc=$?
  printf "%s" "$R"

  [ "$rc" -ne 0 ] && echo "!! curl exit $rc (connection refused? server not running on :$PORT?)" >&2
  return $rc
}

# Interactive prompt. Loops until 'exitt'.
repl() {
  local line
  printf "%sinteractive webserv tester%s — type requests, e.g.  %s-p 8110 -H tchoutchou /%s\n" "$C" "$R" "$D" "$R"
  printf "type %s'help'%s for options, %s'exitt'%s to quit.\n" "$Y" "$R" "$Y" "$R"
  while true; do
    if ! read -e -r -p "${G}webserv>${R} " line; then echo; break; fi   # Ctrl-D exits
    [ -z "${line//[[:space:]]/}" ] && continue                          # blank line
    history -s "$line" 2>/dev/null                                      # up-arrow recall
    case "$line" in
      exitt|exit|quit|q) printf "bye 👋\n"; break ;;
      help|h|-h|\?)      usage; continue ;;
    esac
    if ! eval "set -- $line" 2>/dev/null; then
      echo "parse error — check your quotes" >&2; continue
    fi
    do_request "$@"
  done
}

# ---- entry point ----
if [ "$#" -gt 0 ]; then
  do_request "$@"
else
  repl
fi
