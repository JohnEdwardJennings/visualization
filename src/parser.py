import pickle
import json

import argparse
import pathlib
import os

from typing import Optional

# Parses a .pickle file to extract data and saves it to a JSON file.
# 
# The expected structure of the .pickle file is as follows:
# 
# 1. The .pickle file is a serialized Python object that can be a dictionary, list, or any other Python object.
# 2. The method assumes that the .pickle file contains a dictionary, which will be extracted and converted to JSON.
# 
# Arguments:
# - file_path (str): The path to the input .pickle file to be parsed.
# - output_path: Path to dump json. 
# 
# Returns:
# - data (dict): The extracted data from the .pickle file, returned as a dictionary.
# 
# The method will also save the extracted data into a file called `data.json` in the current directory.
def parse_pickle(file_path, output_path):
    # Initialize an empty dictionary to store the extracted data
    data = {}

    # Open the .pickle file in binary read mode and load the data
    with open(file_path, "rb") as f:
        data = pickle.load(f)

    # Save the extracted data into a JSON file called "data.json"
    if output_path is None:
        output_path = "data.json"
    with open(output_path, "w") as file:
        json.dump(data, file, indent=4)

    print(f"Data has been extracted from {file_path} and saved to {output_path}")
    return data


# Parses a .txt file containing B-Spline data and saves it to a JSON file.
# 
# The expected structure of the .txt file is as follows:
# 
# 1. The first line contains the number of control points (c).
# 2. The second line contains the number of knots (k).
# 3. The next `c` lines contain the control points. Each line may contain multiple numbers (e.g., x, y, z for 3D control points).
# 4. The following `k` lines contain the knot values. Each line contains one float value.
# 5. The last line contains the degree value of the B-Spline. Each line may contain multiple numbers (e.g., x, y, z for 3D control points)
# 
# Arguments:
# - file_path (str): The path to the input .txt file to be parsed.
# - output_path: Path to dump json. 
# 
# Returns:
# - data (dict): The extracted B-Spline data, including "Control Points", "Knots", and "Degree".
# 
# The method will also save the extracted data into a file called `data.json` in the current directory.
def parse_txt(file_path, output_path):
    print("print text, file path: " + file_path + ". output path: " + output_path)
    with open(file_path, "r") as file:
        # Gets the number of control points (c) and knots (k)
        c = int(file.readline().strip())
        k = int(file.readline().strip())

        # Gets control points (each line may contain multiple numbers)
        control_points = [list(map(float, file.readline().strip().split())) for _ in range(c)]

        # Gets knots (each line contains one float value)
        knots = [float(file.readline().strip()) for _ in range(k)]

        # Gets the degree value (last line)
        degrees = list(map(float, file.readline().strip().split()))
        # degree = int(file.readline().strip())

    # Creates dictionary with data and puts it into a .json file named "data.json"
    data = {"Number of Control Points": c, "Control Points": control_points, "Knots": knots, "Degree": degrees}
    with open(output_path, "w") as file:
        json.dump(data, file, indent=4)
    
    return data


# Main method of this file
# This method should be the only method called by other files
#
# pre: file_path != NULL, file_path is a parsable file for this program
# post: Returns a dictionary containing BSpline Data extracted from file_path.
#       Keys are Control Points, Knots, and Degree. Returns error message if a 
#       non parsable file was input as a parameter
def parse_file(file_path: str, output_path: str):
    if file_path is None:
        return {"error": "Parameter is NULL"}
    elif file_path.endswith(".pickle"):
        print("Parsing file, file path: " + file_path)
        return parse_pickle(file_path, output_path)
    elif file_path.endswith(".txt"):
        print("Parsing file, file path: " + file_path)
        return parse_txt(file_path, output_path)
    else:
        return {"error": "Unsupported file format"}
    

##### DRIVER METHODS #####
def parse_commands():
    parser = argparse.ArgumentParser()

    parser.add_argument("-p", "--path", help="Path to file to parse for bspline information")
    parser.add_argument("-o", "--output_path", nargs="?", const="data.json", help="Path to output json file")
    args = parser.parse_args()

    return args

def run_parser(file_input_path: Optional[str], output_path: str = "data.json"):
    input_filepath: str = file_input_path
    parsed_output_path: str = output_path 

    if file_input_path is None or output_path is None:
        args = parse_commands()

        # convert namespace type to dictionary for parsing
        argument_variables = vars(args)
        print("arg variables: " + str(argument_variables))
        input_filepath = argument_variables["path"]
        parsed_output_path = argument_variables["output_path"]
    
    print(f"input file path: {input_filepath}. output filepath: {parsed_output_path}")

    if input_filepath is None:
        print("Input filepath must be specified")
        return
    if parsed_output_path is None: 
        parsed_output_path = "data.json"

    does_output_path_exist = pathlib.Path(parsed_output_path).exists()

    if not does_output_path_exist:
        os.makedirs(os.path.dirname(parsed_output_path), exist_ok=True) 

    parse_file(file_path=input_filepath, output_path=parsed_output_path)

def manual_run(): 
    run_parser(None, None)

if __name__ == "__main__":
    manual_run()