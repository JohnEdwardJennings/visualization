#include <algorithm>
#include <codecvt>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "json.hpp"

using json = nlohmann::json;

class BSplineData {
private:
  std::unordered_map<std::string, json> bsplineData;

public:
  BSplineData(std::unordered_map<std::string, json> data) {
    bsplineData = data;
  }

  int getDegree() {
    if (bsplineData.find("Degree") == bsplineData.end()) {
      return -1;
    }

    return bsplineData.at("Degree");
  }

  std::vector<std::vector<double>> getControlPoints() {
    std::vector<std::vector<double>> controlPoints;

    if (bsplineData.find("Control Points") == bsplineData.end()) {
      return controlPoints;
    }

    for (auto controlPointList : bsplineData["Control Points"]) {
      std::vector<double> controlPointPerList;
      for (auto controlPoint : controlPointList) {
        controlPointPerList.push_back(controlPoint);
      }
      controlPoints.push_back(controlPointPerList);
    }

    return controlPoints;
  }

  std::vector<double> getKnotsVector() {
    std::vector<double> knotVector;

    for (auto knot : bsplineData["Knots"]) {
      double knotDouble = static_cast<double>(knot);

      knotVector.push_back(knotDouble);
    }

    return knotVector;
  }

  void printDegrees() { std::cout << "degrees: " << getDegree() << "\n"; }

  void printControlPoints() {
    std::cout << "[";
    for (std::vector<double> controlList : getControlPoints()) {
      std::cout << "[";
      for (double controlPoint : controlList) {
        std::cout << controlPoint << ",";
      }
      std::cout << "]\n";
    }
    std::cout << "]\n";
  }

  void printKnots() {
    std::cout << "knots\n [";
    for (double knot : getKnotsVector()) {
      std::cout << knot << ",";
    }
    std::cout << "]\n";
  }
};