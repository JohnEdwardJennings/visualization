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
