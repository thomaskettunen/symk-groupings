import re
from lab.parser import Parser
from enum import Enum

class ExitCode(Enum):
    FOUND_K = 0
    FOUND_ALL = 12
    OUT_OF_MEM = 22
    OUT_OF_TIME = 23
    OTHER_ERROR = 9999999

k = 5

def error(content, props):
    props["error"] = not content == ''

def exit_code(content, props):
    if not props["error"] and not props["found_k"]:
        matches = re.findall(r"search exit code: (\d+)", content)

        external_planners_started = len(re.findall(r"Running search", content))
        external_planners_done = len(re.findall(r"search exit code: (\d+)", content))

        if(external_planners_started > external_planners_done): # If more searches were started than done assume timeout
            props['exit code'] = ExitCode.OUT_OF_TIME.value
        elif len(matches) > 0: #. if trannslate sttep aborts, we don't even get one search exit code
            props["exit code"] = int(matches[-1])
        else:
            props["exit code"] = ExitCode.OTHER_ERROR.value #. Fake exit code to indicate this branch

def total_time(content, props):
    if not props["error"] and not props["found_k"]:
        try:
            props["total time"] = float(re.search(r"All iterations are done \[\d+\.\d+s CPU, (\d+\.\d+)s wall-clock\]", content).group(1))
        except:
           props["total time"] = 600

def coverage(content, props):
    if not props["error"] and not props["found_k"]:
        match ExitCode(props["exit code"]):
            case ExitCode.FOUND_K | ExitCode.FOUND_ALL:
                props["coverage"] = 1
            case _:
                props["coverage"] = 0


def plans_found(content, props):
    if not props["error"] and not props["found_k"]:
        matches = re.findall(r"found (\d+) plans", content)
        if len(matches) > 0:
            props["plans found"] = int(matches[-1])
        else:
            props["plans found"] = 0


def last_plan_time(content, props):
    if not props["error"] and not props["found_k"]:
        #. Last time it says "Iteration step \d+ is done, found \d+ plans" is the last time it finds plans
        matches = re.findall(r"Iteration step \d+ is done, found \d+ plans, time \[\d+\.\d+s CPU, (\d+\.\d+)s wall-clock\]", content)
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
        matches = re.findall(f"Iteration step {k} is done, found {k} plans, time \\[\\d+\\.\\d+s CPU, (\\d+\\.\\d+)s wall-clock\\]", content)
        props["over_k"] = False
        props["found_k"] = False
        if len(matches) > 0:
            props["found_k"] = True
            if len(re.findall(f"Iteration step {k+1}, time limit", content)) > 0: # SKriver den når den starter den næste
                props["over_k"] = True
            time = float(matches[-1])
            props["exit code"] = ExitCode.FOUND_K.value
            props["total time"] = time
            props["coverage"] = 1
            props["plans found"] = k
            props["last plan time_mean"] = time
            props["last plan time_min"] = time
            props["last plan time_max"] = time

def to_str(content, props):
    if not props["error"]:
        props["exit code"] = f'{ExitCode(props["exit code"])}' #. enum name. This will intentionally error on unknown exit codes
        props["over_k"] = int(props["over_k"])
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