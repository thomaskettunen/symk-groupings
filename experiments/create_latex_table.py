import json
import os
import sys
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
  construct_error_code_table(results)

  evaluations = json.loads(open("./data/experiments-eval/properties", 'r').read())

  for experiment in experiments:
    flatten_experiments(evaluations, experiment)

def flatten_experiments(evaluations, algorithm):

  rows = []
  for key in evaluations:
    if(evaluations[key]["algorithm"] == algorithm):
      rows.append(evaluations[key])

  columns = [
    ("problem", lambda row: f"{row["domain"]} {row["problem"]}"),
    ("exit code", lambda row: row["exit_code"].replace("ExitCode.", "")),
    ("error", lambda row: row["error"]),
    ("total time", lambda row: row["total_time"]),
    ("plans found", lambda row: row["plans_found"]),
    ("last plan", lambda row: row["last_plan_time_max"])
  ]

  table = """
    <style>
      *{
        margin: 0px;
        padding: 0px;
        box-sizing: border-box;
        font-family: sans-serif;
      }
      table{
        text-align:center;
        border-spacing:0px;
      }
      td{
        border:solid black 2px;
        padding: 10px;
        padding-top: 2px;
        padding-bottom: 2px;
      }
    </style>
  """

  table += "<table>"
  table += "<tr>"
  for column, _ in columns:
    table += f"<td>{column}</td>"
  table += "</tr>"

  colors = ["FFFFBB", "FFFFFF"]
  i = 0
  for row in rows:
    table += f"<tr style=\"background-color:{colors[i]}\">"
    for _, column in columns:
      table += f"<td>{column(row)}</td>"
    table += "</tr>"

    i = (i + 1) % 2

  table += "</table>"

  output_file = open(f"./latex/flattened_{algorithm}.html", 'w')
  output_file.write(table)
  output_file.close()


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

def construct_error_code_table(json_object):
  table = f"\\begin{{tabular}}{{|c|{"|".join(["c" for _ in json_object])}|}}\n\\hline\n"

  table += f"&{"&".join(json_object.keys()).replace("_","\_")} \\\\\n"
  table += "\\hline\\hline\n"
  table += f"FOUND\_ALL & {"&".join([str(json_object[key]["found_all"]) for key in json_object])} \\\\\n"
  table += "\\hline\n"
  table += f"FOUND\_K & {"&".join([str(json_object[key]["found_k"]) for key in json_object])} \\\\\n"
  table += "\\hline\n"
  table += f"OTHER\_ERROR & {"&".join([str(json_object[key]["other_error"]) for key in json_object])} \\\\\n"
  table += "\\hline\n"
  table += f"OUT\_OF\_MEM & {"&".join([str(json_object[key]["out_of_memory"]) for key in json_object])} \\\\\n"
  table += "\\hline\n"
  table += f"OUT\_OF\_TIME & {"&".join([str(json_object[key]["out_of_time"]) for key in json_object])} \\\\\n"
  table += "\\hline\n"
  table += f"OUT\_OF\_SPACE & {"&".join([str(json_object[key]["out_of_space"]) for key in json_object])} \\\\\n"
  table += "\\hline\n"
  table += f"OUT\_OF\_PREPROCESS\_ERR & {"&".join([str(json_object[key]["preprocess_error"]) for key in json_object])} \\\\\n"
  table += "\\hline\n"
  table += f"TRANSLATE\_OUT\_OF\_MEM & {"&".join([str(json_object[key]["out_of_memory"]) for key in json_object])} \\\\\n"
  table += "\\hline\n"
  table += "\\end{tabular}"

  output_file = open("./latex/summation_of_exit_codes_table.txt", 'w')
  output_file.write(table)
  output_file.close()

if __name__ := "__main__":
  main()