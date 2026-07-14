#!/bin/sh

usage() {
  cat <<USAGE
Test runner script

Synopsis:
  $0 [--memcheck|-m] [-s <file>|--skip=<file>] [--help|-h] [<file>...]

Description:
  This script executes specified tests and reports which tests failed.
  When no tests specified run all tests.

  When --memcheck option is enabled, check memory leak by using
  valgrind.

Options:
  <file>...
    Files to be executed as tests. When no <file>s are provided run
    all executable files.

  -m, --memcheck
    Check memory leak using valgrind.

  -s <file>, --skip=<file>
    Skip execution of a test specified as <file>.

  -h, --help
    Show this message.
USAGE
}

# Parse optional arguments.
memcheck=0
skipped=''
while :; do
  case $1 in
    -h|--help)
      usage
      exit
      ;;
    -m|--memcheck)
      memcheck=1
      ;;
    -s|--skip)
      if [ -n "$2" ] && echo "$2" | grep -q -- '^[^-]'; then
        skipped="$skipped $(basename "$2")"
        shift
      else
        echo >&2 "error: --skip requires a non-empty argument"
        exit 1
      fi
      ;;
    --skip=?*)
      skipped="$skipped $(basename "${1#*=}")"
      ;;
    '--skip=')
      echo >&2 "error: --skip requires a non-empty argument"
      exit 1
      ;;
    --)
      shift
      break
      ;;
    -?*)
      echo >&2 "warning: unknown option: $1 (ignored)"
      ;;
    *)
      break
      ;;
  esac
  shift
done

# Set shell options.
set -eu

# Create a temp directory to put a variety of stuff that should be
# deleted when the script exits.
tmpdir=
failed_tests=
cleanup() {
  trap - EXIT
  [ -n "$tmpdir" ] && rm -rf "$tmpdir"
  [ -n "${1:-}" ] && {
    trap - "$1"
    # shellcheck disable=SC2086
    kill -$1 $$
  }
  [ -z "$failed_tests" ]
  exit
}
trap 'cleanup' EXIT
trap 'cleanup HUP' HUP
trap 'cleanup TERM' TERM
trap 'cleanup INT' INT
tmpdir=$(mktemp -d)

# Returns true (0) when a string contains a specific substring. The
# substring to be tested is given as an argument before 'in', and the
# argument after that is considered as the string to be be checked if
# it contains the substring.
# Examples:
#   $ fruits="banana apple orange"
#   $ contains apple in "$fruits"         # returns 0
#   $ contains apple banana in "$fruits"  # returns 0
#   $ contains mango in "$fruits"         # returns 1
#   $ contains apple mango in "$fruits"   # returns 1
contains() {
  testee=''
  while :; do
    [ "$#" -eq 0 ] && {
      echo >&2 "error: contains: incorrect usage"
      return 1
    }
    [ "$1" = 'in' ] && { shift; break; }
    testee="$testee $1"
    shift
  done
  [ -z "$testee" ] && {
    echo >&2 "error: contains: no testing element"
    return 1
  }
  for t in $testee; do
    case "$*" in
      *"$t"*) ;;
      *) return 1 ;;
    esac
  done
  return 0
}

# Runs a test. When --memcheck option is enabled, run it through
# valgrind with --leak-check=full enabled.
run_test() {
  if [ $# -eq 0 ]; then
    echo >&2 "error: run_test: no test is specified"
    exit 1
  fi
  if [ ! -f "$1" ]; then
    echo >&2 "warning: run_test: no such test name: $1"
    return 1
  fi
  echo
  echo "Running $(basename "$1")..."
  if [ $((memcheck)) -ne 0 ] && ! command -v valgrind >/dev/null; then
    echo >&2 "error: memcheck is enabled, but valgrind is not available"
    exit 1
  fi
  if [ $((memcheck)) -ne 0 ]; then
    valgrind -q --leak-check=full --error-exitcode=1 "$1"
  else
    "$1"
  fi
  return $?
}

# Takes path to a specific executable file to run as a test and runs
# it unless it is included in the skipped list.
try_run_test() {
  if contains "$(basename "$1")" in "$skipped"; then
    return 0
  else
    run_test "$1"
    return $?
  fi
}

# Prints ANSI escape code to set foreground color to stdout.
setaf() {
  nc=$(tput colors 2>/dev/null ||:)
  if [ -z "$nc" ] || [ "$nc" -lt 8 ]; then
    return
  fi
  val=''
  case $1 in
    black)   val=0 ;;
    red)     val=1 ;;
    green)   val=2 ;;
    yellow)  val=3 ;;
    blue)    val=4 ;;
    magenta) val=5 ;;
    cyan)    val=6 ;;
    white)   val=7 ;;
    *) echo >&2 "warning: unknown color name: $1" ;;
  esac
  tput setaf $val
}

# Prints the result summary after all tests are executed. Supposed
# that failed tests are passed as arguments. All tput invocations must
# tolerate a missing/dumb $TERM (e.g. CI runners): under `set -e` an
# unguarded tput failure would abort the whole runner with its exit
# code even though every test passed.
report_summary() {
  reset=$(tput sgr0 2>/dev/null ||:)
  red=$(setaf red)
  green=$(setaf green)
  cols=$(tput cols 2>/dev/null ||:)
  echo
  if [ $# -eq 0 ]; then
    echo "${green}All passed!${reset}"
  else
    printf "%s" "$red"
    printf -- "=%.0s" $(seq "${cols:-70}")
    printf "\n"
    printf "Test failed!\n"
    for i in "$@"; do
      printf "  * %s\n" "$i"
    done
    printf -- "=%.0s" $(seq "${cols:-70}")
    printf "%s\n" "$reset"
  fi
}

# Get the directory path where this script exists.
test_directory=$(cd -- "$(dirname -- "$0")" && pwd -P)

# Prints executable files in the specified directory, but exclude
# executable shell scripts.
find_all_tests() {
  find "${1:-$(pwd)}" -maxdepth 1 -type f -executable | grep -vE '\.(bash|sh)$'
}

# Given no arguments, runs all tests in the directory where this
# script exists. Otherwise, runs specified tests.
main() {
  pipe="$tmpdir/pipe"
  mkfifo "$pipe"; :>"$pipe" &
  if [ $# -eq 0 ]; then
    find_all_tests "$test_directory" | while IFS='' read -r filepath; do
      if ! try_run_test "$filepath"; then
        basename "$filepath" >"$pipe" &
      fi
    done
  else
    for filepath in "$@"; do
      case "$filepath" in
        /*|./*) ;;
        *) filepath="$(pwd)/$filepath" ;;
      esac
      if ! try_run_test "$filepath"; then
        basename "$filepath" >"$pipe" &
      fi
    done
  fi
  # wait until writing pipe.
  sleep 0.01

  failed_tests=''
  while IFS='' read -r line; do
    failed_tests="$failed_tests $line"
  done <"$pipe"
  # shellcheck disable=SC2086
  report_summary $failed_tests
}

# Skip rhc_test_test because it always fails.
if [ $# -eq 0 ]; then
  skipped="${skipped} rhc_test_test"
fi
main "$@"
