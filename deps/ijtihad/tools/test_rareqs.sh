#!/bin/bash

# version 1

WORK_DIR="/home/vedad/Informatik/IAIK/rareqs/tools"

PROGRAM="$WORK_DIR/ghostq"
TMP_DIR="$WORK_DIR/../tmp"
TEST_DIR="$WORK_DIR/../tests"
SAT_DIR="$WORK_DIR/../tests_sat"
UNSAT_DIR="$WORK_DIR/../tests_unsat"

rm -Rf "$TMP_DIR"
mkdir "$TMP_DIR"

PROGRAM="rareqs-old"

cp "$WORK_DIR/../$PROGRAM" "$TMP_DIR/$PROGRAM"

FAILS=0
SUCCESSES=0

echo "--------------------- TESTING UNSAT ---------------------"

for test in `ls "$UNSAT_DIR"`
do  
  echo "$test"
  "$TMP_DIR/$PROGRAM" "$UNSAT_DIR/$test" > "$TMP_DIR/$test.out"
  if [[ `grep "s cnf 0" "$TMP_DIR/$test.out"` ]]
  then
    echo "$test SUCCESS" 
    SUCCESSES=$(($SUCCESSES + 1))
  elif [[ `grep "s cnf 1" "$TMP_DIR/$test.out"` ]]
  then
    echo "$test FAILED"
    FAILS=$(($FAILS + 1))
  else
    echo "TIMEOUT/ERROR on $test"
  fi
done

echo "---------------------- TESTING SAT ----------------------"

for test in `ls "$SAT_DIR"`
do  
  echo "$test"
  "$TMP_DIR/$PROGRAM" "$SAT_DIR/$test" > "$TMP_DIR/$test.out"
  if [[ `grep "s cnf 1" "$TMP_DIR/$test.out"` ]]
  then
    echo "$test SUCCESS" 
    SUCCESSES=$(($SUCCESSES + 1))
  elif [[ `grep "s cnf 0" "$TMP_DIR/$test.out"` ]]
  then
    echo "$test FAILED"
    FAILS=$(($FAILS + 1))
  else
    echo "TIMEOUT/ERROR on $test"
  fi
done

echo "FAILS: $FAILS      SUCCESSES: $SUCCESSES"
