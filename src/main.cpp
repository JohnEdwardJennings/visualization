#include "json.hpp"
#include <codecvt>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>

// Define a type alias for our json and dictionary
using json = nlohmann::json;
using BSplineDataDict = std::unordered_map<std::string, json>;

int extractFileData(std::string filepathToExtract, BSplineDataDict *data);

int main(int argc, char **argv) {
  // TODO: Send the file path to the python parser
  const std::string pathToParse = std::string(argv[1]);

  if(pathToParse.empty()) {
    std::cout << "Path to parse is empty, please pass in path" << std::endl;
    return 0;
  } 

  const std::string commandToExecute = "python3 ./parser.py --path " + pathToParse;

  int result = system(commandToExecute.c_str());

  // failed to run parser
  if(result != 0) {
    return -1;
  }

  // Once parser.py is comeplete, a new file called 'data.json' is created
  // File 'data.json' contains the BSplineDataDict in the form of a HashMap
  BSplineDataDict data;

  std::string filepathToExtract = "data.json";

  extractFileData(filepathToExtract, &data);

  return 0;
}

/*
 * Method used to extarct data from an existing file called 'data.json'
 * Should be called after input file is parsed through "parser.py"
 * TODO: Not working for some reason. Even though 'data.json' exists, method is
 * throwing an error
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