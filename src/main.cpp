#include <iostream>
#include <string>

#include "fileInput.h"

int main(int argc, char** argv) {
  // TODO: parse arguments from argv, we want a filepath
  // to navigate to
  std::cout << "number of arguments: " << argc << "\n";

  std::cout << "Hello World!\n";

  FileInput fileInput = FileInput(std::string("hi")); 

  std::cout << "file name: " << fileInput.getFilename() << "\n";

  return 0;
}

/*
 * This method is used to gather BSpline data from a ".vtu" file
 *
 * pre: fileInput != NULL
 * post: BSplineGeometry object created from data within the input file
 */
int parse_vtu(FileInput* fileInput){
  if(fileInput == NULL){
    std::cout << "Error: Parameter 'fileInput' cannot be NULL" << "/n";
    return EXIT_FAILURE;
  }

  // TODO: Parse input file and create a BSpline Geometry from its contents
}

/*
 * This method is used to gather BSpline data from a ".pickle" file
 *
 * pre: fileInput != NULL
 * post: BSplineGeometry object created from data within the input file
 */
int parse_pickle(FileInput* fileInput){
  if(fileInput == NULL){
    std::cout << "Error: Parameter 'fileInput' cannot be NULL" << "/n";
    return EXIT_FAILURE;
  }

  // TODO: Parse input file and create a BSpline Geometry from its contents
}

/*
 * This method is used to gather BSpline data from a ".txt" file
 *
 * pre: fileInput != NULL
 * post: BSplineGeometry object created from data within the input file
 */
int parse_txt(FileInput* fileInput){
  if(fileInput == NULL){
    std::cout << "Error: Parameter 'fileInput' cannot be NULL" << "/n";
    return EXIT_FAILURE;
  }

  // TODO: Parse input file and create a BSpline Geometry from its contents
}
