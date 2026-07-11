#!/usr/bin/env bash
# Runs every test_conf/*.conf case against WebServ and writes a Markdown
# report of which parsing errors are handled gracefully vs. not.
#
# IMPORTANT: utils/utils.hpp unconditionally #defines RUN 1, so any config
# that parses AND binds successfully makes WebServ enter its real poll()
# loop and never return on its own. Every invocation below runs the binary
# in the background and force-kills it after a short grace period so this
# script can never hang, regardless of whether a given test case behaves
# as expected.
set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BIN="$REPO_ROOT/WebServ"
REPORT="$SCRIPT_DIR/PARSING_ERRORS_REPORT.md"
ESC="$(printf '\033')"

echo "== Rebuilding WebServ =="
(cd "$REPO_ROOT" && make server) || { echo "Build failed"; exit 1; }

# Runs "$BIN <conf_path>" with a ~2s grace period, then force-kills it if
# still alive (this is what happens for a config that parses+binds fine,
# since it then blocks forever in the accept/poll loop).
# Sets RUN_BIN_OUTPUT and RUN_BIN_EXIT (124 = killed after timing out).
run_bin() {
    local conf_path="$1"
    local tmpout
    tmpout="$(mktemp)"

    "$BIN" "$conf_path" > "$tmpout" 2>&1 &
    local pid=$!

    local i=0
    while [ "$i" -lt 10 ]; do
        if ! kill -0 "$pid" 2>/dev/null; then
            break
        fi
        sleep 0.2
        i=$((i + 1))
    done

    if kill -0 "$pid" 2>/dev/null; then
        kill -9 "$pid" 2>/dev/null
        wait "$pid" 2>/dev/null
        RUN_BIN_EXIT=124
    else
        wait "$pid"
        RUN_BIN_EXIT=$?
    fi

    RUN_BIN_OUTPUT="$(cat "$tmpout")"
    rm -f "$tmpout"
}

# file | expected substring ("" = expect no error output, "CRASH_PROBE" = no fixed expectation) | short description
read -r -d '' DATA <<'DATAEOF' || true
01_bad_extension.txt|Invalid file name : must end with .conf|Wrong file extension (.txt instead of .conf)
99_does_not_exist.conf|Error opening the file : |Path that does not exist on disk
03_unreadable_file.conf|Error opening the file : |File exists but chmod 000 (unreadable)
04_unterminated_directive.conf|Invalid conf file, reaching the end of file without finding end of directive|Last directive in the file has no terminating ; before true EOF
04b_missing_semicolon_midfile.conf|Invalid syntax for token 'listen'|Missing ; before another directive mid-file (tokens glom together instead of a clean "unterminated" error)
05_scope_underflow.conf|Conf file went out of scope|Stray extra closing }
06_nested_server_in_server.conf|is not valid in server scope|server{} nested inside another server{}
07_nested_server_in_location.conf|is not valid in location scope|server{} nested inside a location{}
08_location_outside_server.conf|is not valid in global scope|location{} in global scope (outside any server)
09_nested_location.conf|is not valid in location scope|location{} nested inside another location{}
10_directive_outside_block.conf|found outside any block|listen; directive in global scope
11_directive_wrong_scope_server.conf|is not valid in server scope|root; directive directly inside server{}
12_directive_wrong_scope_location.conf|is not valid in location scope|server_name; directive inside location{}
13_invalid_syntax_extra_tokens.conf|Invalid syntax for token 'root'|root with an extra token before ;
14_duplicate_location.conf|A location already exists with name |Two location blocks with the same path
15_invalid_method.conf|Unknown HTTP method in methods directive|methods directive with an unknown verb
16_invalid_cgi_args.conf|Invalid syntax for token 'cgi'|cgi directive with missing interpreter arg
17_invalid_cgi_interpreter.conf|Invalid syntax for token '.py'|cgi .py mapped to a non-python3 interpreter
18_max_body_size_nonnumeric.conf|Invalid syntax for token 'abc'|client_max_body_size with non-numeric value
19_max_body_size_negative.conf|Invalid syntax for token '-10'|client_max_body_size with a negative value
20_max_body_size_bad_suffix.conf|Invalid syntax for token '10X'|client_max_body_size with an unknown unit suffix
21_max_body_size_overflow.conf|Invalid syntax for token '99999999999999999999G'|client_max_body_size value overflowing long
22_invalid_host.conf|Invalid host in listen directive|listen with an out-of-range IPv4 host
23_invalid_port_nonnumeric.conf|Invalid port in listen directive|listen with a non-numeric port
24_invalid_port_range.conf|Invalid port in listen directive|listen with a port above 65535
25_invalid_autoindex.conf|autoindex only accepts 'on' or 'off'|autoindex with a value other than on/off
26_duplicate_server_name.conf|A server already exists with name |Two server{} blocks with the same server_name
27_server_name_already_set.conf|Server name is already set|server_name directive repeated in one server{}
28_invalid_return_syntax.conf|Invalid syntax for token 'return'|return directive missing the redirect path
29_invalid_return_code.conf|Invalid error code in return directive|return directive with an out-of-range code
30_invalid_error_page_syntax.conf|Invalid syntax for token 'error_page'|error_page directive missing the path
31_invalid_error_page_code.conf|Invalid error code in error_page directive|error_page directive with a non-numeric code
32_unknown_directive.conf|Invalid directive |Completely unknown directive name
33_incomplete_file_missing_brace.conf|Incomplete conf file|server{} block missing its closing }
34_no_server_block.conf|No server has been configured|File with comments only, no server{} block
35_server_missing_brace.conf|Invalid syntax for token 'server'|"server" keyword not followed by {
36_location_missing_name.conf|Invalid syntax for token 'location'|"location" keyword without a path name
37_server_truncated_eof.conf|CRASH_PROBE|File ends with a bare "server" token (index+1 OOB read probe)
38_location_truncated_eof.conf|CRASH_PROBE|File ends with "server {\n location" (index+2 OOB read probe)
DATAEOF

total=0
pass=0
fail_mismatch=0
fail_crash=0
info=0

results=""
issues=""

run_case() {
    local file="$1" expected="$2" desc="$3"
    local path="$SCRIPT_DIR/$file"
    local restore_perms=0

    if [ "$file" = "03_unreadable_file.conf" ]; then
        chmod 000 "$path"
        restore_perms=1
    fi

    run_bin "$path"
    local exit_code="$RUN_BIN_EXIT"

    if [ "$restore_perms" = "1" ]; then
        chmod 644 "$path"
    fi

    local clean
    clean="$(printf '%s' "$RUN_BIN_OUTPUT" | sed "s/${ESC}\[[0-9;]*m//g")"

    total=$((total + 1))

    if [ "$expected" = "CRASH_PROBE" ]; then
        info=$((info + 1))
        if [ "$exit_code" -gt 128 ] && [ "$exit_code" -ne 124 ]; then
            local sig=$((exit_code - 128))
            results="${results}| ${file} | (probe) | CRASHED (signal ${sig}) | \`${clean}\` |\n"
            issues="${issues}- **${file}** — ${desc}. **Process crashed** with signal ${sig} instead of throwing a graceful parse error. Confirms the unchecked \`_tokens_vector[index+1]\`/\`[index+2]\` access in \`Parser::parseStateDirective\` (server/config/Parser.cpp) is reachable and unsafe.\n"
        elif [ "$exit_code" -eq 124 ]; then
            results="${results}| ${file} | (probe) | unexpectedly parsed OK, entered run loop (killed after timeout) | \`${clean}\` |\n"
            issues="${issues}- **${file}** — ${desc}. The truncated file unexpectedly parsed successfully and the server started binding/listening instead of raising a parse error — the probe did not test what it intended to.\n"
        else
            results="${results}| ${file} | (probe) | no crash (exit ${exit_code}) | \`${clean}\` |\n"
        fi
        return
    fi

    if [ "$exit_code" -eq 124 ]; then
        fail_mismatch=$((fail_mismatch + 1))
        results="${results}| ${file} | \`${expected}\` | FAIL (parsed OK / entered run loop, expected a parse error) | \`${clean}\` |\n"
        issues="${issues}- **${file}** — ${desc}. Expected a parse error containing \`${expected}\` but the config parsed successfully and the server started binding/listening instead.\n"
        return
    fi

    if [ "$exit_code" -gt 128 ]; then
        fail_crash=$((fail_crash + 1))
        local sig=$((exit_code - 128))
        results="${results}| ${file} | \`${expected}\` | **CRASH** (signal ${sig}) | \`${clean}\` |\n"
        issues="${issues}- **${file}** — ${desc}. Expected graceful error containing \`${expected}\` but the **process crashed** (signal ${sig}).\n"
        return
    fi

    case "$clean" in
        *"$expected"*)
            pass=$((pass + 1))
            results="${results}| ${file} | \`${expected}\` | PASS | \`${clean}\` |\n"
            ;;
        *)
            fail_mismatch=$((fail_mismatch + 1))
            results="${results}| ${file} | \`${expected}\` | FAIL | \`${clean}\` |\n"
            issues="${issues}- **${file}** — ${desc}. Expected output to contain \`${expected}\`, got: \`${clean}\`\n"
            ;;
    esac
}

echo "== Running control case (00_control_valid.conf, expect no error output) =="
run_bin "$SCRIPT_DIR/00_control_valid.conf"
control_exit="$RUN_BIN_EXIT"
control_clean="$(printf '%s' "$RUN_BIN_OUTPUT" | sed "s/${ESC}\[[0-9;]*m//g")"
total=$((total + 1))
# For a valid config, RUN=1 means WebServ binds and blocks in its poll loop
# forever -> we expect run_bin to time out (exit 124) with empty output.
if { [ "$control_exit" -eq 124 ] || [ "$control_exit" -eq 0 ]; } && [ -z "$control_clean" ]; then
    pass=$((pass + 1))
    control_line="| 00_control_valid.conf | (no output, should bind & block) | PASS | \`${control_clean}\` |"
else
    fail_mismatch=$((fail_mismatch + 1))
    control_line="| 00_control_valid.conf | (no output, should bind & block) | FAIL | \`${control_clean}\` |"
    issues="${issues}- **00_control_valid.conf (control)** — known-good config produced unexpected output or exit code ${control_exit}: \`${control_clean}\`\n"
fi

echo "== Running error cases =="
while IFS='|' read -r file expected desc; do
    [ -z "$file" ] && continue
    run_case "$file" "$expected" "$desc"
done <<< "$DATA"

{
    echo "# Parsing Error Test Report"
    echo
    echo "Generated by \`test_conf/run_error_tests.sh\` against \`$BIN\`."
    echo
    echo "## Summary"
    echo
    echo "- Total cases: $total"
    echo "- Passed (handled gracefully as expected): $pass"
    echo "- Failed — wrong/missing error message: $fail_mismatch"
    echo "- Failed — process crashed: $fail_crash"
    echo "- Crash probes (informational, not pass/fail): $info"
    echo

    if [ "$fail_mismatch" -gt 0 ] || [ "$fail_crash" -gt 0 ] || [ -n "$issues" ]; then
        echo "## Needs handling"
        echo
        printf '%b' "$issues"
        echo
    else
        echo "## Needs handling"
        echo
        echo "None — every case produced the expected error message and no crashes occurred."
        echo
    fi

    cat <<'MDEOF'
## Known issues found by source review

These were identified by reading `server/config/Parser.cpp` directly,
independent of whether the specific probe below crashed on this machine/build:

1. **Scope errors now name the real scope (FIXED)** — `Parser::parseStateDirective`
   used to always say "...is not valid in location scope" for *any* invalid
   `server`/`location` nesting, even when the actual scope was GLOBAL or SERVER. It now
   throws "Directive is not valid in <global|server|location> scope" via a `scopeName()`
   helper, so cases 06-09 each report the scope they were actually in.
2. **Unchecked vector access on truncated input** — the same function reads
   `_tokens_vector[index + 1]` and `_tokens_vector[index + 2]` with no bounds check.
   A config file truncated right after a bare `server` or `location` keyword (cases
   37/38) triggers undefined behavior via `std::vector::operator[]` instead of the
   graceful `ERRS_PARSER_UNTERMINATED_DIRECTIVE` error. See probe results above for
   whether this crashed on this run.
3. **Reused error text across directives (FIXED)** — a bad `return` code used to print
   the error_page-flavored message "Invalid error code in error_page directive: ".
   `Parser::setupReturn` now throws "Invalid error code in return directive" (case 29),
   so `return` and `error_page` code errors are distinct.
4. **`ERRS_PARSER_SCOPE_OVERFLOW` appears unreachable** — every call site of
   `Parser::upperScope()` (Parser.cpp:71-79) is already guarded by a state check
   that throws its own error first, so the SCOPE_OVERFLOW branch looks like dead code.
5. **A missing `;` mid-file does not raise "unterminated directive"** — `Lexer`
   strips newlines, so `findNextSemicolon` (Parser.cpp:82-91) just keeps
   consuming tokens across line boundaries until it finds *any* semicolon.
   A missing `;` after one directive silently glues it to the next directive's
   tokens, surfacing as a confusing `Invalid syntax for key <first-directive>`
   instead of pointing at the real problem (case 04b). `ERRS_PARSER_UNTERMINATED_DIRECTIVE`
   is only reachable when no semicolon exists anywhere for the rest of the file
   (case 04).
6. **`RUN` is unconditionally `#define`d to `1`** in `utils/utils.hpp:15`, so
   `main.cpp`'s `#if RUN == 1` block always compiles in — any config that
   parses *and* binds successfully makes `WebServ` block forever in its
   accept/poll loop with no way to know from the outside that startup
   succeeded (no "listening" message). This test script works around it by
   backgrounding every invocation and force-killing it after ~2s.

MDEOF

    echo "## Full results"
    echo
    echo "| File | Expected | Verdict | Actual output |"
    echo "|---|---|---|---|"
    echo "$control_line"
    printf '%b' "$results"
} > "$REPORT"

echo
echo "== Done: $pass/$total passed, $fail_mismatch mismatched, $fail_crash crashed, $info probes =="
echo "Report written to $REPORT"
