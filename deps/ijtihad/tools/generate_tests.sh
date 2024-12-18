#!/bin/bash

WORK_DIR="/home/vedad/Informatik/IAIK/mysolver/tools"

GENERATOR="$WORK_DIR/blocksqbf"
TEST_DIR="$WORK_DIR/../tests"

rm -Rf "$TEST_DIR"
mkdir "$TEST_DIR"

# The first 5 test parameters have been tweaked to produce roughly the
# same number of SAT and UNSAT problems in order to test both. 

TEST_PARAMETERS=( \

# "$GENERATOR -c 13 -b 2 -bs 3 -bs 2 -bc 2 -bc 2 > tests/test0w" \

# "$GENERATOR -c 70   -b 3 -bs 2 -bs 3 -bs 2 -bc 2 -bc 2 -bc 2 > tests/test0" \
# "$GENERATOR -c 460  -b 3 -bs 5 -bs 5 -bs 5 -bc 2 -bc 4 -bc 2 > tests/test1" \
# "$GENERATOR -c 75   -b 3 -bs 5 -bs 5 -bs 5 -bc 2 -bc 3 -bc 1 > tests/test2" \
# "$GENERATOR -c 2500 -b 3 -bs 5 -bs 5 -bs 5 -bc 3 -bc 4 -bc 3 > tests/test3" \
# "$GENERATOR -c 3400 -b 3 -bs 5 -bs 8 -bs 5 -bc 2 -bc 6 -bc 3 > tests/test4" \
# "$GENERATOR -c 11000 -b 5 -bs 5 -bs 5 -bs 3 -bs 3 -bs 3 -bc 5 -bc 4 -bc 1 -bc 3 -bc 1 > tests/test5" \
# "$GENERATOR -c 21 -b 5 -bs 1 -bs 1 -bs 1 -bs 1 -bs 1 -bc 1 -bc 1 -bc 1 -bc 1 -bc 1 > tests/test6" \
# "$GENERATOR -c 1400 -b 5 -bs 2 -bs 2 -bs 2 -bs 2 -bs 2 -bc 2 -bc 2 -bc 2 -bc 2 -bc 2 > tests/test7" \

# "$GENERATOR -c 21500 -b 7 -bs 2 -bs 2 -bs 2 -bs 2 -bs 2 -bs 2 -bs 2 -bc 2 -bc 2 -bc 2 -bc 2 -bc 2 -bc 2 -bc 2 > tests/test81" \
# "$GENERATOR -c 23250 -b 7 -bs 5 -bs 2 -bs 2 -bs 2 -bs 2 -bs 2 -bs 2 -bc 2 -bc 2 -bc 2 -bc 2 -bc 2 -bc 2 -bc 2 > tests/test82" \
# "$GENERATOR -c 20750 -b 7 -bs 5 -bs 4 -bs 2 -bs 2 -bs 2 -bs 2 -bs 2 -bc 2 -bc 2 -bc 2 -bc 2 -bc 2 -bc 2 -bc 2 > tests/test83" \
# "$GENERATOR -c 24750 -b 7 -bs 5 -bs 4 -bs 5 -bs 2 -bs 2 -bs 2 -bs 2 -bc 2 -bc 2 -bc 2 -bc 2 -bc 2 -bc 2 -bc 2 > tests/test84" \

# "$GENERATOR -c 419 -b 3 -bs 50 -bs 50 -bs 50 -bc 1 -bc 1 -bc 2 > tests/test0a" \

# "$GENERATOR -c 439 -b 3 -bs 60 -bs 50 -bs 50 -bc 1 -bc 1 -bc 2 > tests/test0b" \

"$GENERATOR -c 641 -b 3 -bs 100 -bs 50 -bs 50 -bc 2 -bc 1 -bc 1 > tests/test0c" \
# "$GENERATOR -c 641 -b 3 -bs 100 -bs 50 -bs 50 -bc 2 -bc 1 -bc 1 > tests/test0d" \


# "$GENERATOR -c 82 -b 7 -bs 1 -bs 1 -bs 1 -bs 1 -bs 1 -bs 1 -bs 1 -bc 1 -bc 1 -bc 1 -bc 1 -bc 1 -bc 1 -bc 1 > tests/test9" \
### "$GENERATOR -c 1290 -b 11 -bs 1 -bs 1 -bs 1 -bs 1 -bs 1 -bs 1 -bs 1 -bs 1 -bs 1 -bs 1 -bs 1 -bc 1 -bc 1 -bc 1 -bc 1 -bc 1 -bc 1 -bc 1 -bc 1 -bc 1 -bc 1 -bc 1 > tests/testX" \
## "$GENERATOR -c 21500 -b 7 -bs 3 -bs 3 -bs 3 -bs 3 -bs 3 -bs 3 -bs 3 -bc 2 -bc 2 -bc 2 -bc 2 -bc 2 -bc 2 -bc 2 > tests/test9" \
# "$GENERATOR -c 20 -b 3 -bs 5 -bs 5 -bs 5 -bc 3 -bc 2 -bc 3 > tests/test9" \
)

for t in {0..499}
do
  if [[ $t -lt 10 ]] 
  then 
    t="000$t" 
  elif [[ $t -lt 100 ]]  
  then
    t="00$t" 
  elif [[ $t -lt 1000 ]]
  then
    t="0$t"
  fi
  for test_g in "${TEST_PARAMETERS[@]}"
  do
    eval "$test_g$t.q"
  done
done
