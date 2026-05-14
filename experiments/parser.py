import re
from lab.parser import Parser
from enum import Enum
import os
import json

from enum import Enum

class ExitCode(Enum):
    KILLED = -15
    FOUND_K = 0
    FOUND_ALL = 12
    TRANSLATE_OUT_OF_MEM = 20
    OUT_OF_MEM = 22
    OUT_OF_TIME = 23
    PREPROCESS_ERR = 67
    OTHER_TRANSLATE_ERR = 1337
    OUT_OF_SPACE = 69420
    OTHER_ERROR = 9999999

def to_string(props):
    props["error"] = str(props["error"])
    try:
        props["exit_code"] = f'{ExitCode(props["exit_code"])}'
    except KeyError:
        props["exit_code"] = f'{ExitCode.OTHER_ERROR}'


def parse(get_content, props):
    if (static_properties := get_content("static-properties")): props.update(json.loads(static_properties))
    else: raise RuntimeError(f"No static properties")

    if (bool(get_content("run.err"))):
        props["error"] = True
        props["coverage"] = 0
        props["plans_found"] = 0
        props["last_plan_time_mean"] = None
        props["last_plan_time_min"] = None
        props["last_plan_time_max"] = None
        props["total_time"] = props["time_limit"]
        return to_string(props)

    else: props["error"] = False #. If there's a non-empty run.err, we had an error

    if not (driver_log := get_content("driver.log")): raise RuntimeError(f"No driver.log, run wasn't started")

    if not (run_log := get_content("run.log")): raise RuntimeError(f"No run.log???")
    if (search := props.get("search")) not in ["forbiditer", "symk_fw", "symk_bw", "symk_bd"]: raise RuntimeError(f"Cannot parse unknown search: {search}")

    print(props)
    print(f'data/{search if search == "forbiditer" else "symk"}.{props["grouping"]}.runs/{props["run_dir"]}')

    ## Exit code
    if (exit_code_match := re.search(r"run-planner exit code: (-?\d+)", driver_log)):
        props["exit_code"] = int(exit_code_match.group(1))
        if ExitCode(props["exit_code"]) == ExitCode.KILLED:
            if re.search(r"run-planner exceeded CPU time limit", driver_log):
                props["exit_code"] = ExitCode.OUT_OF_TIME.value
            # elif re.search(r"", driver_log): # TODO: Figure out what it writes if it goes OOM and get's killed
            #     props["exit_code"] = ExitCode.OUT_OF_MEM.value
            elif re.search(r"run-planner wrote \d+\.\d+ KiB \(hard limit\) to \w+\.\w+ -> abort command", get_content("driver.err") or ''):
                props["exit_code"] = ExitCode.OUT_OF_SPACE.value
            else:
                raise RuntimeError("Killed for suspicious reason")
        elif (translate_exit_code := int(re.search(r"translate exit code: (-?\d+)", run_log).group(1))) != 0:
            match translate_exit_code:
                case 20: props["exit_code"] = ExitCode.TRANSLATE_OUT_OF_MEM.value
                case x: props["exit_code"] = ExitCode.OTHER_TRANSLATE_ERR.value
        elif search.startswith("symk") and ((not (preprocess_exit_code_match := re.search(r"preprocess exit code: (-?\d+)", run_log))) or int(preprocess_exit_code_match.group(1)) != 0):
            props["exit_code"] = ExitCode.PREPROCESS_ERR.value
        elif search == "forbiditer" and ((not (preprocess_exit_code_match := re.search(r"transform_task exit code: (-?\d+)", run_log))) or int(preprocess_exit_code_match.group(1)) != 0):
            props["exit_code"] = ExitCode.PREPROCESS_ERR.value
        elif((not (search_exit_code_match := re.search(r"search exit code: (-?\d+)", run_log)))):
            props["exit_code"] = ExitCode.OTHER_ERROR.value
        elif search.startswith("symk"):
            props["exit_code"] = int(search_exit_code_match.group(1))
            if(props["exit_code"] == 0):
                if(re.search(f"Completed search, open list empty", run_log)):
                    props["exit_code"] = ExitCode.FOUND_ALL.value
                else:
                    props["exit_code"] = ExitCode.FOUND_K.value
        elif search == "forbiditer":
            search_exit_code_matches = re.findall(r"search exit code: (\d+)", run_log)
            external_planners_started = len(re.findall(r"Running search", run_log))
            external_planners_done = len(re.findall(r"search exit code: (\d+)", run_log))
            if(external_planners_started > external_planners_done): # If more searches were started than done assume timeout
                props['exit_code'] = ExitCode.OUT_OF_TIME.value
            elif len(search_exit_code_matches) > 0: #. if trannslate sttep aborts, we don't even get one search exit code
                props["exit_code"] = int(search_exit_code_matches[-1])
        else:
            raise RuntimeError("huge herror when parsing exit codes")
    else:
        props["exit_code"] = ExitCode.OTHER_ERROR.value
        props["error"] = True
        raise RuntimeError(f"No exit code in driver.log")

    ## Plans found + Last plan time
    props["plans_found"] = 0
    props["last_plan_time_mean"] = None
    props["last_plan_time_min"] = None
    props["last_plan_time_max"] = None
    match search:
        case "symk_fw" | "symk_bw" | "symk_bd":
            if len(found_plan_match := re.findall(r"\[t=(\d+\.\d+)s, \d+ KB\] Found plan \[(\d+)/\d+\]", run_log)) > 0:
                for i, match in enumerate(found_plan_match):
                    i = i+1
                    if i != int(match[1]): raise RuntimeError(f"{i} != {int(match[1])}")
                    props[f"plan_{i}_time"] = float(match[0])
                props["last_plan_time_mean"] = float(found_plan_match[-1][0])
                props["last_plan_time_min"] = float(found_plan_match[-1][0])
                props["last_plan_time_max"] = float(found_plan_match[-1][0])
                props["plans_found"] = int(found_plan_match[-1][1])
        case "forbiditer":
            if len(found_plan_match := re.findall(r"Iteration step \d+ is done, found (\d+) plans, time \[(\d+\.\d+)s CPU, \d+\.\d+s wall-clock\]", run_log)) > 0:
                for i, match in enumerate(found_plan_match):
                    i = i+1
                    if i != int(match[0]): raise RuntimeError(f"{i} != {int(match[0])}")
                    props[f"plan_{i}_time"] = float(match[1])
                props["last_plan_time_mean"] = float(found_plan_match[-1][1])
                props["last_plan_time_min"] = float(found_plan_match[-1][1])
                props["last_plan_time_max"] = float(found_plan_match[-1][1])
                props["plans_found"] = int(found_plan_match[-1][0])
        case other: raise RuntimeError(f"Unknown search {other}")

    ## Coverage
    match ExitCode(props["exit_code"]):
        case ExitCode.FOUND_K | ExitCode.FOUND_ALL:
            props["coverage"] = 1
        case _:
            props["coverage"] = 0

    ## Revision
    # if props["coverage"]:
    #     match search:
    #         case search.startswith("symk"):
    #             if (revision_match := re.search("Search code revision: (\w*(?:\-dirty)?)" , run_log)): props["revision"] = revision_match.group(1)
    #             else: raise RuntimeError(f"No revision")
    #         case "forbiditer":
    #             print(props)
    #             if (revision_match := re.search("DOWNWARDS GIT SHA: (\w*(?:\-dirty)?)" , run_log)): props["revision"] = revision_match.group(1)
    #             else: raise RuntimeError(f"No revision")
    #         case other: raise RuntimeError(f"Unknown search {other}")

    ## Toal time
    try:
        match search:
            case "symk_fw" | "symk_bw" | "symk_bd":
                props["total_time"] = float(re.search(r"Total time: (\d+\.\d+)s", run_log).group(1))
            case "forbiditer":
                props["total_time"] = float(re.search(r"All iterations are done \[\d+\.\d+s CPU, (\d+\.\d+)s wall-clock\]", run_log).group(1))
            case other: raise RuntimeError(f"Unknown search {other}")
    except:
       props["total_time"] = props["time_limit"]

    return to_string(props)

class FIParser(Parser):
    def __init__(self):
        super().__init__()
        self.add_function(parse)