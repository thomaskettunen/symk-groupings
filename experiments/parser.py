import re
from lab.parser import Parser
from enum import Enum

import os

class ExitCode(Enum):
    FOUND_K = 1
    FOUND_ALL = 2
    OUT_OF_MEM = 4
    OUT_OF_TIME = 8
    OTHER_ERROR = 16

"""
    TODO:
        things to get done average plan time at the time of non dominated plan found:
		- should_be_top5 er suspicious
        - A better way to get average time spent
        - why is k just set equal to 5 in here xd
"""

def error(content, props):
    props["error"] = not content == ''

def exit_code(content, props):
    props["exit code"] = ExitCode.OTHER_ERROR

    if(re.search(f"Completed search space no more plans in the open list",content) != None):
        props["exit code"] = ExitCode.FOUND_ALL


def total_time(content, props):
    if not props["error"]:
        try:
            props["total time"] = float(re.search(r"Total time: (\d+\.\d+)s", content).group(1))
        except:
           props["total time"] = 600


def coverage(content, props):

    print("the search", re.search(f"The search has ended",content))

    if (re.search(f"The search has ended",content) != None):
        props["coverage"] = 1
    else:
        props["coverage"] = 0


def plans_found(content, props):
    if not props["error"]:
        matches = re.findall(r"found non dominated plan", content)
        props["plans found"] = len(matches)


def last_plan_time(content, props):
    if not props["error"]:
        matches = re.findall(r"time spent on step: (\d+\.\d+)", content)

        if len(matches) > 0:
            props["last plan time_mean"] = float(matches[-1])
            props["last plan time_min"] = float(matches[-1])
            props["last plan time_max"] = float(matches[-1])
        else:
            props["last plan time_mean"] = None
            props["last plan time_min"] = None
            props["last plan time_max"] = None


def should_be_top5(content, props):
    if not props["error"]:
        plan_count = re.search("Number of plans: (\d+)", content)

        if plan_count == None :
            props["over_k"] = False
            return
        
        plan_count = int(plan_count.group(1))

        if plan_count > 0:
            time = float(re.search("Total time: (\d+\.\d+)", content).group(1))

            props["exit code"] = ExitCode.FOUND_K.value
            props["total time"] = time
            props["coverage"] = 1
            props["plans found"] = plan_count
            props["last plan time_mean"] = time
            props["last plan time_min"] = time
            props["last plan time_max"] = time

def to_str(content, props):
    if not props["error"]:
        props["exit code"] = f'{ExitCode(props["exit code"])}' #. enum name. This will intentionally error on unknown exit codes
    props["error"] = str(props["error"])

class FIParser(Parser):
    def __init__(self):
        super().__init__()
        self.add_function(error, file = 'run.err') #. keep first
        self.add_function(should_be_top5)  #. keep second
        self.add_function(exit_code)
        self.add_function(total_time)
        self.add_function(coverage)
        self.add_function(plans_found)
        self.add_function(last_plan_time)
        self.add_function(to_str) #. keep last, stringyfies thigs that shoulnd't be summed