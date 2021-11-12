#!/bin/sh

usage() {
  cat <<USAGE
Run example program and make plot.

Synopsis:
  $0 [-d <file>|--data=<file>] [-p <file>|--plot-script=<file>]
     [-D|--delete] [-P|--plot-only] [--no-make] [--no-plot]
     <file> [<arg>...]
  $0 [-h|--help]

Description:
  This script runs a specified example program and shows plotting of
  the produced data.

  To plot the data, a python script whose name matches the specified
  example program and ends with .py extension is used. Let's say you
  have an example program 'slip_test', you need a python script
  'slip_test.py' to plot the produced data. Or, you can specify an
  alternative script by using '--plot-script' option, such as
  'plot_one_hop.py'.

  By default, the data file produced from the program is named as the
  same as the program name with '.csv' extension, for example,
  'slip_test.csv'.

Options:
  <file>
    Program file to be executed.

  <arg>...
    Arguments to be passed to the example program.

  -d <file>, --data=<file>
    Use <file> as an intermediate data.

  -p <file>, --plot-script=<file>
    Specify a script file to execute to make plot.

  -D, --delete
    Delete the intermediate data file after plotting.

  -P, --plot-only
    Do not execute the program. Instead, plot using existing data
    file.

  --no-plot
    Do not plot.

  --no-make
    Do not run make.

  -h, --help
    Show this message.

Examples:
  Execute 'slip_test', then show plot using 'slip_test.py'
    $0 slip_test

  Show plot using 'slip_test.py' with 'slip_test.csv'.
    $0 --plot-only slip_test

  Show plot using 'plot_one_hop.py' with 'slip_test.csv'.
    $0 --plot-script plot_one_hop.py slip_test

  Output data to 'slip_test.log' and use it to plot.
    $0 --data=slip_test.log slip_test

  Delete the data file after plotting.
    $0 --delete slip_test
USAGE
}

data=''
plot_script=''
delete=0
make=1
run=1
plot=1
while :; do
  case $1 in
    -h|--help)
      usage
      exit
      ;;
    -d|--data)
      if [ -n "$2" ] && echo "$2" | grep -q -- '^[^-]'; then
        data="$2"
        shift
      else
        echo >&2 "error: --data reauires a non-empty argument"
        exit 1
      fi
      ;;
    --data=?*)
      data="${1#*=}"
      ;;
    '--data=')
      echo >&2 "error: --data reauires a non-empty argument"
      exit 1
      ;;
    -p|--plot-script)
      if [ -n "$2" ] && echo "$2" | grep -q -- '^[^-]'; then
        plot_script="$2"
        shift
      else
        echo >&2 "error: --plot-script reauires a non-empty argument"
        exit 1
      fi
      ;;
    --plot-script=?*)
      plot_script="${1#*=}"
      ;;
    '--plot-script=')
      echo >&2 "error: --plot-script reauires a non-empty argument"
      exit 1
      ;;
    -D|--delete)
      delete=1
      ;;
    -P|--plot-only)
      make=0
      run=0
      plot=1
      ;;
    --no-plot)
      plot=0
      ;;
    --no-make)
      make=0
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

# Cleans up the produced intermediate data file when exiting.
cleanup() {
  trap - EXIT
  if [ $((delete)) -ne 0 ]; then
    [ -n "$data" ] && rm -f "$data"
  fi
}
trap 'cleanup' EXIT

# Set shell options.
set -eu

# Exit with error when no program is specified.
if [ $# -eq 0 ]; then
  echo >&2 "error: program name is required"
  echo >&2 "Example:"
  echo >&2 "  $0 slip_test"
  exit 1
fi

# Run make.
if [ $((make)) -ne 0 ]; then
  make
fi

# Run program and redirect produced output to an intermediate data
# file.
program=$1; shift
case "$program" in
  /*|./*) ;;
  *) program="$(pwd)/$program" ;;
esac
program_name=$(basename "$program")
if [ $((run)) -ne 0 ]; then
  ret=
  if [ -f "$program" ]; then
    [ -z "$data" ] && data="${program_name}.csv"
    # "$program" "$@" >"$data"
    "$program" "$@" '>' "$data"
    ret=$?
  else
    echo >&2 "error: $program not found"
    exit 1
  fi

  # When the program exits with error, abort it.
  if [ $ret -ne 0 ]; then
    echo >&2 "error: ${program} exited with non-zero status"
    exit $ret
  fi
  unset ret
fi

# Terminate when --no-plot is enabled.
[ $((plot)) -eq 0 ] && exit 0

# Plot the data with an appropriate Python script.
example_directory=$(cd -- "$(dirname -- "$0")" && pwd -P)
if [ -n "$plot_script" ]; then
  # Fix path to the plot script specified by the user.
  if [ ! -f "$plot_script" ]; then
    if [ -f "${example_directory}/${plot_script}.py" ]; then
      plot_script=${example_directory}/${plot_script}.py
    elif [ -f "${example_directory}/${plot_script}" ]; then
      plot_script=${example_directory}/${plot_script}
    else
      echo >&2 "error: specified plot script not found: $plot_script"
      exit 1
    fi
  fi
else
  # Find an accessible script since none is specified by the user.
  if [ -f "${example_directory}/${program_name}.py" ]; then
    plot_script=${example_directory}/${program_name}.py
  elif [ -f "${example_directory}/${program_name}" ]; then
    plot_script=${example_directory}/${program_name}
  else
    echo >&2 "error: Python script to plot not found"
    echo >&2 "Or use plot_one_hop.py like as:"
    echo >&2 "  $0 -p plot_one_hop slip_test"
    exit 1
  fi
fi

if command -v poetry >/dev/null && \
    [ -f "${example_directory}/pyproject.toml" ]; then
  poetry run "$plot_script" "$data"
elif command -v pipenv >/dev/null && \
    [ -f "${example_directory}/Pipfile" ]; then
  pipenv run "$plot_script" "$data"
elif command -v python >/dev/null; then
  py_ver=$(python --version | cut -d' ' -f2)
  if [ "${py_ver%%.*}" -gt 2 ]; then
    python "$plot_script" "$data"
  else
    echo >&2 "error: Python3 is required"
    exit 1
  fi
else
  echo >&2 "error: Python is not availble on the system"
  exit 1
fi