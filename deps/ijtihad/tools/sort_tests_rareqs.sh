#!/bin/bash

WORK_DIR="/home/vedad/Informatik/IAIK/rareqs/tools"

PROGRAM="$WORK_DIR/../rareqs-old"
TMP_DIR="$WORK_DIR/../tmp"
TEST_DIR="$WORK_DIR/../tests"
SAT_DIR="$WORK_DIR/../tests_sat"
UNSAT_DIR="$WORK_DIR/../tests_unsat"


SAT_COUNT=0
UNSAT_COUNT=0

rm -Rf "$TMP_DIR"
mkdir "$TMP_DIR"

rm -Rf "$SAT_DIR"
mkdir "$SAT_DIR"

rm -Rf "$UNSAT_DIR"
mkdir "$UNSAT_DIR"

for test in `ls "$TEST_DIR"`
do
  echo "$test"
  $PROGRAM "$TEST_DIR/$test" > "$TMP_DIR/$test.out"
  
  if [[ `grep "s cnf 1" "$TMP_DIR/$test.out"` ]]
  then
    echo "$test is SAT" 
    SAT_COUNT=$(($SAT_COUNT + 1))
    cp "$TEST_DIR/$test" "$SAT_DIR/$test"
  elif [[ `grep "s cnf 0" "$TMP_DIR/$test.out"` ]]
  then
    echo "$test is UNSAT"
    UNSAT_COUNT=$(($UNSAT_COUNT + 1))
    cp "$TEST_DIR/$test" "$UNSAT_DIR/$test"
  else
    echo "TIMEOUT/ERROR on $test"
  fi
done

echo "SAT : $SAT_COUNT        UNSAT : $UNSAT_COUNT"
