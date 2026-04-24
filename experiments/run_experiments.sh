#!/bin/bash

# $1 planner
# $2 BENCHMARKS
# $3 number of plans (k)

# example:
# ./run_experiments.sh ../fast-downward.py ../../benchmarks 2 symk_bd -- --all

PLANNER=""
PLANNER_FLAG="--planner"
PLANNER_DEFAULT="../fast-downward.py"

BENCHMARKS=""
BENCHMARKS_FLAG="--benchmarks"
BENCHMARKS_DEFAULT="../../benchmarks"

K=""
K_FLAG="--k"
K_DEFAULT="5"

while [ "$#" -ge 1 ] && [ "$1" != -- ]; do
    if [ "$#" -ge 1 ] && [[ "$1" == --* ]]; then #. key-value args
        if [ "$#" -ge 1 ] && [ "$1" == "$PLANNER_FLAG" ]; then
            shift;
            if [ "$#" -ge 1 ] && [ "$1" != -- ]; then
                PLANNER="$1"; shift;
                continue;
            else
                echo "$PLANNER_FLAG expects a path";
                exit -1;
            fi
        fi

        if [ "$#" -ge 1 ] && [ "$1" == "$BENCHMARKS_FLAG" ]; then
            shift;
            if [ "$#" -ge 1 ] && [ "$1" != -- ]; then
                BENCHMARKS="$1"; shift;
                continue;
            else
                echo "$BENCHMARKS_FLAG expects a path";
                exit -1;
            fi
        fi

        if [ "$#" -ge 1 ] && [ "$1" == "$K_FLAG" ]; then
            shift;
            if [ "$#" -ge 1 ] && [ "$1" != -- ]; then
                K="$1"; shift;
                continue;
            else
                echo "$K_FLAG expects a num";
                exit -1;
            fi
        fi

        echo "Unknown option: \"$1\"";
        exit -1;
    else #. positional args
        if [ "$PLANNER" == "" ] && [ "$#" -ge 1 ] && [ "$1" != -- ]; then
            PLANNER="$1"; shift;
            continue;
        fi

        if [ "$BENCHMARKS" == "" ] && [ "$#" -ge 1 ] && [ "$1" != -- ]; then
            BENCHMARKS="$1"; shift;
            continue;
        fi

        if [ "$K" == "" ] && [ "$#" -ge 1 ] && [ "$1" != -- ]; then
            K="$1"; shift;
            continue;
        fi

        echo "Unknown positional arg: \"$1\"";
        exit -1;
    fi
done

#. defaults
if [ "$PLANNER" == "" ]; then PLANNER="$PLANNER_DEFAULT"; fi
if [ "$BENCHMARKS" == "" ]; then BENCHMARKS="$BENCHMARKS_DEFAULT"; fi
if [ "$K" == "" ]; then K="$K_DEFAULT"; fi

export PLANNER=$(readlink -f "$PLANNER")
echo "exporting PLANNER=$(readlink -f "$PLANNER")"
export DOWNWARD_BENCHMARKS=$(readlink -f "$BENCHMARKS")
echo "exporting DOWNWARD_BENCHMARKS=$(readlink -f "$BENCHMARKS")"
export K="$K"
echo "exporting K="$K""

if [ "$1" == -- ]; then shift; fi

./experiments.py "$@"
