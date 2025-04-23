
#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "bspline/geometry.h"

#include "bsplineData.h"
#include "json.hpp"

using BSplineDataDict = std::unordered_map<std::string, json>;

bool parseTextInput(const std::string &pathToParse,
                    const std::string &outputPath) {
  const std::string commandToExecute = "python3 ./parser.py --path " +
                                       pathToParse + " --output_path " +
                                       outputPath;
  int result = system(commandToExecute.c_str());

  return result >= 0;
}

bool directoryExists(const std::string &path) {
  return std::filesystem::exists(path) && std::filesystem::is_directory(path);
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

int main() {
  std::string testInputDirectoryPath = "./tests/inputs";
  std::string testOutputDirectoryPath = "./tests/outputs";

  for (auto &testInputFile :
       std::filesystem::directory_iterator{testInputDirectoryPath}) {

    std::ifstream fileInputStream{testInputFile.path()};

    // ex: if test input file is "hi.txt" => "test_hi.txt"
    std::string filename = testInputFile.path().filename();
    std::string testFilename = "test_" + filename;

    std::filesystem::path fileOutputPath{testOutputDirectoryPath};
    fileOutputPath.append(testFilename);

    // should generate corresponding test_input json file
    // ex: input file name: test_hello.txt output: test_hello.json
    parseTextInput(testInputFile.path(), fileOutputPath.string());

    BSplineDataDict data;
    std::string filepathToExtract = fileOutputPath;
    int result = extractFileData(filepathToExtract, &data);

    // malformed json data, incorrect format or data
    if (result < 0) {
      std::cout << "File path: " << testInputFile.path().string()
                << ". Could not be parsed.\n";
      continue;
    }

    BSplineData structuredBsplineData{data};

    // run evaluator, using the filename, search the standard
    // folder and compare csv data
    // run evaluator and output
    // BSplineGeometry<size_t n_kdims, size_t n_cdims> geometry;
    std::vector<std::vector<double>> controlPoints =
        structuredBsplineData.getControlPoints();
    std::vector<std::vector<double>> knots =
        structuredBsplineData.getKnotsVector();
    std::vector<size_t> degrees = structuredBsplineData.getDegree();

    size_t knotDimensions = static_cast<size_t>(knots.size());
    size_t controlDimension = static_cast<size_t>(controlPoints.size());

    // templates are compile time, need to map knot dimensions,
    // control dimensions manually to appropriate spline evaluator

  return 0;
}
