#!/bin/bash

# $1 planner
# $2 BENCHMARKS
# $3 number of plans (k)

PLANNER=""
PLANNER_FLAG="--planner"
PLANNER_DEFAULT="../plan_supermultisetgroups_topk.sh"

BENCHMARKS=""
BENCHMARKS_FLAG="--benchmarks"
BENCHMARKS_DEFAULT="../../benchmarks"

K=""
K_FLAG="--k"
K_DEFAULT="5"

while [ "$#" -ge 1 ] && [ "$1" != "--" ]; do
    echo
    echo "$#"
    echo "$1"
    if [ "$#" -ge 1 ] && [ "$1" == "--*" ]; then #. key-value args
        if [ "$#" -ge 1 ] && [ "$1" == "$PLANNER_FLAG" ]; then
            shift;
            if [ "$#" -ge 1 ] && [ "$1" != "--" ]; then
                PLANNER="$1"; shift;
            else
                echo "--planner expects a path"
            fi
        fi

        if [ "$#" -ge 1 ] && [ "$1" == "$BENCHMARKS_FLAG" ]; then
            shift;
            if [ "$#" -ge 1 ] && [ "$1" != "--" ]; then
                echo path1.2.1
                BENCHMARKS="$1"; shift;
            else
                echo "--benchmark expects a path"
            fi
        fi

        if [ "$#" -ge 1 ] && [ "$1" == "$K_FLAG" ]; then
            shift;
            if [ "$#" -ge 1 ] && [ "$1" != "--" ]; then
                K="$1"; shift;
            else
                echo "--k expects a num"
            fi
        fi
    else #. positional args
        if [ "$PLANNER" == "" ] && [ "$#" -ge 1 ] && [ "$1" != "--" ]; then
            PLANNER="$1"; shift;
        fi

        if [ "$BENCHMARKS" == "" ] && [ "$#" -ge 1 ] && [ "$1" != "--" ]; then
            BENCHMARKS="$1"; shift;
        fi

        if [ "$K" == "" ] && [ "$#" -ge 1 ] && [ "$1" != "--" ]; then
            K="$1"; shift;
        fi
    fi
done

#. defaults
if [ "$PLANNER" == "" ]; then PLANNER="$PLANNER_DEFAULT"; fi
if [ "$BENCHMARKS" == "" ]; then BENCHMARKS="$BENCHMARKS_DEFAULT"; fi
if [ "$K" == "" ]; then K="$K_DEFAULT"; fi

export PLANNER=$(readlink -f "$PLANNER")
export DOWNWARD_BENCHMARKS=$(readlink -f "$BENCHMARKS")
export K="$K"

if [ "$1" == "--" ]; then shift; fi

uv run ./experiments.py "$@"
