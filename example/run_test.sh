#!/bin/sh

if [ -z $1 ]; then
  echo "please specify test program to run"
  echo "for example,"
  echo "  ./run_test.sh slip_test"
  exit 1
fi

make
./$1 > $1.csv
if [ -f Pipfile ]; then
  pipenv run ./plot_one_hop.py $1.csv
fi