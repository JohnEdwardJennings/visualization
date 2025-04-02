#include "bspline/geometry.h"
#include "json.hpp"
#include <array>
#include <codecvt>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>
#include <vtkCellArray.h>
#include <vtkCommonDataModelModule.h>
#include <vtkHexahedron.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h> //Throwing error but still compiles??
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

int getDegree(BSplineDataDict bsplineData);

int main(int argc, char **argv) {
  // Retrieving the file path the of the users file
  // File path should be the path to the files that contains BSplineData
  const std::string pathToParse = std::string(argv[1]);
  if (pathToParse.empty()) {
    std::cout << "Path to parse is empty, please pass in path" << std::endl;
    return 0;
  }

  // Parses the given file using the python parser
  const std::string commandToExecute =
      "python3 ./parser.py --path " + pathToParse;
  int result = system(commandToExecute.c_str());

  // Failed to run parser
  if (result != 0) {
    return -1;
  }

  // Once parser.py is comeplete, a new file called 'data.json' is created
  // File 'data.json' contains the BSplineDataDict in the form of a HashMap
  // The following code extracts that and stores it in a BSplineDataDict
  // (unordered_map)
  BSplineDataDict data;
  std::string filepathToExtract = "data.json";
  result = extractFileData(filepathToExtract, &data);

  // Failed to extract data from ,json
  if (result != 0) {
    return -1;
  }

  BSplineData structuredBsplingData{data};

  structuredBsplingData.printControlPoints();
  structuredBsplingData.printDegrees();
  structuredBsplingData.printKnots();

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

  int numNodesX = 3;
  int numNodesY = 2;
  int numNodesZ = 5;
  generateHexahedralGrid(numNodesX, numNodesY, numNodesZ, "hexahedral_mesh.vtu");

  /* ----- END NODE ORDERING / MESH CONNECTIVITY TESTING ----- */

  // std::array<size_t, 2> degrees{2, 2};
  // std::array<std::vector<double>, 2> knot_vectors{std::vector<double>{1, 2,
  // 3, 4, 5}, std::vector<double>{1, 2, 3, 4, 5}};
  // std::vector<std::array<double, 3>> control_points(25);
  // BSplineGeometry<2,3> b(degrees, knot_vectors, control_points);
  // std::vector<std::array<double,2>> x = {{0, 1}, {2, 3}};
  // b.evaluate(x);

  // Attempt at hooking up data to evaluator
  // std::array<size_t, 1> degrees{1};
  // std::array<std::vector<double>, 1> knotVectors{
  //     structuredBsplingData.getKnotsVector()};
  // std::vector<std::array<double, 3>> controlPoints(25);

  // BSplineGeometry<1, 3> b{degrees, knotVectors, controlPoints};

  // auto splines = b.evaluate(structuredBsplingData.getKnotsVector());

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

void generateHexahedralGrid(int numX, int numY, int numZ,
                            const std::string filename) {
  int numNodes[3] = {numX, numY, numZ};

  // Global Points Array Creation
  int globalArrayLength = (numX + 1) * (numY + 1) * (numZ + 1);
  IntegerMatrix globalPoints = createGlobalPoints(numNodes);
  /* USE TO PRINT OUT THE GLOBAL POINTS ARRAY */
  std::cout << "------------------------------------" << std::endl;
  std::cout << "Global Points Array" << std::endl;
  for (int i = 0; i < globalArrayLength; i++) {
    std::cout << "[" << globalPoints[i][0] << ", ";
    std::cout << globalPoints[i][1] << ", ";
    std::cout << globalPoints[i][2] << "]\n";
  }
  std::cout << "------------------------------------" << std::endl;

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
  /* USE TO PRINT OUT THE GLOBAL POINTS ARRAY */
  std::cout << "------------------------------------" << std::endl;
  std::cout << "Cell Points Array" << std::endl;
  for (int i = 0; i < cellArrayLength; i++) {
    std::cout << "[" << cellPoints[i][0] << ", ";
    std::cout << cellPoints[i][1] << ", ";
    std::cout << cellPoints[i][2] << ", ";
    std::cout << cellPoints[i][3] << ", ";
    std::cout << cellPoints[i][4] << ", ";
    std::cout << cellPoints[i][5] << ", ";
    std::cout << cellPoints[i][6] << ", ";
    std::cout << cellPoints[i][7] << "]\n";
  }
  std::cout << "------------------------------------" << std::endl;

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