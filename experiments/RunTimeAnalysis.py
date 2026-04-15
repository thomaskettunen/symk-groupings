import json
import os
import matplotlib.pyplot as plt
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("-runtimes", action='store_true')
parser.add_argument("-compare", action='store_true')
parser.add_argument("-foundall", action='store_true')
parser.add_argument("-log", action="store_true")
args = parser.parse_args()

runs = {}
for file in ["our_run_prefix1","our_run_prefix2","their_run_with_fix2"]:
    with open(f"runs/{file}/data/experiments-eval/properties","r") as f:
        runs[file] = json.load(f)


names = {
    "their_run_with_fix2":"FI submultiset",
    "our_run_prefix1":"Grouped prefix₁",
    "our_run_prefix2":"Grouped prefix₂"
}

tasks = runs["their_run_with_fix2"].keys()
filteredtasks = []

for task in tasks:
    count = 0
    notIncludedDomain = False
    for domain in ["agricola-opt18-strips", "data-network-opt18-strips", "woodworking-opt08-strips", "woodworking-opt11-strips", "organic-synthesis-split-opt18-strips-p08.pddl"]:
        if domain in task:
            notIncludedDomain = True
    if not notIncludedDomain:
        filteredtasks.append(task)
tasks = filteredtasks
print(f"{len(tasks)} tasks")

def GetTotalAndLastPlanTime(planner, found_all = False):
    if found_all:
        coverage1_foundall = []
        coverage1_foundk = []
    else:
        coverage1 = []

    coverage0 = []
    unsolvable = 0
    for task in tasks:
        if "plans found" in runs[planner][task] and runs[planner][task]["plans found"] != 0:
                if runs[planner][task]["coverage"] == 1:
                    if found_all:
                        if runs[planner][task]["exit code"] == "ExitCode.FOUND_ALL":
                            coverage1_foundall.append((runs[planner][task]["last plan time_mean"],runs[planner][task]["total time"]))
                        else:
                            coverage1_foundk.append((runs[planner][task]["last plan time_mean"],runs[planner][task]["total time"]))
                    else:
                        coverage1.append((runs[planner][task]["last plan time_mean"],runs[planner][task]["total time"]))
                else:
                     coverage0.append((runs[planner][task]["last plan time_mean"],runs[planner][task]["total time"]))
        elif "plans found" in runs[planner][task] and runs[planner][task]["plans found"] == 0 and runs[planner][task]["coverage"] == 1:
            unsolvable +=1
    if not found_all:
        print("----------------------------------------")
        print(f"For the planner {planner}:")
        print(f"{len(coverage1)} planning tasks are solved.")
        print(f"{unsolvable} tasks are unsolvable.")
        print(f"At least one plan is found for {len(coverage0)} unsolved planning tasks")
        print(f"{len(tasks) -(len(coverage0)+len(coverage1)+unsolvable)} planning tasks are unsolved.")

    if found_all:
        return coverage0, coverage1_foundall, coverage1_foundk
    else:
        return coverage0, coverage1

def ComparePlannerTimes(planner1, planner2):
    times = []
    for task in tasks:
        Err = False
        if "coverage" in runs[planner1][task]:
            if runs[planner1][task]["coverage"] == 0:
                planner1_time = 600
            elif "total time" in runs[planner1][task]:
                planner1_time = runs[planner1][task]["total time"]
            else:
                Err = True
        else:
            Err = True

        if "coverage" in runs[planner2][task]:
            if runs[planner2][task]["coverage"] == 0:
                planner2_time = 600
            elif "total time" in runs[planner2][task]:
                planner2_time = runs[planner2][task]["total time"]
            else:
                Err = True
        else:
            Err = True
        if not Err:
            times.append((planner1_time, planner2_time))

    return times

def CountUnsolved(times):
    both_unsolved = 0
    unsolved_1 = 0
    unsolved_2 = 0
    for task in times:
        if task[0] == 600 and task[1] == 600:
            both_unsolved+= 1
        elif task[0] == 600:
            unsolved_1 += 1
        elif task[1] == 600:
            unsolved_2 += 1
    return both_unsolved, unsolved_1, unsolved_2
                 

if args.runtimes:
    if args.foundall:
            for planner in ["our_run_prefix1","our_run_prefix2"]:
                coverage0, coverage1foundall, coverage1foundk = GetTotalAndLastPlanTime(planner, True)
                for coverage in [("unsolved", coverage0), ("solved foundall", coverage1foundall), ("solved foundk", coverage1foundk)]:    
                    x_val = [data[0] for data in coverage[1]]
                    y_val = [data[1] for data in coverage[1]]
                    plt.ecdf(x_val)
                    plt.title(names[planner] +" "+ coverage[0])
                    plt.xlabel("Last plan time")
                    plt.xlim(0,600)
                    plt.savefig("analysisFolder/"+ planner +" "+ coverage[0]+" cdf")
                    plt.close()

                    plt.ecdf(y_val)
                    plt.title(names[planner] +" "+ coverage[0])
                    plt.xlabel("Total plan time")
                    plt.xlim(0,600)
                    plt.savefig("analysisFolder/"+ planner +" "+ coverage[0]+" total plan time cdf")
                    plt.close()
                    
                    if coverage[0] == "unsolved":
                        plt.scatter(x_val,y_val)
                        plt.title(names[planner] +" "+ coverage[0])
                        plt.xlabel("Last plan time")
                        plt.ylabel("Total time")
                        if args.log:
                            plt.xscale("log", base=2)
                            plt.yscale("log", base=2)
                        plt.xlim(0,600)
                        plt.ylim(0,600)
                        plt.savefig("analysisFolder/"+ planner +" "+ coverage[0])
                        plt.close()
                    if coverage[0] == "solved foundall":
                        plt.scatter(x_val,y_val)
                        plt.title(names[planner] +" "+ coverage[0])
                        plt.xlabel("last plan time")
                        plt.ylabel("total time")
                        if args.log:
                            plt.xscale("log", base=2)
                            plt.yscale("log", base=2)
                        plt.xlim(0,600)
                        plt.ylim(0,600)
                        plt.savefig("analysisFolder/"+ planner +" "+ coverage[0])
                        plt.close()


    else:
        for planner in runs.keys():
            coverage0, coverage1 = GetTotalAndLastPlanTime(planner)
            for coverage in [("unsolved", coverage0), ("solved", coverage1)]:    
                x_val = [data[0] for data in coverage[1]]
                y_val = [data[1] for data in coverage[1]]
                plt.ecdf(x_val)
                plt.title(names[planner] +" "+ coverage[0])
                plt.xlabel("last plan time")
                plt.xlim(0,600)
                plt.savefig("analysisFolder/"+ planner +" "+ coverage[0]+" cdf")
                plt.close()
                if coverage[0] == "solved":
                    plt.scatter(x_val,y_val)
                    plt.title(names[planner] +" "+ coverage[0])
                    plt.xlabel("last plan time")
                    plt.ylabel("total time")
                    if args.log:
                        plt.xscale("log", base=2)
                        plt.yscale("log", base=2)
                    plt.xlim(0,600)
                    plt.ylim(0,600)
                    plt.savefig("analysisFolder/"+ planner +" "+ coverage[0])
                    plt.close()


if args.compare:
    for planner in ["our_run_prefix1","our_run_prefix2"]:
        times = ComparePlannerTimes(planner, "their_run_with_fix2")
        x_val = [data[0] for data in times]
        y_val = [data[1] for data in times]

        both_unsolved, our_unsolved, their_unsolved = CountUnsolved(times)

        print(f"Of {len(times)} planning tasks, {their_unsolved} tasks were unsolved by their planner, {our_unsolved} were unsolved by our planner, {both_unsolved} were unsolved by both")

        plt.scatter(x_val,y_val)
        plt.title(f"{names['their_run_with_fix2']} vs {names[planner]}")
        plt.xlabel(names[planner])
        plt.ylabel(names["their_run_with_fix2"])
        if args.log:
            plt.xscale("log", base=2)
            plt.yscale("log", base=2)
        plt.xlim(0,600)
        plt.ylim(0,600)
        plt.savefig("analysisFolder/" + "their run" +" vs "+ planner)
        plt.close()
        