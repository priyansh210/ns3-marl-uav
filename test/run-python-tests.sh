#!/usr/bin/env bash

# Simple shell script to execute the python tests one by one

set -eo pipefail

if [ -z "$NS3_HOME" ]; then
  echo "You have to specify NS3_HOME in order to test this module"
  exit 1
fi
cd "$NS3_HOME" || exit 1

python_tests="$(pytest --collect-only | awk '{print|"cat 1>&2";print}' | grep Function | sed -Ee 's/^\s*<Function|>$//g')"
echo "$python_tests" | while read -r line
do
  pytest -s -k "$line"
done
