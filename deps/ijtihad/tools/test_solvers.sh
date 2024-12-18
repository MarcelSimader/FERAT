#!/bin/bash

# version 6

SPACE_LIMIT=4194304
TIME_LIMIT=100

WORK_DIR="/home/vedad/Informatik/IAIK/rareqs/tools"

cp "$WORK_DIR/../mysolver" "$WORK_DIR/mysolver"
cp "$WORK_DIR/../rareqs" "$WORK_DIR/rareqs"

TMP_DIR="$WORK_DIR/../tmp"
PREP_DIR="$WORK_DIR/../prep"
CQBF_DIR="$WORK_DIR/../cqbf"

TEST_DIR="$WORK_DIR/../tests"
TIME="$WORK_DIR/time"
TIMEOUT="$WORK_DIR/ctimeout.sh"


PREPROCESSOR="$WORK_DIR/bloqqer"

rm -Rf "$TMP_DIR"
rm -Rf "$PREP_DIR"
rm -Rf "$CQBF_DIR"

mkdir "$TMP_DIR"
mkdir "$PREP_DIR"
mkdir "$CQBF_DIR"

PROGRAMS=( "mysolver" "rareqs" "ghostq" )

RES_FORMAT_SAT=( "s cnf 1" "s cnf 1" "true" )
RES_FORMAT_UNSAT=( "s cnf 0" "s cnf 0" "false" )

## the arrays bellow hold the statistics for the solvers

sat_totals1=( 0 0 0 )
unsat_totals1=( 0 0 0 )
timeout_totals1=( 0 0 0 )

sat_totals2=( 0 0 0 )
unsat_totals2=( 0 0 0 )
timeout_totals2=( 0 0 0 )

total_time1=( 0 0 0 )
total_time2=( 0 0 0 )
test_time1=( 0 0 0 )
test_time2=( 0 0 0 )

test_space1=( 0 0 0 )
test_space2=( 0 0 0 )


printf "|   MySolver    |    RAReQS     |    GhostQ     ||   MySolver    |    RAReQS     |    GhostQ     | Test \n" \
    > "$WORK_DIR/results.txt"

printf "+---------------+---------------+---------------++---------------+---------------+---------------+------------------------------------\n" \
    >> "$WORK_DIR/results.txt"

for test in `ls "$TEST_DIR"`
do
  ## used for saving the results of the solvers

  ret1=( "" "" "" )
  ret2=( "" "" "" )

  ## Preprocess the test file and add it to the prep dir
  echo "Preprocessing $test"
  "$PREPROCESSOR" "$TEST_DIR/$test" "$PREP_DIR/prep-$test"

  ## Convert the original and preprocessed files into ghostq readable format
  ## This is done in parallel by adding & at the end and saving the pids for joins
  echo "Converting $test"
  python2 "$WORK_DIR/cqbf-conv.py" "$TEST_DIR/$test" -o "$CQBF_DIR/$test.cqbf" -quiet & pid_c1=$!
  python2 "$WORK_DIR/cqbf-conv.py" "$PREP_DIR/prep-$test" -o "$CQBF_DIR/prep-$test.cqbf" -quiet & pid_c2=$! 

  ## Wait for the two conversions to be done
  wait $pid_c1 $pid_c2

  ## start normal and preprocessed test for each of the solvers in parallel
  for index in 0 1 2
  do
    solver="${PROGRAMS[$index]}"
    if [[ $index = 2 ]]
    then
      t1="$CQBF_DIR/$test.cqbf"
      t2="$CQBF_DIR/prep-$test.cqbf"
    else
      t1="$TEST_DIR/$test"
      t2="$PREP_DIR/prep-$test"
    fi

    ## run the program with limited time and save time usage into a file
    "$TIME" -f "%M %U %S %E " -o "$TMP_DIR/time-usage-$solver-1.txt" \
      "$TIMEOUT" $TIME_LIMIT $SPACE_LIMIT "$WORK_DIR/$solver" "$t1" > "$TMP_DIR/$test-$solver.out" \
      & pid_sol[$(($index * 2 + 0))]=$!

    ## run the program with limited time and save time usage into a file
    "$TIME" -f "%M %U %S %E " -o "$TMP_DIR/time-usage-$solver-2.txt" \
      "$TIMEOUT" $TIME_LIMIT $SPACE_LIMIT "$WORK_DIR/$solver" "$t2" > "$TMP_DIR/prep-$test-$solver.out" \
      & pid_sol[$(($index * 2 + 1))]=$!
  done

  ## wait for all solvers to finish the test
  for i in `seq 0 5`
  do
    wait ${pid_sol[$i]}
  done

  ## recover the results and times for each of the test runs joined above
  for index in 0 1 2
  do
    solver="${PROGRAMS[$index]}"

    time_usages=( `cat "$TMP_DIR/time-usage-$solver-1.txt"` )
    test_time1[$index]=`echo "${time_usages[-2]} + ${time_usages[-3]}" | bc`
    test_space1[$index]=`echo "${time_usages[0]} / 1024" | bc`
    total_time1[$index]=`echo "${total_time1[$index]} + ${test_time1[$index]}" | bc`

    ## save result and add it to totals
    if [[ `grep "${RES_FORMAT_SAT[$index]}" "$TMP_DIR/$test-$solver.out"` ]]
    then
      ret1[$index]="SAT"
      sat_totals1[$index]=$(( ${sat_totals1[$index]} + 1 ))
    elif [[ `grep "${RES_FORMAT_UNSAT[$index]}" "$TMP_DIR/$test-$solver.out"` ]]
    then
      ret1[$index]="UNSAT"
      unsat_totals1[$index]=$(( ${unsat_totals1[$index]} + 1 ))
    else
      ret1[$index]="ERROR"
      timeout_totals1[$index]=$(( ${timeout_totals1[$index]} + 1 ))
    fi

    time_usages=( `cat "$TMP_DIR/time-usage-$solver-2.txt"` )
    test_time2[$index]=`echo "${time_usages[-2]} + ${time_usages[-3]}" | bc`
    total_time2[$index]=`echo "${total_time2[$index]} + ${test_time2[$index]}" | bc`

    ## save result and add it to totals
    if [[ `grep "${RES_FORMAT_SAT[$index]}" "$TMP_DIR/prep-$test-$solver.out"` ]]
    then
      ret2[$index]=" SAT "
      sat_totals2[$index]=$(( ${sat_totals2[$index]} + 1 ))
    elif [[ `grep "${RES_FORMAT_UNSAT[$index]}" "$TMP_DIR/prep-$test-$solver.out"` ]]
    then
      ret2[$index]="UNSAT"
      unsat_totals2[$index]=$(( ${unsat_totals2[$index]} + 1 ))
    else
      ret2[$index]="ERROR"
      timeout_totals2[$index]=$(( ${timeout_totals2[$index]} + 1 ))
    fi
  done

  printf "| %7s %5s | %7s %5s | %7s %5s || %7s %5s | %7s %5s | %7s %5s | $test\n" \
    ${test_time1[0]} ${ret1[0]}  ${test_time1[1]} ${ret1[1]} ${test_time1[2]} ${ret1[2]} \
    ${test_time2[0]} ${ret2[0]}  ${test_time2[1]} ${ret2[1]} ${test_time2[2]} ${ret2[2]} \
    >> "$WORK_DIR/results.txt"

done

printf "+---------------+---------------+---------------++---------------+---------------+---------------+------------------------------------\n" \
    >> "$WORK_DIR/results.txt"

printf "| %13s | %13s | %13s || %13s | %13s | %13s | TOTAL TIME\n" \
${total_time1[0]} ${total_time1[1]} ${total_time1[2]} \
${total_time2[0]} ${total_time2[1]} ${total_time2[2]} \
>> "$WORK_DIR/results.txt"

printf "| %13s | %13s | %13s || %13s | %13s | %13s | TOTAL SAT\n" \
${sat_totals1[0]} ${sat_totals1[1]} ${sat_totals1[2]} \
${sat_totals2[0]} ${sat_totals2[1]} ${sat_totals2[2]} \
>> "$WORK_DIR/results.txt"

printf "| %13s | %13s | %13s || %13s | %13s | %13s | TOTAL UNSAT\n" \
${unsat_totals1[0]} ${unsat_totals1[1]} ${unsat_totals1[2]} \
${unsat_totals2[0]} ${unsat_totals2[1]} ${unsat_totals2[2]} \
>> "$WORK_DIR/results.txt"

printf "| %13s | %13s | %13s || %13s | %13s | %13s | TOTAL ERROR\n" \
${timeout_totals1[0]} ${timeout_totals1[1]} ${timeout_totals1[2]} \
${timeout_totals2[0]} ${timeout_totals2[1]} ${timeout_totals2[2]} \
>> "$WORK_DIR/results.txt"
