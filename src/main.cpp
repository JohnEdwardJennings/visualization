#include <iostream>
#include <string>

#include "fileInput.h"

int main() {
  std::cout << "Hello World!\n";

  FileInput fileInput = FileInput(std::string("hi")); 

  std::cout << "filename: " << fileInput.getFilename() << "\n";


  return 0;
}
