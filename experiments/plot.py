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

colors = plt.cm.tab10.colors

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
    return self.value[key]

  def has(self, dict):
    for key in dict:
      try:
        if self.value[key] != dict[key]: return False
      except KeyError as e:
        return False
    return True

def main():
  print("Starting")
  plot_dir = ensure_dir("plots")
  searches = ["forbiditer", "symk_fw"]
  groupings = ["prefix(1)", "prefix(2)"]

  with open("./data/experiments-eval/properties", 'r') as f:
    properties = json.loads(f.read())
  with open("./domains_with_zero_cost.txt", "r") as forbidden_domain_file:
   forbiden_domains = [line.replace("\n","") for line in forbidden_domain_file]
  runs = [X((key, properties[key])) for key in properties if properties[key]["domain"] not in forbiden_domains]

  ## plans_found / time
  brightness = 1.3
  for grouping in groupings:
    color_i = 0;
    for search in searches:
      color = multiply(colors[color_i % len(colors)], brightness)
      times = []
      for data in [run for run in runs if run.has({"search": search, "grouping": grouping})]:
        for i in range(1, data["plans_found"]+1):
          times.append(data[f"plan_{i}_time"])
      times = sorted(times)
      counts = range(1, len(times)+1)

      plt.plot(times, counts, label = f"{search}-{grouping}", color = color)
      color_i += 1
    brightness -= 0.6

  plt.xlabel("Time")
  plt.ylabel("Plans Found")
  plt.title(f"Plans Found Over Time")
  plt.grid(True)
  plt.legend()

  plt.savefig(f"{ensure_dir(f'{plot_dir}')}/plans_found_over_time.png", dpi=300, bbox_inches="tight")
  plt.close()

  ## progressive k
  ks = list(range(2, 6))
  for grouping in groupings:
    for search in searches:
      coverages = [0 for k in ks]
      for k in ks:
        for data in [run for run in runs if run.has({"search": search, "grouping": grouping})]:
          if data["plans_found"] >= k or data["exit_code"] == "ExitCode.FOUND_ALL":
            coverages[k-2] += 1

      plt.plot(ks, coverages, label = search)

    plt.xlabel("K")
    plt.xticks(ks)
    plt.ylabel("Coverage")
    plt.title(f"Progressive Coverage ({grouping})")
    plt.grid(True)
    plt.legend()

    plt.savefig(f"{ensure_dir(f'{plot_dir}/progressive_coverage')}/{grouping}.png", dpi=300, bbox_inches="tight")
    plt.close()

  ## exhausted search / time
  brightness = 1.3
  for grouping in groupings:
    color_i = 0;
    for search in searches:
      color = multiply(colors[color_i % len(colors)], brightness)
      times = sorted([run["total_time"] or 0 for run in runs if run.has({"search": search, "grouping": grouping, "exit_code": "ExitCode.FOUND_ALL"})])
      counts = range(1, len(times)+1)
      plt.plot(times, counts, label = f"{search}-{grouping}", color = color)
      color_i += 1
    brightness -= 0.6

  plt.xlabel("Time")
  plt.ylabel("Exhausted Search")
  plt.title(f"Exhausted Search (FOUND_ALL) Over Time")
  plt.grid(True)
  plt.legend()

  plt.savefig(f"{ensure_dir(f'{plot_dir}')}/exhausted_search_over_time.png", dpi=300, bbox_inches="tight")
  plt.close()

  ## exhausted search vs last plan
  for grouping in groupings:
    color_i = 0;
    for search in searches:
      color = colors[color_i % len(colors)]
      last_plans = sorted([run["last_plan_time_max"] or 0 for run in runs if run.has({"search": search, "grouping": grouping, "exit_code": "ExitCode.FOUND_ALL"})])
      search_done = sorted([run["total_time"] or 0 for run in runs if run.has({"search": search, "grouping": grouping, "exit_code": "ExitCode.FOUND_ALL"})])
      counts = range(1, len(last_plans)+1)
      plt.plot(counts, last_plans, label = f"{search}-Found last plan", color = multiply(color, 1.3))
      plt.plot(counts, search_done, label = f"{search}-Proved it", color = multiply(color, 0.7))
      plt.fill_between(counts, last_plans, search_done, alpha=0.3, color = color)
      color_i += 1

    plt.xlabel("#Problems")
    plt.ylabel("Time")
    plt.title(f"Exhausted Search vs Last Plan ({grouping})")
    plt.grid(True)
    plt.legend()

    plt.savefig(f"{ensure_dir(f'{plot_dir}/exhausted_search_vs_last_plan')}/{grouping}.png", dpi=300, bbox_inches="tight")
    plt.close()

  ## total time scatter
  #! Assumes just two searches
  selected = (searches[0], searches[1])
  step = 1000 #. grid steps
  for grouping in groupings:
    problems = [(run["domain"], run["problem"]) for run in runs if run.has({"grouping": grouping})] # Extract problems first to ensure same order
    times = {search: [] for search in searches}
    max_time = max([run["total_time"] for run in runs if run.has({"grouping": grouping})])
    limit = ((int(max_time * 1.10) + step - 1) // step) * step #. round to step, so it is exactly at the line in the plot
    for (domain, problem) in problems:
      for search in searches:
        key = {"domain": domain, "problem": problem, "grouping": grouping, "search": search}
        time = [run["total_time"] if run["coverage"] else limit for run in runs if run.has(key)]
        if not len(time) == 1: raise RuntimeError(f"Expected exactly one run with {key}, found: {len(time)}")
        times[search] += time
    plt.scatter(times[selected[0]], times[selected[1]])

    plt.xlabel(f"Time {selected[0]}")
    plt.ylabel(f"Time {selected[1]}")
    ticks = list(range(0, limit + step, step))
    labels = list(map(str, ticks))[:-1] + ["∞"]
    plt.xticks(ticks, labels)
    plt.yticks(ticks, labels)
    plt.title(f"Total Time {selected[0]} vs {selected[1]} ({grouping})")
    plt.grid(True)

    plt.savefig(f"{ensure_dir(f'{plot_dir}/total_time_vs')}/{grouping}.png", dpi=300, bbox_inches="tight")
    plt.close()

if __name__ := "__main__":
  main()