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

typedef std::vector<std::vector<double>> IntegerMatrix;
typedef std::vector<double> IntegerArray;

int extractFileData(BSplineDataDict* data);
IntegerMatrix createGlobalPoints(int numNodes[]);

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

/*
 * Method is used to create a global points array
 * Creates a global points array for a cube
 * 
 * Parameter: integer array of length 3, where the number of x-points,
 *            y-points, and z-points are contained within the array respectively
 * 
 */
IntegerMatrix createGlobalPoints(int numNodes[]){
    // Increments each number by 1 to deal with orgin point
    numNodes[0] += 1;
    numNodes[1] += 1;
    numNodes[2] += 1;

    // Creates and array of doubles representing the range of possible X point values
    IntegerArray rangeX(numNodes[0]);
    for (int i = 0; i < numNodes[0]; i++) {
        rangeX[i] = static_cast<double>(i) / (numNodes[0] - 1);
    }

    // Creates and array of doubles representing the range of possible Y point values
    IntegerArray rangeY(numNodes[1]);
    for (int i = 0; i < numNodes[1]; i++) {
        rangeY[i] = static_cast<double>(i) / (numNodes[1] - 1);
    }

    // Creates and array of doubles representing the range of possible Z point values
    IntegerArray rangeZ(numNodes[2]);
    for (int i = 0; i < numNodes[2]; i++) {
        rangeZ[i] = static_cast<double>(i) / (numNodes[2] - 1);
    }

    // Generates the global points array
    int totalPoints = numNodes[0] * numNodes[1] * numNodes[2];
    IntegerMatrix grid(totalPoints, IntegerArray(3));
    int count = 0;
    for(int i = 0; i < numNodes[0]; i++){
      for(int j = 0; j < numNodes[1]; j++){
        for(int k = 0; k < numNodes[2]; k++){
          grid[count] = {rangeX[i], rangeY[j], rangeZ[k]};
          count++;
        }
      }
    }

    return grid;
}