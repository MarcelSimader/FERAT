SPACE_LIMIT=8000000
TIME_LIMIT=$1

WORK_DIR="/workstore/vhadzic/tools"

RUN_NUM=$1
TEST_SET=$2

echo "$RUN_NUM $TIME_LIMIT"

TMP_DIR="$WORK_DIR/../tmp-$RUN_NUM$TEST_SET"
PREP_DIR="$WORK_DIR/../prep-$RUN_NUM$TEST_SET"
CQBF_DIR="$WORK_DIR/../cqbf$RUN_NUM$TEST_SET"

mkdir "$WORK_DIR/results/res-${RUN_NUM}"

RESULTS="$WORK_DIR/results/res-${RUN_NUM}/${TEST_SET}.txt"

TEST_DIR="$WORK_DIR/../tests$TEST_SET"
TIME="/usr/bin/time"
TIMEOUT="$WORK_DIR/ctimeout.sh"
BC="$WORK_DIR/bc"

PREPROCESSOR="$WORK_DIR/bloqqer31"

rm -Rf "$TMP_DIR"
rm -Rf "$PREP_DIR"
rm -Rf "$CQBF_DIR"

mkdir "$TMP_DIR"
mkdir "$PREP_DIR"
mkdir "$CQBF_DIR" 

PROGRAMS=( "mysolver5" "rareqs")

cp "$WORK_DIR/${PROGRAMS[0]}" "$TMP_DIR/${PROGRAMS[0]}"
cp "$WORK_DIR/${PROGRAMS[1]}" "$TMP_DIR/${PROGRAMS[1]}"

RES_FORMAT_SAT=( "s cnf 1" "s cnf 1" )
RES_FORMAT_UNSAT=( "s cnf 0" "s cnf 0")

## the arrays bellow hold the statistics for the solvers

sat_totals=( 0 0 )
unsat_totals=( 0 0 )
timeout_totals=( 0 0 )

total_time=( 0 0 )
test_time=( 0 0 )
test_space=( 0 0 )
prep_time=( 0 0 )

printf "" > "$RESULTS"

for test in `ls "$TEST_DIR"`
do
  ## used for saving the results of the solvers

  ret=( "" "" )

  ## start normal and preprocessed test for each of the solvers in parallel
  for index in 0 1
  do
    {
    solver="${PROGRAMS[$index]}"
    
    ## if [[ $index = 1 ]]
    ## then
      ## Preprocess the test file and add it to the prep dir
      ## echo "Preprocessing $test"
      ## "$TIME" -f "%U %S %E" -o "$TMP_DIR/time-usage-preprocessor-${test}.txt" \
      ##   "$TIMEOUT" $TIME_LIMIT $SPACE_LIMIT "$PREPROCESSOR" "$TEST_DIR/$test" "$PREP_DIR/prep-$test"

      ## time_usages=( `cat "$TMP_DIR/time-usage-preprocessor-${test}.txt"` )
      ## prep_time[1]=`echo "${time_usages[-2]} + ${time_usages[-3]}" | "$BC"`
     
      # t1="$PREP_DIR/prep-$test"
    # else
      prep_time[$index]=0
      t1="$TEST_DIR/$test"
    # fi

    time_limit_new=`echo "a=($TIME_LIMIT - ${prep_time[$index]}); scale=0; a/1" | "$BC" -l`
    echo "$time_limit_new"
    
    ## run the program with limited time and save time usage into a file
    "$TIME" -f "%M %U %S %E" -o "$TMP_DIR/time-usage-$test-$index-${solver}.txt" \
      "$TIMEOUT" $time_limit_new $SPACE_LIMIT "$TMP_DIR/$solver" "$t1" > "$TMP_DIR/$test-$index-${solver}.out"
    } & pid_sol[$index]=$!
  done

  ## wait for all solvers to finish the test
  for index in 0 1
  do
    echo "Waiting for pid_sol[$index] = ${pid_sol[$index]}"
    wait ${pid_sol[$index]}
    
    ## if [[ $index = 1 ]]
    ## then
    ##  time_usages=( `cat "$TMP_DIR/time-usage-preprocessor-${test}.txt"` )
    ##  prep_time[1]=`echo "${time_usages[-2]} + ${time_usages[-3]}" | "$BC"`
    ##else
      prep_time[$index]=0
    ##fi 
    solver="${PROGRAMS[$index]}"

    time_usages=( `cat "$TMP_DIR/time-usage-$test-$index-${solver}.txt"` )
  
    test_space[$index]=`echo "a=(${time_usages[-4]} / 1000.0); scale=0; a/1" | "$BC" -l`
    test_time[$index]=`echo "${prep_time[$index]} + ${time_usages[-2]} + ${time_usages[-3]}" | "$BC"`
    total_time[$index]=`echo "${total_time[$index]} + ${test_time[$index]}" | "$BC"`

    ## save result and add it to totals
    if [[ `grep "${RES_FORMAT_SAT[$index]}" "$TMP_DIR/$test-$index-${solver}.out"` ]]
    then
      ret[$index]="SAT"
      sat_totals[$index]=$(( ${sat_totals[$index]} + 1 ))
    elif [[ `grep "${RES_FORMAT_UNSAT[$index]}" "$TMP_DIR/$test-$index-${solver}.out"` ]]
    then
      ret[$index]="UNSAT"
      unsat_totals[$index]=$(( ${unsat_totals[$index]} + 1 ))
    else
      ret[$index]="ERROR"
      timeout_totals[$index]=$(( ${timeout_totals[$index]} + 1 ))
    fi
    printf "%7s %5s %5s " ${test_time[$index]} ${test_space[$index]} ${ret[$index]} >> "$RESULTS"
  done
  printf "$test \n" >> "$RESULTS"
done

printf "+---------------------+------------------------------------\n" \

