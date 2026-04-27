#!/bin/bash

# usage: ./parse_all.sh data/*.runs

[ -e ./data ] || { echo "no ./data. Exiting." ; exit 1; }
rm -rf ./data/experiments-eval

for name in $@ ;
do echo "parsing $name" ;
   mv "$name" ./data/experiments ;
   yes 'm' | ./run_experiments.sh -- 3 4 5 || { echo "Error. Exiting." ; mv ./data/experiments "$name" ; exit 1; } ;
   mv ./data/experiments "$name" ;
done ;
