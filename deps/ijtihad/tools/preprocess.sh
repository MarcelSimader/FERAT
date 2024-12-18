#!/usr/bin/env bash

WORK_DIR="/home/vedad/Informatik/IAIK/rareqs/tools"

TMP_DIR="$WORK_DIR/../tmp"
PREP_DIR="$WORK_DIR/../prep"
CQBF_DIR="$WORK_DIR/../cqbf"
TEST_DIR="$WORK_DIR/../tests"

PREPROCESSOR="$WORK_DIR/bloqqer"

rm -Rf "$TMP_DIR"
rm -Rf "$PREP_DIR"
rm -Rf "$CQBF_DIR"

mkdir "$TMP_DIR"
mkdir "$PREP_DIR"
mkdir "$CQBF_DIR"

for test in `ls "$TEST_DIR"`
do
  ## Add preprocessed file to prep dir
  echo "Preprocessing $test"
  "$PREPROCESSOR" "$TEST_DIR/$test" "$PREP_DIR/prep-$test"

  ## Add converted test file to prep dir
  echo "Converting $test"
  ( python2 "$WORK_DIR/cqbf-conv.py" "$TEST_DIR/$test" -o "$CQBF_DIR/$test.cqbf" -quiet & )
  echo "Converting prep-$test"
  ( python2 "$WORK_DIR/cqbf-conv.py" "$PREP_DIR/prep-$test" -o "$CQBF_DIR/prep-$test.cqbf" -quiet & )
done

wait