#include "json.hpp"
#include <array>
#include <codecvt>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>

#include "bspline/geometry.h"

// Define a type alias for our json and dictionary
using json = nlohmann::json;
using BSplineDataDict = std::unordered_map<std::string, json>;

typedef std::vector<std::vector<double>> IntegerMatrix;
typedef std::vector<double> IntegerArray;

int extractFileData(std::string filepathToExtract, BSplineDataDict* data);
IntegerMatrix createGlobalPoints(int numNodes[]);

int main(int argc, char **argv) {
  // Retrieving the file path the of the users file
  // File path should be the path to the files that contains BSplineData
  const std::string pathToParse = std::string(argv[1]);
  if(pathToParse.empty()) {
    std::cout << "Path to parse is empty, please pass in path" << std::endl;
    return 0;
  } 



  // Parses the given file using the python parser
  const std::string commandToExecute = "python3 ./parser.py --path " + pathToParse;
  int result = system(commandToExecute.c_str());

  // Failed to run parser
  if(result != 0) {
    return -1;
  }



  // Once parser.py is comeplete, a new file called 'data.json' is created
  // File 'data.json' contains the BSplineDataDict in the form of a HashMap
  // The following code extracts that and stores it in a BSplineDataDict (unordered_map)
  BSplineDataDict data;
  std::string filepathToExtract = "data.json";
  result = extractFileData(filepathToExtract, &data);

  // Failed to extract data from ,json
  if(result != 0){
    return -1;
  }

  /* ----- Code used to print out the contents of the data BSplineData array ----- */
  std::cout << "------------------------------------" << std::endl;
  std::cout << "Printing out extracted data contents" << std::endl;
  for (const auto& pair : data) {
        std::cout << pair.first << ": " << pair.second << std::endl;
  }
  std::cout << "------------------------------------" << std::endl;



  /* ----- TESTING THE "createGlobalPoints" METHOD  ----- */
  int test[3] = {2, 2, 2};
  IntegerMatrix globalPoints = createGlobalPoints(test);
  // USE TO PRINT OUT THE GLOBAL POINTS ARRAY
  // int globalPointsSize = test[0] * test[1] * test[2];
  // for(int i = 0; i < globalPointsSize; i++){
  //   std::cout << "[" << globalPoints[i][0] << ", ";
  //   std::cout << globalPoints[i][1] << ", ";
  //   std::cout << globalPoints[i][2] << "]\n";
  // }

	std::array<size_t,2> degrees{2,2};
	std::array<std::vector<double>, 2> knot_vectors{std::vector<double>{1, 2, 3, 4, 5}, std::vector<double>{1, 2, 3, 4, 5}}; 
	std::vector<std::array<double, 3>> control_points(25);
	BSplineGeometry<2,3> b(degrees, knot_vectors, control_points);
	std::vector<std::array<double,2>> x = {{0, 1}, {2, 3}};
	b.evaluate(x);

  return 0;
}

/*
 * Method used to extarct data from an existing file called 'data.json'
 * Should be called after input file is parsed through "parser.py"
 */
int extractFileData(std::string filepathToExtract, BSplineDataDict *data) {
  // Open the JSON file
  std::ifstream file(filepathToExtract);
  if (!file.is_open()) {
    std::cerr << "ERROR: File does not exist or cannot be opened" << std::endl;
    return 1;
  }

  // Parse JSON file into a json object
  json jsonData;
  file >> jsonData;

  // Store extracted values into an unordered_map
  BSplineDataDict splineData;
  splineData["Control Points"] = jsonData["Control Points"];
  splineData["Knots"] = jsonData["Knots"];
  splineData["Degree"] = jsonData["Degree"];

  *data = splineData;
  return 0; // Successfully extracted data
}

/*
 * Method is used to create a global points array
 * Creates a global points array for a cube
 * 
 * Parameter: Integer array of length 3, where the number of x-points,
 *            y-points, and z-points are contained within the array respectively
 * 
 * Return: Returns a global points array given the specific input.
 *         Points in each direction are evenly spaced
 * 
 */
IntegerMatrix createGlobalPoints(int numNodes[]){
    // Increments each number by 1 to deal with orgin point
    numNodes[0] += 1;
    numNodes[1] += 1;
    numNodes[2] += 1;

    // Generates the global points array
    int totalPoints = numNodes[0] * numNodes[1] * numNodes[2];
    IntegerMatrix grid(totalPoints, IntegerArray(3));
    int count = 0;
    for(int i = 0; i < numNodes[0]; i++){
      for(int j = 0; j < numNodes[1]; j++){
        for(int k = 0; k < numNodes[2]; k++){
          grid[count][0] = (static_cast<double>(i) / (numNodes[0] - 1));
          grid[count][1] = (static_cast<double>(j) / (numNodes[1] - 1));
          grid[count][2] = (static_cast<double>(k) / (numNodes[2] - 1));
          count++;
        }
      }
    }

    return grid;
}