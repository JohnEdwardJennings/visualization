#include <algorithm>
#include <codecvt>
#include <cstddef>
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

  std::vector<size_t> getDegree() {
    if (bsplineData.find("Degree") == bsplineData.end()) {
      return std::vector<size_t>();
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

  std::vector<std::vector<double>> getKnotsVector() {
    std::vector<std::vector<double>> knots{};

    for (auto knotVector : bsplineData["Knots"]) {
      std::vector<double> knotDouble =
          static_cast<std::vector<double>>(knotVector);

      knotVector.push_back(knotDouble);
    }

    return knots;
  }

  void printDegrees() {
    for (size_t degree : getDegree()) {
      std::cout << "degree: " << degree << "\n";
    }
  }

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
    for (std::vector<double> knot : getKnotsVector()) {
      for (double knotValue : knot) {
        std::cout << knotValue << ",";
      }
      std::cout << "\n";
    }
    std::cout << "]\n";
  }
};