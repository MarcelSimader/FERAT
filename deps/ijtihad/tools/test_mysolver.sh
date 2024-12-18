#!/bin/bash

# version 3

WORK_DIR="/home/vedad/Informatik/IAIK/rareqs/tools"

TMP_DIR="$WORK_DIR/../tmp"
TEST_DIR="$WORK_DIR/../tests"
SAT_DIR="$WORK_DIR/../tests_sat"
UNSAT_DIR="$WORK_DIR/../tests_unsat"

PREPROCESSOR="$WORK_DIR/bloqqer"

rm -Rf "$TMP_DIR"
mkdir "$TMP_DIR"

PROGRAM="$1"

cp "$WORK_DIR/$PROGRAM" "$TMP_DIR/$PROGRAM"

FAILS=0
SUCCESSES=0

TIMEOUTS=0

function evaluate_tests {

echo "------------------------------ TESTING ------------------------------"

for test in `ls "$1"`
do
  echo "$test"
  # "$PREPROCESSOR" "$1/$test" "$TMP_DIR/prep-$test"
  timeout 30s "$TMP_DIR/$PROGRAM" "$1/$test" > "$TMP_DIR/$test.out"
  if [[ `grep "$2" "$TMP_DIR/$test.out"` ]]
  then
    echo "$test SUCCESS"
    SUCCESSES=$(($SUCCESSES + 1))
  elif [[ `grep "$3" "$TMP_DIR/$test.out"` ]]
  then
    echo "$test FAILED"
    FAILS=$(($FAILS + 1))
  else
    echo "TIMEOUT/ERROR on $test"
    TIMEOUTS=$(($TIMEOUTS + 1))
  fi
done

}

evaluate_tests "$UNSAT_DIR" "s cnf 0" "s cnf 1"
evaluate_tests "$SAT_DIR" "s cnf 1" "s cnf 0"

echo "FAILS: $FAILS     SUCCESSES: $SUCCESSES     TIMEOUTS: $TIMEOUTS"
