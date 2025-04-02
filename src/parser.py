import xml.etree.ElementTree as ET
import pickle
import json

import argparse

# Parses a .pickle file to extract data and saves it to a JSON file.
# 
# The expected structure of the .pickle file is as follows:
# 
# 1. The .pickle file is a serialized Python object that can be a dictionary, list, or any other Python object.
# 2. The method assumes that the .pickle file contains a dictionary, which will be extracted and converted to JSON.
# 
# Arguments:
# - file_path (str): The path to the input .pickle file to be parsed.
# 
# Returns:
# - data (dict): The extracted data from the .pickle file, returned as a dictionary.
# 
# The method will also save the extracted data into a file called `data.json` in the current directory.
def parse_pickle(file_path):
    # Initialize an empty dictionary to store the extracted data
    data = {}

    # Open the .pickle file in binary read mode and load the data
    with open(file_path, "rb") as f:
        data = pickle.load(f)

    # Save the extracted data into a JSON file called "data.json"
    with open("data.json", "w") as file:
        json.dump(data, file, indent=4)

    print(f"Data has been extracted from {file_path} and saved to 'data.json'")
    return data


# Parses a .txt file containing B-Spline data and saves it to a JSON file.
# 
# The expected structure of the .txt file is as follows:
# 
# 1. The first line contains the number of control points (c).
# 2. The second line contains the number of knots (k).
# 3. The next `c` lines contain the control points. Each line may contain multiple numbers (e.g., x, y, z for 3D control points).
# 4. The following `k` lines contain the knot values. Each line contains one float value.
# 5. The last line contains the degree value of the B-Spline.
# 
# Arguments:
# - file_path (str): The path to the input .txt file to be parsed.
# 
# Returns:
# - data (dict): The extracted B-Spline data, including "Control Points", "Knots", and "Degree".
# 
# The method will also save the extracted data into a file called `data.json` in the current directory.
def parse_txt(file_path):
    print("print text, file path: " + file_path)
    with open(file_path, "r") as file:
        # Gets the number of control points (c) and knots (k)
        c = int(file.readline().strip())
        k = int(file.readline().strip())

        # Gets control points (each line may contain multiple numbers)
        control_points = [list(map(float, file.readline().strip().split())) for _ in range(c)]

        # Gets knots (each line contains one float value)
        knots = [float(file.readline().strip()) for _ in range(k)]

        # Gets the degree value (last line)
        degree = int(file.readline().strip())

    # Creates dictionary with data and puts it into a .json file named "data.json"
    data = {"Control Points": control_points, "Knots": knots, "Degree": degree}
    with open("data.json", "w") as file:
        json.dump(data, file, indent=4)
    
    return data


# Main method of this file
# This method should be the only method called by other files
#
# pre: file_path != NULL, file_path is a parsable file for this program
# post: Returns a dictionary containing BSpline Data extracted from file_path.
#       Keys are Control Points, Knots, and Degree. Returns error message if a 
#       non parsable file was input as a parameter
def parse_file(file_path):
    if file_path is None:
        return {"error": "Parameter is NULL"}
    elif file_path.endswith(".pickle"):
        print("Parsing file, file path: " + file_path)
        return parse_pickle(file_path)
    elif file_path.endswith(".txt"):
        print("Parsing file, file path: " + file_path)
        return parse_txt(file_path)
    else:
        return {"error": "Unsupported file format"}
    

##### DRIVER METHODS #####
def parse_commands():
    parser = argparse.ArgumentParser()

    parser.add_argument("-p", "--path", help="Path to file to parse for bspline information")
    args = parser.parse_args()

    return args

def run_parser():
    args = parse_commands()
    parse_file(args.path)

run_parser()