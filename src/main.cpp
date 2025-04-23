#include "bspline/geometry.h"
#include "json.hpp"
#include <algorithm>
#include <array>
#include <codecvt>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>
#include <vtkCellArray.h>
#include <vtkCommonDataModelModule.h>
#include <vtkHexahedron.h>
#include <vtkLine.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLUnstructuredGridWriter.h>

#include "bsplineData.h"

// Define a type alias for our json and dictionary
using json = nlohmann::json;
using BSplineDataDict = std::unordered_map<std::string, json>;

typedef std::vector<std::vector<double>> IntegerMatrix;
typedef std::vector<double> IntegerArray;

int extractFileData(std::string filepathToExtract, BSplineDataDict *data);
IntegerMatrix createGlobalPoints(int numNodes[]);
IntegerMatrix createHexahedronCellsArray(int numNodes[]);
void generateHexahedralGrid(int numX, int numY, int numZ,
                            const std::string filename);
void generateWireframe(int numX, int numY, int numZ,
                       const std::string filename);

int getDegree(BSplineDataDict bsplineData);
int upSample = 20;

const std::vector<std::string> supportedFileExtensions = {".txt", ".pickle"};

int main(int argc, char **argv) {
  if (argv == nullptr) {
    std::cout << "Path to parse is empty, please pass in path" << std::endl;
    return -1;
  }

  // Retrieving the file path the of the users file
  // File path should be the path to the files that contains BSplineData
  if (argv[1] == nullptr) {
    std::cout << "Path to parse is empty, please pass in path" << std::endl;
    return -1;
  }

  const std::string pathToParse = std::string(argv[1]);
  std::cout << "path to parse: " << pathToParse << "\n";
  if (pathToParse.empty()) {
    std::cout << "Path to parse is empty, please pass in path" << std::endl;
    return 0;
  }

  if (std::filesystem::is_directory(pathToParse)) {
    std::cout << "Input path is a directory, append --r to recursively parse "
                 "input files. Ex: Visualization [input path] --r\n";
  }

  // later, potentially expand using flags
  std::string flags = "";

  const std::string recursiveFlag = "--r";
  bool isRecursiveParsing = false;
  if (argv[2] != nullptr) {
    std::string flags = std::string(argv[2]);

    isRecursiveParsing = !flags.empty() && flags == recursiveFlag;
  }

  if (flags.length() > 0 && !isRecursiveParsing) {
    std::cout << "Invalid Flag, only --r for recursive parsing is valid.\n";
  }

  std::vector<std::string> pathsToProcess{};

  // hard coding output folder for now
  std::string outputRootPath = "./output";
  if (!std::filesystem::exists(outputRootPath)) {
    std::filesystem::create_directories("output");
  }

  std::cout << "Output root path: " << outputRootPath << "\n";

  if (!outputRootPath.empty() && isRecursiveParsing &&
      std::filesystem::is_directory(pathToParse)) {
    // searches through all files in directory, including sub folders
    for (const auto &dirEntry :
         std::filesystem::recursive_directory_iterator(pathToParse)) {

      // TODO: for now, just text files, will want to extend to other file types
      const std::string currentFileExtension =
          dirEntry.path().extension().string();

      auto extensionIterator =
          std::find(supportedFileExtensions.begin(),
                    supportedFileExtensions.end(), currentFileExtension);

      if (dirEntry.is_regular_file() &&
          extensionIterator != supportedFileExtensions.end()) {
        pathsToProcess.push_back(dirEntry.path());
      }
    }
  } else if (std::filesystem::is_regular_file(pathToParse)) {
    pathsToProcess.push_back(pathToParse);
  }

  // we want to mirror mirror the input folder structure in the output folder
  // all json files should exist in output folder if specified so
  for (std::string pathToProcess : pathsToProcess) {
    std::filesystem::path mirroredOutputPath{outputRootPath};
    mirroredOutputPath.append(pathToProcess);

    // remove input root to replace with output root
    int indexOfDivider = pathToProcess.find("/");

    std::string pathWithoutInputRoot = pathToProcess.substr(
        indexOfDivider + 1, pathToProcess.length() - indexOfDivider - 1);

    pathWithoutInputRoot =
        std::filesystem::path(pathWithoutInputRoot).replace_extension("json");

    std::string outputPath = outputRootPath + "/" + pathWithoutInputRoot;

    // Parses the given file using the python parser
    std::cout << "output path: " << outputPath << "\n";
    const std::string commandToExecute = "python3 ./parser.py --path " +
                                         pathToProcess + " --output_path " +
                                         outputPath;
    int result = system(commandToExecute.c_str());

    // Failed to run parser
    if (result != 0) {
      return -1;
    }
  }

  // Once parser.py is comeplete, a new file called 'data.json' is created
  // File 'data.json' contains the BSplineDataDict in the form of a HashMap
  // The following code extracts that and stores it in a BSplineDataDict
  // (unordered_map)
  BSplineDataDict data;
  std::string filepathToExtract = "data.json";

  if (extractFileData(filepathToExtract, &data) != 0) {
    return -1;
  }

  // BSplineData structuredBsplingData{data};

  // structuredBsplingData.printControlPoints();
  // structuredBsplingData.printDegrees();
  // structuredBsplingData.printKnots();

  /* ----- Code used to print out the contents of the data BSplineData array
   * ----- */
  // std::cout << "------------------------------------" << std::endl;
  // std::cout << "Printing out extracted data contents" << std::endl;
  // for (const auto& pair : data) {
  //       std::cout << pair.first << ": " << pair.second << std::endl;
  // }
  // std::cout << "------------------------------------" << std::endl;

  // std::cout << "data: " << data << "\n";

  // std::cout << "degrees custom getter: " << getDegree(data) << "\n";
  // std::cout << "degrees custom getter: " << data["Control Points"] << "\n";

  /* ----- TESTING NODE ORDERING / MESH CONNECTIVITY  ----- */

  int numNodesX = 5;
  int numNodesY = 5;
  int numNodesZ = 5;
  generateHexahedralGrid(numNodesX * upSample, numNodesY * upSample,
                         numNodesZ * upSample, "hexahedral_mesh.vtu");
  generateWireframe(numNodesX, numNodesY, numNodesZ, "wireframe_mesh.vtu");

  /* ----- END NODE ORDERING / MESH CONNECTIVITY TESTING ----- */

  // Attempt at hooking up data to evaluator
  const int n_kdims = 1;
  const int n_cdims = 1;
  std::vector<size_t> degrees{data.at("Degree")};
  std::vector<std::vector<scalar_t>> knots{data.at("Knots")};
  std::vector<ctrl_t> control_points{data.at("Control Points")};

  BSplineGeometry bsplineEvaluator{n_kdims, n_cdims, degrees, knots,
                                   control_points};
  bsplineEvaluator.evaluate();

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
IntegerMatrix createGlobalPoints(int numNodes[]) {
  // Increments each number by 1 to deal with orgin point
  numNodes[0] += 1;
  numNodes[1] += 1;
  numNodes[2] += 1;

  // Generates the global points array
  int totalPoints = numNodes[0] * numNodes[1] * numNodes[2];
  IntegerMatrix grid(totalPoints, IntegerArray(3));
  int count = 0;
  for (int i = 0; i < numNodes[0]; i++) {
    for (int j = 0; j < numNodes[1]; j++) {
      for (int k = 0; k < numNodes[2]; k++) {
        grid[count][0] = (static_cast<double>(i) / (numNodes[0] - 1));
        grid[count][1] = (static_cast<double>(j) / (numNodes[1] - 1));
        grid[count][2] = (static_cast<double>(k) / (numNodes[2] - 1));
        count++;
      }
    }
  }

  // Decrement each number by 1 to undo what I did earlier
  numNodes[0] -= 1;
  numNodes[1] -= 1;
  numNodes[2] -= 1;

  return grid;
}

IntegerMatrix createHexahedronCellsArray(int numNodes[]) {
  int numPointsX = numNodes[0];
  int numPointsY = numNodes[1];
  int numPointsZ = numNodes[2];
  int zBase = numNodes[2] + 1;
  int yBase = zBase * (numNodes[1] + 1);

  // Number of hexahedron cells
  int totalCells = numPointsX * numPointsY * numPointsZ;
  IntegerMatrix grid(totalCells,
                     IntegerArray(8)); // Each hexahedron has 8 points
  int count = 0;

  for (int i = 0; i < numPointsZ; i++) {     // Loop through x dimension
    for (int j = 0; j < numPointsX; j++) {   // Loop through y dimension
      for (int k = 0; k < numPointsY; k++) { // Loop through z dimension
        // Calculate the indices of the 8 points of the hexahedron
        grid[count][0] = i + (yBase * j) + (zBase * k);
        grid[count][1] = grid[count][0] + yBase;
        grid[count][2] = grid[count][1] + zBase;
        grid[count][3] = i + zBase + (yBase * j) + (zBase * k);
        grid[count][4] = grid[count][0] + 1;
        grid[count][5] = grid[count][1] + 1;
        grid[count][6] = grid[count][2] + 1;
        grid[count][7] = grid[count][3] + 1;

        count++;
      }
    }
  }

  return grid;
}

// TODO: Split up code into different methods serving different functionalities
void generateHexahedralGrid(int numX, int numY, int numZ,
                            const std::string filename) {
  int numNodes[3] = {numX, numY, numZ};

  // Global Points Array Creation
  int globalArrayLength = (numX + 1) * (numY + 1) * (numZ + 1);
  IntegerMatrix globalPoints = createGlobalPoints(numNodes);
  /* USE TO PRINT OUT THE GLOBAL POINTS ARRAY */
  // std::cout << "------------------------------------" << std::endl;
  // std::cout << "Global Points Array" << std::endl;
  // for (int i = 0; i < globalArrayLength; i++) {
  //   std::cout << "[" << globalPoints[i][0] << ", ";
  //   std::cout << globalPoints[i][1] << ", ";
  //   std::cout << globalPoints[i][2] << "]\n";
  // }
  // std::cout << "------------------------------------" << std::endl;

  vtkSmartPointer<vtkUnstructuredGrid> grid =
      vtkSmartPointer<vtkUnstructuredGrid>::New();

  // Create VTK points container and fill it out with global points
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  for (int i = 0; i < globalArrayLength; i++) {
    points->InsertNextPoint(globalPoints[i][0], globalPoints[i][1],
                            globalPoints[i][2]);
  }
  grid->SetPoints(points);

  // Cell Points Array Creation
  int cellArrayLength = numX * numY * numZ;
  IntegerMatrix cellPoints = createHexahedronCellsArray(numNodes);
  // /* USE TO PRINT OUT THE GLOBAL POINTS ARRAY */
  // std::cout << "------------------------------------" << std::endl;
  // std::cout << "Cell Points Array" << std::endl;
  // for (int i = 0; i < cellArrayLength; i++) {
  //   std::cout << "[" << cellPoints[i][0] << ", ";
  //   std::cout << cellPoints[i][1] << ", ";
  //   std::cout << cellPoints[i][2] << ", ";
  //   std::cout << cellPoints[i][3] << ", ";
  //   std::cout << cellPoints[i][4] << ", ";
  //   std::cout << cellPoints[i][5] << ", ";
  //   std::cout << cellPoints[i][6] << ", ";
  //   std::cout << cellPoints[i][7] << "]\n";
  // }
  // std::cout << "------------------------------------" << std::endl;

  // Create hexahedral cells
  vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
  for (int i = 0; i < cellArrayLength; i++) {
    vtkSmartPointer<vtkHexahedron> hexahedron =
        vtkSmartPointer<vtkHexahedron>::New();

    hexahedron->GetPointIds()->SetId(0, cellPoints[i][0]); // Front-bottom-left
    hexahedron->GetPointIds()->SetId(1, cellPoints[i][1]); // Front-bottom-right
    hexahedron->GetPointIds()->SetId(2, cellPoints[i][2]); // Back-bottom-right
    hexahedron->GetPointIds()->SetId(3, cellPoints[i][3]); // Back-bottom-left
    hexahedron->GetPointIds()->SetId(4, cellPoints[i][4]); // Front-top-left
    hexahedron->GetPointIds()->SetId(5, cellPoints[i][5]); // Front-top-right
    hexahedron->GetPointIds()->SetId(6, cellPoints[i][6]); // Back-top-right
    hexahedron->GetPointIds()->SetId(7, cellPoints[i][7]); // Back-top-left

    cells->InsertNextCell(hexahedron);
  }
  grid->SetCells(VTK_HEXAHEDRON, cells);

  // Write to a .vtu file
  vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer =
      vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
  writer->SetFileName(filename.c_str());
  writer->SetInputData(grid);
  writer->Write();

  std::cout << "VTU file saved as: " << filename << std::endl;
}

void generateWireframe(int numX, int numY, int numZ,
                       const std::string filename) {
  // Objects to be stored in the resulting ".vtu" file
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
  vtkSmartPointer<vtkUnstructuredGrid> grid =
      vtkSmartPointer<vtkUnstructuredGrid>::New();

  // Create global points and insert them into the ".vtu" file
  int numNodes[3] = {numX * upSample, numY * upSample, numZ * upSample};
  int globalArrayLength =
      (numNodes[0] + 1) * (numNodes[1] + 1) * (numNodes[2] + 1);
  IntegerMatrix globalPoints = createGlobalPoints(numNodes);
  for (int i = 0; i < globalArrayLength; i++) {
    points->InsertNextPoint(globalPoints[i][0], globalPoints[i][1],
                            globalPoints[i][2]);
  }
  grid->SetPoints(points);

  // Connect all points along X-axis @ numZ interval (@ y == 0 && y == 1)
  for (int i = 0; i <= 1; i++) { // y == 0 and y == 1
    int y_offset = (i * (numNodes[1] * (numNodes[2] + 1)));
    for (int j = 0; j < numZ; j++) { // numZ interval
      int z_offset = (j * numZ);
      int x_offset = ((numNodes[1] + 1) * (numNodes[2] + 1));
      for (int k = 0; k < numNodes[0]; k++) { // all x-points along line
        int pointIndex1 = y_offset + z_offset + (x_offset * k);
        int pointIndex2 = pointIndex1 + x_offset;

        // Create a line between 2 points
        vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
        line->GetPointIds()->SetId(0, pointIndex1); // start point ID
        line->GetPointIds()->SetId(1, pointIndex2);

        // Insert line into lines Cell Array
        lines->InsertNextCell(line);
      }
    }
  }

  // Connect all points along X-axis @ numY interval (@ z == 0 && z == 1)
  for (int i = 0; i <= 1; i++) { // z == 0 and z == 1
    int z_offset = (i * numNodes[2]);
    for (int j = 1; j < numY - 1;
         j++) { // numY interval (Start and end slightly different to prevent
                // re-doing edge lines)
      int y_offset = (j * (numY * (numNodes[2] + 1)));
      int x_offset = ((numNodes[1] + 1) * (numNodes[2] + 1));
      for (int k = 0; k < numNodes[0]; k++) { // all x-points along line
        int pointIndex1 = y_offset + z_offset + (x_offset * k);
        int pointIndex2 = pointIndex1 + x_offset;

        // Create a line between 2 points
        vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
        line->GetPointIds()->SetId(0, pointIndex1); // start point ID
        line->GetPointIds()->SetId(1, pointIndex2);

        // Insert line into lines Cell Array
        lines->InsertNextCell(line);
      }
    }
  }

  // TODO: Connect all points along Y-axis @ numX interval (@ z == 0 && z == 1)
  for (int i = 0; i <= 1; i++) { // z == 0 and z == 1
    int z_offset = (i * numNodes[2]);
    for (int j = 0; j < numX; j++) { // numX interval
      int x_offset = (j * numX * ((numNodes[1] + 1) * (numNodes[2] + 1)));
      int y_offset = (numNodes[2] + 1);
      for (int k = 0; k < numNodes[1]; k++) {
        int pointIndex1 = (y_offset * k) + z_offset + x_offset;
        int pointIndex2 = pointIndex1 + y_offset;

        // Create a line between 2 points
        vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
        line->GetPointIds()->SetId(0, pointIndex1); // start point ID
        line->GetPointIds()->SetId(1, pointIndex2);

        // Insert line into lines Cell Array
        lines->InsertNextCell(line);
      }
    }
  }

  // TODO: Connect all points along Y-axis @ numZ interval (@ x == 0 && x == 1)
  for (int i = 0; i <= 1; i++) { // x == 0 and x == 1
    int x_offset = (i * ((numNodes[1] + 1) * (numNodes[2] + 1) * numNodes[0]));
    for (int j = 1; j < numZ - 1;
         j++) { // numZ interval (Start and end slightly different to prevent
                // re-doing edge lines)
      int z_offset = (j * numZ);
      int y_offset = (numNodes[2] + 1);
      for (int k = 0; k < numNodes[1]; k++) {
        int pointIndex1 = (y_offset * k) + z_offset + x_offset;
        int pointIndex2 = pointIndex1 + y_offset;

        // Create a line between 2 points
        vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
        line->GetPointIds()->SetId(0, pointIndex1); // start point ID
        line->GetPointIds()->SetId(1, pointIndex2);

        // Insert line into lines Cell Array
        lines->InsertNextCell(line);
      }
    }
  }

  // TODO: Connect all points along Z-axis @ numX interval (@ y == 0 && y == 1)
  for (int i = 0; i <= 1; i++) { // y == 0 and y == 1
    int y_offset = (i * (numNodes[1] * (numNodes[2] + 1)));
    for (int j = 0; j < numX; j++) { // numX interval
      int x_offset = (j * numX * ((numNodes[1] + 1) * (numNodes[2] + 1)));
      int z_offset = 0;
      for (int k = 0; k < numNodes[1]; k++) {
        int pointIndex1 = y_offset + (z_offset + k) + x_offset;
        int pointIndex2 = pointIndex1 + 1;

        // Create a line between 2 points
        vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
        line->GetPointIds()->SetId(0, pointIndex1); // start point ID
        line->GetPointIds()->SetId(1, pointIndex2);

        // Insert line into lines Cell Array
        lines->InsertNextCell(line);
      }
    }
  }

  // TODO: Connect all points along Z-axis @ numY interval (@ x == 0 && x == 1)
  for (int i = 0; i <= 1; i++) { // x == 0 and x == 1
    int x_offset = (i * ((numNodes[1] + 1) * (numNodes[2] + 1) * numNodes[0]));
    for (int j = 1; j < numY - 1;
         j++) { // numY interval (Start and end slightly different to prevent
                // re-doing edge lines)
      int y_offset = (j * (numY * (numNodes[2] + 1)));
      int z_offset = 0;
      for (int k = 0; k < numNodes[1]; k++) {
        int pointIndex1 = y_offset + (z_offset + k) + x_offset;
        int pointIndex2 = pointIndex1 + 1;

        // Create a line between 2 points
        vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
        line->GetPointIds()->SetId(0, pointIndex1); // start point ID
        line->GetPointIds()->SetId(1, pointIndex2);

        // Insert line into lines Cell Array
        lines->InsertNextCell(line);
      }
    }
  }

  grid->SetCells(VTK_LINE, lines);

  // Write to VTU
  vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer =
      vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
  writer->SetFileName("wireframe_lines.vtu");
  writer->SetInputData(grid);
  writer->Write();

  std::cout << "Wireframe VTU file created.\n";
}