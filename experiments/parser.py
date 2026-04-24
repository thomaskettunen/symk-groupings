import re
from lab.parser import Parser
from enum import Enum
import os

from enum import Enum

class ExitCode(Enum):
    FOUND_K = 0
    FOUND_ALL = 12
    OUT_OF_MEM = 22
    OUT_OF_TIME = 23
    OTHER_ERROR = 9999999

def set_err_false(content, props):
    props["error"] = False

def error_if_present(content, props):
    props["error"] = props["error"] or not content == ''

def error_if_missing(content, props):
    props["error"] = props["error"] or content == ''

def exit_code(content, props):
    if(props["error"]): return
    solution_found = re.search(f"Solution found", content)
    if(solution_found):
        completed_search = re.search(f"Completed search, open list empty", content)

        if(completed_search):
            props["exit code"] = ExitCode.FOUND_ALL.value
        else:
            props["exit code"] = ExitCode.FOUND_K.value
        return

    time_limit_reached = re.search(f"Time limit reached. Abort search", content)
    if(time_limit_reached):
        props["exit code"] = ExitCode.OUT_OF_TIME.value
        return

    props["exit code"] = ExitCode.OTHER_ERROR.value

def coverage(content, props):
    if props["error"]: return
    match ExitCode(props["exit code"]):
        case ExitCode.FOUND_K | ExitCode.FOUND_ALL:
            props["coverage"] = 1
        case _:
            props["coverage"] = 0

def total_time(content, props):
    if props["error"]: return
    try:
        props["total time"] = float(re.search(r"Total time: (\d+\.\d+)s", content).group(1))
    except:
       props["total time"] = 600


def plans_found(content, props):
    if props["error"]: return
    matches = re.findall(r"Found plan \[(\d+)/\d\]", content)
    if len(matches) > 0:
        props["plans found"] = int(matches[-1])
    else:
        props["plans found"] = 0

def last_plan_time(content, props):
    if props["error"]: return
    matches = re.findall(r"\[t=(\d+\.\d+)s, \d+ KB\] Found plan \[\d+/\d\]", content)
    if len(matches) > 0:
        props["last plan time_mean"] = float(matches[-1])
        props["last plan time_min"] = float(matches[-1])
        props["last plan time_max"] = float(matches[-1])
    else:
        props["last plan time_mean"] = None
        props["last plan time_min"] = None
        props["last plan time_max"] = None



def to_str(content, props):
    error = props["error"]
    props["error"] = str(props["error"])
    if error: return
    props["exit code"] = f'{ExitCode(props["exit code"])}' #. enum name. This will intentionally error on unknown exit codes

def revision(content, props):
    if(props["error"]): return
    props["revision"] = re.findall("Search code revision: (\w*(?:\-dirty)?)" , content)[0];

class FIParser(Parser):
    def __init__(self):
        super().__init__()
        self.add_function(set_err_false) #. keep first
        self.add_function(error_if_present, file = 'run.err') #. keep first
        self.add_function(error_if_missing, file = 'driver.log') #. keep first (ignores unstarted runs)
        self.add_function(exit_code)
        self.add_function(coverage)
        self.add_function(total_time)
        self.add_function(plans_found)
        self.add_function(last_plan_time)
        self.add_function(revision)
        self.add_function(to_str) #. keep last, stringyfies thigs that shoulnd't be summed