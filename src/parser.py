import vtk
import pickle

# Method is used to parse standard format ".vtu" files for BSpline Data
def parse_vtu(file_path):
    # TODO: Don't Really know how to properly gather data from a ".vtu" file

    reader = vtk.vtkXMLUnstructuredGridReader()
    reader.SetFileName(file_path)
    reader.Update()
    
    data = reader.GetOutput()
    points = [data.GetPoint(i) for i in range(data.GetNumberOfPoints())]
    
    return {"control_points": points}


# Method is used to parse python ".pickle" files for BSpline Data
# Pickle file should be a dictionary formated as follows:
#
# "Control Points": List of control points
# "Knots": List of knots
# "Degrees": List (of size 1) containing the degree value
def parse_pickle(file_path):
    with open(file_path, "rb") as f:
        data = pickle.load(f)
    return data


# Method is used to parse a ".txt" file for BSpline Data
# Expected Formatting of file is as follows:
#
# Line 1: c - Number of Control Points
# Line 2: k - Number of Knots
# Next n lines: Control Points (Each line contains 1 control point)
# Next k lines: Knots (Each line contains 1 knot)
# Final line: Degree Value
def parse_txt(file_path):
    with open(file_path, "r") as file:
        # Gets the number of control points (c) and knots (k)
        c = int(file.readline().strip())
        k = int(file.readline().strip())
        
        # Gets controld points, knots, and degree, storing them into lists
        control_points = [float(file.readline().strip()) for _ in range(c)]
        knots = [float(file.readline().strip()) for _ in range(k)]
        degree = [int(file.readline().strip())]
        
    return {"Control Points": control_points, "Knots": knots, "Degree": degree}


# Main method of this file
# This method should the only method called by other files
#
# pre: file_path != NULL, file_path is a parsable file for this program
# post: Returns a dictionary containing BSpline Data extracted from file_path
#       Keys are Control Points, Knots, and Degree. Returns error message if a 
#       non parsable file was input as a parameter
def parse_file(file_path):
    if file_path is None:
        return {"error": "Parameter is NULL"}
    elif file_path.endswith(".vtu"):
        return parse_vtu(file_path)
    elif file_path.endswith(".pickle"):
        return parse_pickle(file_path)
    elif file_path.endswith(".txt"):
        return parse_txt(file_path)
    else:
        return {"error": "Unsupported file format"}