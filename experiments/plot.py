#! /usr/bin/env python

import json
import os
import sys
import re
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from pathlib import Path
import matplotlib.colors as mcolors
import math
import importlib.util

if importlib.util.find_spec("tqdm") is not None:
  from tqdm import tqdm
  print = tqdm.write
else:
  print("install 'tqdm' (pip install tqdm) to show progress bars")
  class tqdm:
    def __init__(self, *args, **kwargs):
      if len(args) > 0:
        self.iter = args[0]

    def __enter__(self):
      return self

    def __exit__(self, exc_type, exc, tb):
      return False

    def __iter__(self):
      return self.iter.__iter__()

    def update(selg, *args):
      pass

color_set = plt.cm.tab10.colors

def multiply(color, amount):
    return tuple(max(0, min(1, (x * amount))) for x in mcolors.to_rgb(color))

def ensure_dir(path):
  p = Path(path)
  p.mkdir(parents=True, exist_ok=True)
  return p

class X:
  def __init__(self, data):
    self.key = data[0]
    self.value = data[1]

  def __getitem__(self, key):
    return self.value.get(key, None)

  def __setitem__(self, key, value):
    self.value[key] = value

  def __str__(self):
    return f"{self.key}: {self.value}"

  def has(self, dict):
    for key in dict:
      if key in self.value.keys() and self.value[key] == dict[key]: continue
      elif callable(dict[key]) and dict[key](self.value[key]): continue
      elif ("__iter__" in dir(dict[key])) and self.value[key] in dict[key]: continue
      else: return False
    return True

def latex_table(data):
  return rf"""\begin{{adjustbox}}{{width=\columnwidth,center}}
  \begin{{tabular}}{{|{"|".join(["c" for _ in data[0]])}|}}
    \hline
    {" \\\\\\hline\n  ".join([" & ".join([f"{cell}".replace("_", " ") for cell in row]) for row in data])} \\\hline
  \end{{tabular}}
\end{{adjustbox}}
"""

def main():
  print("Starting")
  plot_dir = ensure_dir("plots")

  with open("./data/experiments-eval/properties", 'r') as f:
    properties = json.loads(f.read())
  print(" - loading from disk")
  unfiltered_runs = [X((key, properties[key])) for key in tqdm(properties)]

  print(" - generating filter")
  run_filter = [
    {"domain": "airport", "problem": "p35-airport4halfMUC-p12.pddl", "grouping": "prefix(1)"}, # One time preprocess gave up, so different #Groups
    *[{"domain": domain, "problem": problem, "grouping": grouping} for domain, problem, grouping in set([(run["domain"], run["problem"], run["grouping"]) for run in tqdm(unfiltered_runs) if (
      run.has({"error": 1})
      or run.has({"exit_code": ["ExitCode.OTHER_ERROR", "ExitCode.PREPROCESS_ERR", "ExitCode.OUT_OF_SPACE", "ExitCode.TRANSLATE_OUT_OF_MEM"]})
    )])]
  ]

  print("Filter:")
  for filtered in run_filter:
    print(f"\t{filtered}")

  print(" - filtering data")
  all_runs = [run for run in tqdm(unfiltered_runs) if not any([run.has(filtered) for filtered in run_filter])]

  print(" - preprocessing")
  runs = {}
  for run in tqdm(all_runs):
    runs[(run["search"], run["grouping"])] = runs.get((run["search"], run["grouping"]), []) + [run]

  runs_T = {}
  for run in tqdm(all_runs):
    runs_T[(run["domain"], run["problem"], run["grouping"])] = runs_T.get((run["domain"], run["problem"], run["grouping"]), {})
    runs_T[(run["domain"], run["problem"], run["grouping"])][run["search"]] = run

  searches, groupings = map(set, zip(*runs.keys()))
  hues = {s: color_set[i % len(color_set)] for (i, s) in enumerate(set([s for (s, g) in runs]))}
  values = {g: 1.3 - i*0.6 for (i, g) in enumerate(set([g for (s, g) in runs]))}
  colors = {(s, g): multiply(hues[s], values[g]) for (s, g) in runs}

  ## add #Groups to forbiditer data from symk data
  print(" - add #Groups to forbiditer data from symk data")
  for (domain, problem, grouping) in tqdm(runs_T):
    groups_counts = [runs_T[(domain, problem, grouping)][search]["#Groups"] for search in searches if runs_T[(domain, problem, grouping)][search]["#Groups"] is not None]
    num_groups = 133769420
    match len(set(groups_counts)):
      case 1: num_groups = groups_counts[0]
      case 0: print(f"{domain}:{problem} ({grouping}) has no #Groups"); num_groups = 133769420
      case 2 if 0 in groups_counts: print(f"{domain}:{problem} ({grouping}) has a zero #Groups"); num_groups = [g for g in groups_counts if g != 0][0] #. Case where one search didn't reach the #Groups print, not necessarily a disagreement
      case x: raise RuntimeError(f"Expected exactly group count for problem {domain}:{problem} ({grouping}), found: {x}, {groups_counts}")
    for search in runs_T[(domain, problem, grouping)]:
      runs_T[(domain, problem, grouping)][search]["#Groups"] = num_groups

  ## sanity check
  print(" - running sanity check")
  disagreement = False
  for (domain, problem, grouping) in tqdm(runs_T):
    plan_counts = [(search, runs_T[(domain, problem, grouping)][search]["plans_found"]) for search in searches if runs_T[(domain, problem, grouping)][search]["coverage"]]
    if len(set([c for (s, c) in plan_counts])) > 1:
      disagreement = True
      print(f"{domain}:{problem} ({grouping}) has a disagreement, {plan_counts}")
  if disagreement: raise RuntimeError(f"There were disagreements in plan counts. Aborting.")

  ## plans_found / time
  print(" - plans_found / time")
  for (search, grouping) in tqdm(runs):
      times = []
      for run in tqdm(runs[(search, grouping)], leave=False):
        for i in range(1, run["plans_found"]+1):
          times.append(run[f"plan_{i}_time"])
      times = sorted(times)
      counts = range(1, len(times)+1)

      plt.plot(times, counts, label = f"{search}-{grouping}", color = colors[(search, grouping)])

  plt.xlabel("Time")
  plt.ylabel("Plans Found")
  plt.title(f"Plans Found Over Time")
  plt.grid(True)
  plt.legend()

  plt.savefig(f"{ensure_dir(f'{plot_dir}')}/plans_found_over_time.png", dpi=300, bbox_inches="tight")
  plt.close()

  ## progressive k
  print(" - progressive k")
  ks = list(range(1, int(next(iter(runs.values()))[0]["k"]))) #. Assumes all data has same k (or at least that the first one has max k)
  with tqdm(total=len(runs)) as pbar:
    for grouping in groupings:
      for search in searches:
        coverages = [0 for k in ks]
        for k in tqdm(ks, leave=False):
          for run in runs[(search, grouping)]:
            if run["plans_found"] >= k or run["exit_code"] == "ExitCode.FOUND_ALL":
              coverages[k-1] += 1

        plt.plot(ks, coverages, label = search)
        pbar.update(1)

      plt.xscale("log")
      plt.xticks(
        [10**e for e in range(0, int(math.ceil(math.log10(ks[-1]))) + 1)],
        [rf"$10^{{{e}}}$" for e in range(0, int(math.ceil(math.log10(ks[-1]))) + 1)],
      )
      plt.xlabel("K")
      plt.ylabel("Coverage")
      plt.title(f"Progressive Coverage ({grouping})")
      plt.grid(True)
      plt.legend()

      plt.savefig(f"{ensure_dir(f'{plot_dir}/progressive_coverage')}/{grouping}.png", dpi=300, bbox_inches="tight")
      plt.close()

  ## exhausted search / time
  print(" - exhausted search / time")
  with tqdm(total=len(runs)) as pbar:
    for grouping in groupings:
      for search in searches:
        times = sorted([run["total_time"] or 0 for run in runs[(search, grouping)] if run.has({"exit_code": "ExitCode.FOUND_ALL"})])
        counts = range(1, len(times)+1)
        plt.plot(times, counts, label = f"{search}-{grouping}", color = colors[(search, grouping)])
        pbar.update(1)

    plt.xlabel("Time")
    plt.ylabel("Exhausted Search")
    plt.title(f"Exhausted Search (FOUND_ALL) Over Time")
    plt.grid(True)
    plt.legend()

    plt.savefig(f"{ensure_dir(f'{plot_dir}')}/exhausted_search_over_time.png", dpi=300, bbox_inches="tight")
    plt.close()

  ## exhausted search vs last plan
  print(" - exhausted search vs last plan")
  with tqdm(total=len(runs)) as pbar:
    for grouping in groupings:
      for search in searches:
        color = colors[(search, next(iter(groupings)))] #. Since we do darkening/lighting locally here
        last_plans, search_done = map(sorted, zip(*[(run["last_plan_time_max"] or 0, run["total_time"] or 0) for run in runs[(search, grouping)] if run.has({"exit_code": "ExitCode.FOUND_ALL"})]))
        counts = range(1, len(last_plans)+1)
        plt.plot(counts, last_plans, label = f"{search}-Found last plan", color = multiply(color, 1.3))
        plt.plot(counts, search_done, label = f"{search}-Proved it", color = multiply(color, 0.7))
        plt.fill_between(counts, last_plans, search_done, alpha=0.3, color = color)
        pbar.update(1)

      plt.xlabel("#Problems")
      plt.ylabel("Time")
      plt.title(f"Exhausted Search vs Last Plan ({grouping})")
      plt.grid(True)
      plt.legend()

      plt.savefig(f"{ensure_dir(f'{plot_dir}/exhausted_search_vs_last_plan')}/{grouping}.png", dpi=300, bbox_inches="tight")
      plt.close()

  ## total time scatter
  print(" - total time scatter")
  step = 1000 #. grid steps
  max_time = max([run["total_time"] for run in all_runs])
  limit = 10 ** math.ceil(math.log10(max_time)) # since log scale
  for sa, sb in tqdm([(sa, sb) for sa in searches for sb in searches if sa < sb]):
    for grouping in groupings:
      points = []
      for (sa_run, sb_run) in tqdm([(runs_T[(domain, problem, grouping)][sa], runs_T[(domain, problem, grouping)][sb]) for (domain, problem, g) in runs_T if g == grouping], leave=False):
        points += [(sa_run["total_time"] if sa_run["coverage"] else limit, sb_run["total_time"] if sb_run["coverage"] else limit)]
      plt.scatter(*zip(*points))
      plt.xscale("log")
      plt.yscale("log")

      plt.xlabel(f"Time {sa}")
      plt.ylabel(f"Time {sb}")

      ticks = [1] + [10**e for e in range(1, int(math.log10(limit)))] + [limit]
      labels = [rf"$10^0$"] + [rf"$10^{{{e}}}$" for e in range(1, int(math.log10(limit)))] + [rf"$\infty$"]

      plt.xticks(ticks, labels)
      plt.yticks(ticks, labels)
      plt.title(f"Total Time {sa} vs {sb} ({grouping})")
      plt.grid(True)

      plt.savefig(f"{ensure_dir(f'{plot_dir}/total_time_vs/{grouping}')}/{sa}_vs_{sb}.png", dpi=300, bbox_inches="tight")
      plt.close()

  ## coverage / #Groups
  print(" - coverage / #Groups")
  n_bars = len(searches)
  width = 0.8 / n_bars
  i = 0
  for i, search  in tqdm(enumerate(searches)):
    group_counts = {}
    for group_count in tqdm([int(run["#Groups"]) for (s, g) in runs if s == search for run in runs[(s, g)] if run.has({"coverage": 1})], leave=False):
      group_counts[group_count] = group_counts.get(group_count, 0) + 1

    x = sorted([group_count for group_count in group_counts if group_count <= 50])
    y = [group_counts[group_count] for group_count in x]
    plt.bar(
      [xi - ((i - (n_bars - 1) / 2) * width) for xi in x],
      y,
      width = width,
      label = f"{search}",
    )

  plt.xlabel("#Groups")
  plt.ylabel("Coverage")
  plt.title(f"Coverage over #Groups")
  plt.grid(True)
  plt.legend()

  plt.savefig(f"{ensure_dir(f'{plot_dir}')}/coverage_over_num_groups.png", dpi=300, bbox_inches="tight")
  plt.close()

  ## exit code table
  print(" - exit code table")
  for grouping in tqdm(groupings):
    all_codes = set()
    exit_codes = {}
    for search in tqdm(searches, leave=False):
      exit_codes[search] = {"total": 0}
      for exit_code in tqdm([run["exit_code"].removeprefix("ExitCode.") for run in runs[(search, grouping)]], leave=False):
        all_codes.add(exit_code)
        exit_codes[search][exit_code] = exit_codes[search].get(exit_code, 0) + 1
        exit_codes[search]["total"] += 1

    w0 = max(len("total"), len(grouping), *[len(search) for search in exit_codes])
    w_rest = max([len(exit_code) for exit_code in all_codes])

    print(f"{grouping:>{w0}}", end=" | ")
    for exit_code in all_codes:
      print(f"{exit_code:^{w_rest}}", end=" | ")
    print(f"{"total":^{w_rest}}", end=" | \n")

    for search in exit_codes:
      print(f"{search:>{w0}}", end=" | ")
      for exit_code in all_codes:
        print(f"{exit_codes[search].get(exit_code, 0):^{w_rest}}", end=" | ")
      print(f"{exit_codes[search]["total"]:^{w_rest}}", end=" | \n")

    ### latex
    with open(f"{ensure_dir(f'{plot_dir}/tables/exit_code')}/{grouping}.tex", "w", encoding="utf-8") as f:
      f.write(latex_table([[grouping, *all_codes]] + [[search] + [exit_codes[search].get(exit_code, 0) for exit_code in all_codes] for search in searches]))


  ## summary table
  print(" - summary table")
  rows = ["coverage", "total_time", "plans_found"]
  for grouping in tqdm(groupings):
    columns = {}
    for search in tqdm(searches, leave=False):
      columns[search] = {row: 0 for row in rows}
      for run in tqdm(runs[(search, grouping)], leave=False):
        for row in rows:
          columns[search][row] += run[row]

    w0 = max(len(grouping), *[len(row) for row in rows])
    w_rest = max([len(search) for search in searches] + [len(f"{columns[search].get(row, 0)}") for row in rows for search in searches])

    print(f"{grouping:>{w0}}", end=" | ")
    for search in searches:
      print(f"{search:^{w_rest}}", end=" | ")
    print("")

    for row in rows:
      print(f"{row:>{w0}}", end=" | ")
      for search in searches:
        print(f"{columns[search].get(row, 0):^{w_rest}.2f}", end=" | ")
      print("")

    ### latex
    with open(f"{ensure_dir(f'{plot_dir}/tables/summary')}/{grouping}.tex", "w", encoding="utf-8") as f:
      f.write(latex_table([[grouping, *searches]] + [[row] + [int(columns[search].get(row, 0)) for row in rows] for search in searches]))

  print("Done")

if __name__ := "__main__":
  main()