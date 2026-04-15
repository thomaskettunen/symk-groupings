#!/bin/bash
[ -e ./data ] && { echo "./data exists. Exiting." ; exit 1; }

for name in ./runs/* ;
do echo "$name" ;
   mv "$name/data" ./data ;
   yes 'o' | ./run_experiments.sh -- 3 4 5 ;
   mv ./data "$name/data" ;
done ;
i=0
while [ -e ./runs.zip.$i ]; do i=(( i + 1 )); done
zip -r runs.zip.$i runs