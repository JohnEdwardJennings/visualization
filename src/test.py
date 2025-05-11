import sys
import os
import json

import numpy as np

from bspline_src import bspline_eval

def gold_standard():
    pass

def run_gold_standard(testcase_folder: str, standard_output_folder: str):
    # expecting a directory with no nested directories
    for root, dirs, files in os.walk(testcase_folder, topdown=False):
        for filename in files:
            test_number_start_idx = filename.find("test_")
            test_number_end_idx = filename.find(".json")
            test_name = filename[test_number_start_idx:test_number_end_idx]

            with open("./tests/outputs/" + filename, "r") as test_data:
                test_json = json.load(test_data)

                control_points = test_json["Control Points"]
                degrees = test_json["Degree"]
                knots = test_json["Knots"]

                print("*" * 10)
                print(f"Running test: {test_name}")

                cps: list[float] = control_points
                knots: list[list[float]] = knots
                n_cp_list: list[int] = [len(control_points)]
                degs: list[int] = [degrees]

                print("knots: " + str(knots) + ", n cp list: " + str(n_cp_list))
                print(f"knots[0]: " + str(len(knots[0])))
                print(f"len cps: {len(cps)}, ")

                eval_pts = np.linspace(0.0,1.0,100)

                param_point_to_point : dict[float, float] = {}

                for point in eval_pts: 
                    param_point_to_point[point] = bspline_eval.naive_deboor(cps=cps, knots=knots, degs=degs, 
                                            x=point, n_cp_list=n_cp_list)
                
                with open(f"{standard_output_folder}/standard_{test_name}.csv", "a") as standard_output:
                    for param_point in param_point_to_point:
                        standard_output.write(f"{param_point},{param_point_to_point[param_point]}")
                
                print("evaled points")
                print(param_point_to_point)
                print("*" * 10)


def run():
    # for now, just using hardcoded test case input files
    # testcase_folder : str = sys.argv[1]
    # misnomer, really just the json files of test inputs
    testcase_input_folder: str = "./tests/outputs"
    testcase_gold_standard_output: str = "./tests/standard"

    if not os.path.isdir(testcase_input_folder):
        print("path to test case folder is not a path")
    
    if not os.path.exists(testcase_gold_standard_output):
        os.mkdir(testcase_gold_standard_output)

    if os.path.isdir(testcase_input_folder): 
        run_gold_standard(testcase_folder=testcase_input_folder, standard_output_folder=testcase_gold_standard_output)

run()
