#! /usr/bin/env python

"""
Experiment for the symk-groups planner
"""

import os
import sys

from downward import suites
from downward.reports.absolute import AbsoluteReport
from lab.environments import LocalEnvironment
from lab.experiment import Experiment
from lab.reports import Attribute, geometric_mean
from deis_mcc import DEISSlurmEnvironment

def mean(a):
    return sum(a) / len(a)

# Create custom report class with suitable info and error attributes.
class BaseReport(AbsoluteReport):
    INFO_ATTRIBUTES = ["time_limit", "memory_limit", "k"]
    ERROR_ATTRIBUTES = [
        "domain",
        "problem",
        "algorithm",
        "unexplained_errors",
        "error",
    ]

benchmarks = [
    "airport",
    # "barman-opt11-strips",
    # "barman-opt14-strips",
    "blocks",
    # "childsnack-opt14-strips",
    "depot",
    "driverlog",
    # "elevators-opt08-strips",
    # "elevators-opt11-strips",
    # "floortile-opt11-strips",
    # "floortile-opt14-strips",
    # "freecell",
    # "ged-opt14-strips",
    # "grid",
    # "gripper",
    # "hiking-opt14-strips",
    "logistics00",
    # "logistics98",
    # "miconic",
    # "movie",
    # "mprime",
    # "mystery",
    # "nomystery-opt11-strips",
    # "openstacks-opt08-strips",
    # "openstacks-opt11-strips",
    # "openstacks-opt14-strips",
    # "openstacks-strips",
    # "organic-synthesis-opt18-strips",
    # "organic-synthesis-split-opt18-strips",
    "parcprinter-08-strips",
    # "parcprinter-opt11-strips",
    # "parking-opt11-strips",
    # "parking-opt14-strips",
    # "pathways",
    # "pegsol-08-strips",
    "pegsol-opt11-strips",
    # "petri-net-alignment-opt18-strips",
    # "pipesworld-notankage",
    # "pipesworld-tankage",
    # "psr-small",
    # "quantum-layout-opt23-strips",
    # "rovers",
    # "satellite",
    # "scanalyzer-08-strips",
    # "scanalyzer-opt11-strips",
    # "snake-opt18-strips",
    # "sokoban-opt08-strips",
    # "sokoban-opt11-strips",
    # "spider-opt18-strips",
    # "storage",
    # "termes-opt18-strips",
    # "tetris-opt14-strips",
    # "tidybot-opt11-strips",
    # "tidybot-opt14-strips",
    # "tpp",
    # "transport-opt08-strips",
    # "transport-opt11-strips",
    # "transport-opt14-strips",
    # "trucks-strips",
    # "visitall-opt11-strips",
    # "visitall-opt14-strips",
    # "zenotravel",
]

if DEISSlurmEnvironment.is_cluster():
    ENV = DEISSlurmEnvironment(partition='dhabi')
else:
    ENV = LocalEnvironment(processes=9)
SUITE = benchmarks
ATTRIBUTES = [
    "error",
    Attribute("exit code"),
    Attribute("total time", min_wins=True),
    Attribute("coverage", min_wins=False, scale="linear"),
    Attribute("plans found", min_wins=False),
    Attribute("last plan time_mean", min_wins=True, function=mean),
    Attribute("last plan time_min", min_wins=True, function=min),
    Attribute("last plan time_max", min_wins=True, function=max),
]
TIME_LIMIT = 3600
MEMORY_LIMIT = 2048

# Create a new experiment.
exp = Experiment(environment=ENV)

from parser import FIParser
exp.add_parser(FIParser())

for task in suites.build_suite(os.environ['DOWNWARD_BENCHMARKS'], SUITE):
    for search, grouping in [(search, grouping) for search in ["symk_fw", "symk_bd"] for grouping in ["prefix(1)"]]:
        run = exp.add_run()
        run.add_resource("domain", task.domain_file, symlink=True)
        run.add_resource("problem", task.problem_file, symlink=True)
        run.add_command(
            "run-planner",
            [sys.executable, os.environ['PLANNER'], "--build", "symbolic", "{domain}", "{problem}", "--search", f"{search}(silent=true,k={os.environ['K']},grouping={grouping},max_time={TIME_LIMIT})"],
            time_limit=TIME_LIMIT,
            memory_limit=MEMORY_LIMIT,
        )
        # AbsoluteReport needs the following properties:
        # 'domain', 'problem', 'algorithm', 'coverage'.
        run.set_property("domain", task.domain)
        run.set_property("problem", task.problem)
        run.set_property("algorithm", f'{search}')
        run.set_property("grouping", f'{grouping}')
        # BaseReport needs the following properties:
        # 'time_limit', 'memory_limit'.
        run.set_property("time_limit", TIME_LIMIT)
        run.set_property("memory_limit", MEMORY_LIMIT)
        run.set_property("k", f"{os.environ['K']}")
        # Every run has to have a unique id in the form of a list.
        # The algorithm name is only really needed when there are
        # multiple algorithms.
        run.set_property("id", [f"{search}", f"{grouping}", f"{os.environ['K']}", f"{TIME_LIMIT}", f"{MEMORY_LIMIT}", task.domain, task.problem])

# Add step that writes experiment files to disk.
exp.add_step("build", exp.build)

# Add step that executes all runs.
exp.add_step("start", exp.start_runs)

# Add step that parses log output into "properties" files.
exp.add_step("parse", exp.parse)

# Add step that collects properties from run directories and
# writes them to *-eval/properties.
exp.add_fetcher(name="fetch")

# Make a report.
exp.add_report(
    BaseReport(
        attributes=ATTRIBUTES,
        filter = lambda data:
            data['domain'] not in ["agricola-opt18-strips", "data-network-opt18-strips", "woodworking-opt08-strips", "woodworking-opt11-strips"]
            and not (data['domain'] == 'organic-synthesis-split-opt18-strips' and data['problem'] == 'p08.pddl')
    ),
    outfile="report.html"
)

# Parse the commandline and run the specified steps.
exp.run_steps()
