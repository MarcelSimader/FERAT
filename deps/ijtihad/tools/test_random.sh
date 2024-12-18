#!/bin/bash

SPACE_LIMIT=8000000
TIME_LIMIT=400

WORK_DIR="/home/vedad/Informatik/IAIK/mysolver/tools"

GENERATOR="$WORK_DIR/blocksqbf"
TEST_DIR="$WORK_DIR/../tests"
TMP_DIR="$WORK_DIR/../tmp"

BC="/usr/bin/bc"
TIME="/usr/bin/time"
TIMEOUT="$WORK_DIR/ctimeout.sh"
RESULTS="$WORK_DIR/results.txt"
printf "" > $RESULTS

PROGRAMS=( "mysolver" "rareqs")

cp "$WORK_DIR/${PROGRAMS[0]}" "$TMP_DIR/${PROGRAMS[0]}"
cp "$WORK_DIR/${PROGRAMS[1]}" "$TMP_DIR/${PROGRAMS[1]}"

STEP=10
TESTS=10
MAX_TIME=400

RES_FORMAT_SAT=( "s cnf 1" "s cnf 1" )
RES_FORMAT_UNSAT=( "s cnf 0" "s cnf 0")

for alternations in `seq 1 20` 
do
    command0="-b $alternations"
    
    for vars in `seq 1 100`
    do
        command1=$command0
        for i in `seq 1 ${alternations}`
        do
            command1="$command1 -bs $vars"
        done
        
        for lits in `seq 1 $vars`
        do
            command2=$command1
            for i in `seq 1 ${alternations}`
            do
                command2="$command2 -bc $lits"
            done
            echo $command2;
            
            num_clauses=1
            total_sat=0
            total_unsat=0
            total_error=0
            
            while [[ "$(($total_unsat + $total_error))" -ne "$(($TESTS + $TESTS))" ]]
            do
                pid_sol=( 0 0 )
                for t in `seq 1 $TESTS`
                do
                    t1="t-${alternations}-${vars}-${lits}-${t}.q"
                    echo "$GENERATOR -c $num_clauses $command2 > $TEST_DIR/$t1"
                    eval "$GENERATOR -c $num_clauses $command2 > $TEST_DIR/$t1"
                    
                    for prog in `seq 0 1`
                    do 
                    {
                        solver=${PROGRAMS[$prog]}
                        "$TIME" -f "%M %U %S %E" -o "$TMP_DIR/time-usage-$t1-${solver}.txt" \
      "$TIMEOUT" $TIME_LIMIT $SPACE_LIMIT "$TMP_DIR/$solver" "$TEST_DIR/$t1" > "$TMP_DIR/$t1-${solver}.out"
                    } & pid_sol[$prog]=$!
                    done
                    
                    for prog in `seq 0 1`
                    do 
                        echo "Waiting for pid_sol[$prog] = ${pid_sol[$prog]}"
                        wait ${pid_sol[$prog]}
                       
                        solver="${PROGRAMS[$prog]}"

                        time_usages=( `cat "$TMP_DIR/time-usage-$t1-${solver}.txt"` )
                      
                        test_space=`echo "a=(${time_usages[-4]} / 1000.0); scale=0; a/1" | "$BC" -l`
                        test_time=`echo "${time_usages[-2]} + ${time_usages[-3]}" | "$BC"`
                        ret=""
                        ## save result and add it to totals
                        if [[ `grep "${RES_FORMAT_SAT[$prog]}" "$TMP_DIR/$t1-${solver}.out"` ]]
                        then
                          ret="SAT"
                          total_sat=$(($total_sat + 1))
                        elif [[ `grep "${RES_FORMAT_UNSAT[$prog]}" "$TMP_DIR/$t1-${solver}.out"` ]]
                        then
                          ret="UNSAT"
                          total_unsat=$(($total_unsat + 1))
                        else
                          ret="ERROR"
                          total_error=$(($total_error + 1))
                        fi
                        printf "%7s %5s %5s " ${test_time} ${test_space} ${ret} >> "$RESULTS"
                    done
                    printf "$t1 \n" >> "$RESULTS"
                done
                
                num_clauses=`echo "a=(${num_clauses} * 2); a" | "$BC" -l`
            done
        done
    done
    
done

