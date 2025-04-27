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
#include <set>
#include <vtkCellArray.h>
#include <vtkCommonDataModelModule.h>
#include <vtkHexahedron.h>
#include <vtkLine.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLUnstructuredGridWriter.h>
#include <vtkXMLUnstructuredGridReader.h>

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
int generateWireframeForFile(const std::string filename, const std::string outputFilename);
int extractingNecessaryPoints(const std::string filename); // TEMPORARY: Testing extracting certain points

int getDegree(BSplineDataDict bsplineData);
int upSample = 20;

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

  int numNodesX = 5;
  int numNodesY = 3;
  int numNodesZ = 2;
  //generateHexahedralGrid(numNodesX * upSample, numNodesY * upSample, numNodesZ * upSample, "hexahedral_mesh.vtu");
  generateWireframe(numNodesX, numNodesY, numNodesZ, "wireframe_mesh.vtu");

  generateWireframeForFile("quadratic_bspline_example_10x10x4.vtu", "testing.vtu");
  extractingNecessaryPoints("quadratic_bspline_example_10x10x4.vtu");

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
  IntegerMatrix grid(totalCells, IntegerArray(8)); // Each hexahedron has 8 points
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
                            const std::string filename){
    // Objects to be stored in the resulting ".vtu" file
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkUnstructuredGrid> grid = vtkSmartPointer<vtkUnstructuredGrid>::New();

    // Create global points and insert them into the ".vtu" file
    int numNodes[3] = {numX * upSample, numY * upSample, numZ * upSample};
    int globalArrayLength = (numNodes[0] + 1) * (numNodes[1] + 1) * (numNodes[2] + 1);
    IntegerMatrix globalPoints = createGlobalPoints(numNodes);
    for (int i = 0; i < globalArrayLength; i++) {
    points->InsertNextPoint(globalPoints[i][0], globalPoints[i][1],
                            globalPoints[i][2]);
    }
    grid->SetPoints(points);

    // Connect all points along X-axis @ numZ interval (@ y == 0 && y == 1)
     for(int i = 0; i <= 1; i++){ // y == 0 and y == 1
       int y_offset = (i * (numNodes[1] * (numNodes[2] + 1)));
       for(int j = 0; j < numZ + 1; j++){ // numZ interval
         int z_offset = (j * upSample);
         int x_offset = ((numNodes[1] + 1) * (numNodes[2] + 1));
         for(int k = 0; k < numNodes[0]; k++){ // all x-points along line
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
     for(int i = 0; i <= 1; i++){ // z == 0 and z == 1
       int z_offset = (i * numNodes[2]);
       for(int j = 1; j < numY; j++){ // numY interval (Start and end slightly different to prevent re-doing edge lines)
         int y_offset = (j * (upSample * (numNodes[2] + 1)));
         int x_offset = ((numNodes[1] + 1) * (numNodes[2] + 1));
         for(int k = 0; k < numNodes[0]; k++){ // all x-points along line
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


    // Connect all points along Y-axis @ numX interval (@ z == 0 && z == 1)
     for(int i = 0; i <= 1; i++){ // z == 0 and z == 1
       int z_offset = (i * numNodes[2]);
       for(int j = 0; j < numX + 1; j++){ // numX interval
         int x_offset = (j * upSample * ((numNodes[1] + 1) * (numNodes[2] + 1)));
         int y_offset = (numNodes[2] + 1);
         for(int k = 0; k < numNodes[1]; k++){
           int pointIndex1 = (y_offset * k)+ z_offset + x_offset;
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


    // Connect all points along Y-axis @ numZ interval (@ x == 0 && x == 1)
     for(int i = 0; i <= 1; i++){ // x == 0 and x == 1
       int x_offset = (i * ((numNodes[1] + 1) * (numNodes[2] + 1) * numNodes[0]));
       for(int j = 1; j < numZ; j++){ // numZ interval (Start and end slightly different to prevent re-doing edge lines)
         int z_offset = (j * upSample);
         int y_offset = (numNodes[2] + 1);
         for(int k = 0; k < numNodes[1]; k++){
           int pointIndex1 = (y_offset * k)+ z_offset + x_offset;
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


    // Connect all points along Z-axis @ numX interval (@ y == 0 && y == 1)
     for(int i = 0; i <= 1; i++){ // y == 0 and y == 1
       int y_offset = (i * (numNodes[1] * (numNodes[2] + 1)));
       for(int j = 0; j < numX + 1; j++){ // numX interval
         int x_offset = (j * upSample * ((numNodes[1] + 1) * (numNodes[2] + 1)));
         int z_offset = 0;
         for(int k = 0; k < numNodes[2]; k++){
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


    // Connect all points along Z-axis @ numY interval (@ x == 0 && x == 1)
    for(int i = 0; i <= 1; i++){ // x == 0 and x == 1
      int x_offset = (i * ((numNodes[1] + 1) * (numNodes[2] + 1) * numNodes[0]));
      for(int j = 1; j < numY; j++){ // numY interval (Start and end slightly different to prevent re-doing edge lines)
        int y_offset = (j * (upSample * (numNodes[2] + 1)));
        int z_offset = 0;
        for(int k = 0; k < numNodes[2]; k++){
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
    vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
    writer->SetFileName("wireframe_lines.vtu");
    writer->SetInputData(grid);
    writer->Write();

    std::cout << "Wireframe VTU file created.\n";
}

int generateWireframeForFile(const std::string filename, const std::string outputFilename) {
    if (!(std::filesystem::exists(filename))) {
        std::cerr << "Error: File '" << filename << "' does not exist." << std::endl;
        return -1;
    }

     // Read the .vtu file
     vtkSmartPointer<vtkXMLUnstructuredGridReader> reader = vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
     reader->SetFileName(filename.c_str());
     reader->Update();

     // Get the unstructured grid and points from file
     vtkUnstructuredGrid* unstructuredGrid = reader->GetOutput();
     vtkPoints* points = unstructuredGrid->GetPoints();
     vtkIdType numPoints = points->GetNumberOfPoints();
     std::cout << "Number of Points: " << numPoints << std::endl;

    // Creates containers for new files
    vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkUnstructuredGrid> newUnstructuredGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();

    // Store points in a new vtkPoints object for writing
     int numX = 0;
     int numY = 0;
     bool numYFound = false;
     int numZ = 0;
     bool numZFound = false;
      for (int i = 0; i < numPoints; i++) {
          double p[3];
          points->GetPoint(i, p);
          if(p[1] != 0 && !numZFound){
            numZ = i;
            numZFound = true;
          } 
          if(p[0] != 0 && !numYFound){
            numY = i / numZ;
            numYFound = true;
          }
      } 
     newUnstructuredGrid->SetPoints(points);
     numX = (numPoints / (numY * numZ)) - 1;
     numY -= 1;
     numZ -= 1;
     std::cout << "numX: " << numX << ", numY: " << numY << ", numZ: " << numZ << std::endl;


    /* ENTIRE AREA IS FOR CONNECTING THE POINTS */

    int numNodes[3] = {numX, numY, numZ};

    // Connect all points along X-axis @ numZ interval (@ y == 0 && y == 1)
     for(int i = 0; i <= 1; i++){ // y == 0 and y == 1
       int y_offset = (i * (numNodes[1] * (numNodes[2] + 1)));
       for(int j = 0; j < numZ + 1; j++){ // numZ interval
         int z_offset = j;
         int x_offset = ((numNodes[1] + 1) * (numNodes[2] + 1));
         for(int k = 0; k < numNodes[0]; k++){ // all x-points along line
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
       for(int i = 0; i <= 1; i++){ // z == 0 and z == 1
         int z_offset = (i * numNodes[2]);
         for(int j = 1; j < numY; j++){ // numY interval (Start and end slightly different to prevent re-doing edge lines)
           int y_offset = (j * ((numNodes[2] + 1)));
           int x_offset = ((numNodes[1] + 1) * (numNodes[2] + 1));
           for(int k = 0; k < numNodes[0]; k++){ // all x-points along line
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


    // Connect all points along Y-axis @ numX interval (@ z == 0 && z == 1)
       for(int i = 0; i <= 1; i++){ // z == 0 and z == 1
        int z_offset = (i * numNodes[2]);
        for(int j = 0; j < numX + 1; j++){ // numX interval
          int x_offset = (j * ((numNodes[1] + 1) * (numNodes[2] + 1)));
          int y_offset = (numNodes[2] + 1);
          for(int k = 0; k < numNodes[1]; k++){
            int pointIndex1 = (y_offset * k)+ z_offset + x_offset;
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


    // Connect all points along Y-axis @ numZ interval (@ x == 0 && x == 1)
      for(int i = 0; i <= 1; i++){ // x == 0 and x == 1
        int x_offset = (i * ((numNodes[1] + 1) * (numNodes[2] + 1) * numNodes[0]));
        for(int j = 1; j < numZ; j++){ // numZ interval (Start and end slightly different to prevent re-doing edge lines)
          int z_offset = (j);
          int y_offset = (numNodes[2] + 1);
          for(int k = 0; k < numNodes[1]; k++){
            int pointIndex1 = (y_offset * k)+ z_offset + x_offset;
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


    // Connect all points along Z-axis @ numX interval (@ y == 0 && y == 1)
      for(int i = 0; i <= 1; i++){ // y == 0 and y == 1
        int y_offset = (i * (numNodes[1] * (numNodes[2] + 1)));
        for(int j = 0; j < numX + 1; j++){ // numX interval
          int x_offset = (j * ((numNodes[1] + 1) * (numNodes[2] + 1)));
          int z_offset = 0;
          for(int k = 0; k < numNodes[2]; k++){
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


    // Connect all points along Z-axis @ numY interval (@ x == 0 && x == 1)
     for(int i = 0; i <= 1; i++){ // x == 0 and x == 1
       int x_offset = (i * ((numNodes[1] + 1) * (numNodes[2] + 1) * numNodes[0]));
       for(int j = 1; j < numY; j++){ // numY interval (Start and end slightly different to prevent re-doing edge lines)
         int y_offset = (j * ((numNodes[2] + 1)));
         int z_offset = 0;
         for(int k = 0; k < numNodes[2]; k++){
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

    newUnstructuredGrid->SetCells(VTK_LINE, lines);

    /* DONE CONNECTING POINTS */











    // Write to the new .vtu file
    vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
    writer->SetFileName(outputFilename.c_str());
    writer->SetInputData(newUnstructuredGrid);
    writer->Write();

    std::cout << "Successfully written to " << outputFilename << std::endl;

    return EXIT_SUCCESS;
}

int extractingNecessaryPoints(const std::string filename){
  if (!(std::filesystem::exists(filename))) {
        std::cerr << "Error: File '" << filename << "' does not exist." << std::endl;
        return -1;
  }

  // Read the .vtu file
  vtkSmartPointer<vtkXMLUnstructuredGridReader> reader = vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
  reader->SetFileName(filename.c_str());
  reader->Update();

  // Get the Necessary information (Grid, Points, Connectivity)
  vtkUnstructuredGrid* unstructuredGrid = reader->GetOutput();
  vtkPoints* points = unstructuredGrid->GetPoints();
  /* THE INFO THAT NEEDS TO BE DONE */

  // Extarct Connectivity
  vtkCellArray* connectivity = unstructuredGrid->GetCells();

  // Create a set of the indecies of points that contribute to connectivity
  std::set<vtkIdType> pointIndices;
  for (vtkIdType cellId = 0; cellId < unstructuredGrid->GetNumberOfCells(); ++cellId) {
    vtkCell* cell = unstructuredGrid->GetCell(cellId);
    for (vtkIdType i = 0; i < cell->GetNumberOfPoints(); ++i) {
      pointIndices.insert(cell->GetPointId(i));
    }
  }
  size_t numGlobalPoints = pointIndices.size();

  // Create a global points array of the set size
  vtkNew<vtkPoints> globalPoints;
  globalPoints->SetNumberOfPoints(numGlobalPoints);

  // Put the points into the global points array
  vtkPoints* originalPoints = unstructuredGrid->GetPoints();

  vtkIdType newIdx = 0;
  for (auto idx : pointIndices) {
      double point[3];
      originalPoints->GetPoint(idx, point);
      globalPoints->SetPoint(newIdx, point);
      ++newIdx;
  }

  // Create a .vtu file and store points
  vtkNew<vtkUnstructuredGrid> newGrid;
  newGrid->SetPoints(globalPoints);
  vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
  writer->SetFileName("extractedNodes.vtu");
  writer->SetInputData(newGrid);
  writer->Write();

  std::cout << "Nodes thing created \n";

  generateWireframeForFile("extractedNodes.vtu", "new_wireframe.vtu");

  // TODO: Clean up code and make it more compartamentalized

  return 0;
}
