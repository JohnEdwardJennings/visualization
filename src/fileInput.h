#include <string>
class FileInput {
public:
  FileInput() {}

  FileInput(std::string stringToRead) : filename(stringToRead) {}

  std::string getFilename() { return filename; }

private:
  std::string filename;
};