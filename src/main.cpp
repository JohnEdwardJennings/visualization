#include <iostream>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include "json.hpp"
#include "fileInput.h"

// Define a type alias for our json and dictionary
using json = nlohmann::json;
using BSplineDataDict = std::unordered_map<std::string, json>;

int extractFileData(BSplineDataDict* data);

int main(int argc, char** argv) {
  // TODO: parse arguments from argv, we want a filepath
  // to navigate to
  std::cout << "number of arguments: " << argc << "\n";

  std::cout << "Hello World!\n";

  FileInput fileInput = FileInput(std::string("hi")); 

  std::cout << "file name: " << fileInput.getFilename() << "\n";

  // TODO: Send the file path to the python parser
  
  // Once parser.py is comeplete, a new file called 'data.json' is created
  // File 'data.json' contains the BSplineDataDict in the form of a HashMap
  BSplineDataDict data;
  extractFileData(&data);



  return 0;
}

/*
 * Method used to extarct data from an existing file called 'data.json'
 * Should be called after input file is parsed through "parser.py"
 * TODO: Not working for some reason. Even though 'data.json' exists, method is throwing an error
 */
int extractFileData(BSplineDataDict* data){
    // Open the JSON file
    std::ifstream file("data.json");
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
    return 0;  // Successfully extracted data
}