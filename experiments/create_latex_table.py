import json
import os
import argparse
import re

def main():
  experiments = ["forbiditer-prefix(1)", "forbiditer-prefix(2)", "symk_bd-prefix(1)", "symk_bd-prefix(2)", "symk_fw-prefix(1)", "symk_fw-prefix(2)"]

  results = {}
  for experiment in experiments:
    evaluations = open("./data/experiments-eval/properties", 'r')
    evaluations_as_plain_text = evaluations.read()
    evaluations.close()

    results[experiment] = json.loads('{"coverage":0, "error-False":0, "error-True":0, "last_plan_time_max":0, "last_plan_time_mean":0, "last_plan_time_min":0,"plans_found":0,"total_time":0,"found_all":0,"found_k":0,"other_error":0,"out_of_memory":0,"out_of_space":0,"out_of_time":0,"preprocess_error":0,"translate_out_of_memory":0}')
    
    runs = re.findall(f"{experiment.replace("(", "\(").replace(")", "\)")}"+".*?(\{.*?\})", evaluations_as_plain_text, re.DOTALL)
    
    for run in runs:
      run = json.loads(run)
      results[experiment]["coverage"] += 0 if run["coverage"] is None else run["coverage"]
      results[experiment]["plans_found"] += 0 if run["plans_found"] is None else run["plans_found"]
      results[experiment]["last_plan_time_max"] += 0 if run["last_plan_time_max"] is None else run["last_plan_time_max"]
      results[experiment]["last_plan_time_min"] += 0 if run["last_plan_time_min"] is None else run["last_plan_time_min"]
      results[experiment]["last_plan_time_mean"] += 0 if run["last_plan_time_mean"] is None else run["last_plan_time_max"]
      results[experiment]["total_time"] += 0 if run["total_time"] is None else run["total_time"]
      # handle error codes
      match run["exit_code"]:
        case "ExitCode.FOUND_ALL":
          results[experiment]["found_all"] += 1
        case "ExitCode.FOUND_K":
          results[experiment]["found_k"] += 1
        case "ExitCode.OTHER_ERROR":
          results[experiment]["other_error"] += 1
        case "ExitCode.OUT_OF_MEM": 
          results[experiment]["out_of_memory"] += 1
        case "ExitCode.OUT_OF_TIME":
          results[experiment]["out_of_time"] += 1
        case "ExitCode.OUT_OF_SPACE":
          results[experiment]["out_of_space"] += 1
        case "ExitCode.OUT_OF_PREPROCESS_ERR":
          results[experiment]["preprocess_error"] += 1
        case "ExitCode.TRANSLATE_OUT_OF_MEM":
          results[experiment]["out_of_memory"] += 1

  construct_coverage_time_found_table(results)

def construct_coverage_time_found_table(json_object):
  table = f"\\begin{{tabular}}{{|c|{"|".join(["c" for _ in json_object])}|}}\n\\hline\n"

  table += f"&{"&".join(json_object.keys()).replace("_","\_")} \\\\\n"

  table += "\\hline\\hline\n"

  table += f"coverage & {"&".join([str(json_object[key]["coverage"]) for key in json_object])} \\\\\n"

  table += "\\hline\n"

  table += f"total time & {"&".join([str(int(json_object[key]["total_time"])) for key in json_object])} \\\\\n"

  table += "\\hline\n"

  table += f"plans found & {"&".join([str(json_object[key]["plans_found"]) for key in json_object])} \\\\\n"

  table += "\\hline\n"

  table += "\\end{tabular}"

  output_file = open("./latex/summation_table.txt", 'w')
  output_file.write(table)
  output_file.close()

def construct_error_code_table(object):
  pass

if __name__ := "__main__":
  main()