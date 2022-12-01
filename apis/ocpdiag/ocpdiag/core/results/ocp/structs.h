// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef OCPDIAG_CORE_RESULTS_OCP_STRUCTS_H_
#define OCPDIAG_CORE_RESULTS_OCP_STRUCTS_H_

#include <string>
#include <variant>

#include "absl/strings/str_cat.h"

namespace ocpdiag::results {

// The structs in this file are representations of the JSON output of the
// results library. See
// https://github.com/opencomputeproject/ocp-diag-core/tree/main/json_spec for
// detailed documentation.

enum class ValidatorType {
  kUnspecified = 0,
  kEqual = 1,
  kNotEqual = 2,
  kLessThan = 3,
  kLessThanOrEqual = 4,
  kGreaterThan = 5,
  kGreaterThanOrEqual = 6,
  kRegexMatch = 7,
  kRegexNoMatch = 8,
  kInSet = 9,
  kNotInSet = 10,
};

struct Validator {
  ValidatorType type;                             // Required
  std::variant<std::string, double, bool> value;  // Required
  std::string name;
};

class DutInfo;

struct HardwareInfo {
  std::string name;  // Required
  std::string computer_system;
  std::string location;
  std::string odata_id;
  std::string part_number;
  std::string serial_number;
  std::string manager;
  std::string manufacturer;
  std::string manufacturer_part_number;
  std::string part_type;
  std::string version;
  std::string revision;
};

class RegisteredHardwareInfo {
 public:
  RegisteredHardwareInfo() = default;

 private:
  friend class DutInfo;
  std::string id_;
};

enum class SoftwareType {
  kUnspecified = 0,
  kFirmware = 1,
  kSystem = 2,
  kApplication = 3
};

struct SoftwareInfo {
  std::string name;  // Required
  std::string version;
  std::string revision;
  SoftwareType software_type;
};

class RegisteredSoftwareInfo {
 public:
  RegisteredSoftwareInfo() = default;

 private:
  friend class DutInfo;
  std::string id_;
};

struct PlatformInfo {
  std::string info;  // Required
};

enum class SubcomponentType {
  kUnspecified = 0,
  kAsic = 1,
  kAsicSubsystem = 2,
  kBus = 3,
  kFunction = 4,
  kConnector = 5,
};

struct Subcomponent {
  std::string name;  // Required
  SubcomponentType type;
  std::string location;
  std::string version;
  std::string revision;
};

struct MeasurementInfo {
  std::string name;  // Required
  std::string unit;
  RegisteredHardwareInfo* hardware_info = nullptr;
  Subcomponent* subcomponent = nullptr;
  std::vector<Validator> validators;
};

enum class DiagnosisType {
  kUnknown = 0,
  kPass = 1,
  kFail = 2,
};

struct Diagnosis {
  std::string verdict;  // Required
  DiagnosisType type;   // Required
  std::string message;
  RegisteredHardwareInfo* hardware_info = nullptr;
  Subcomponent* subcomponent = nullptr;
};

struct Error {
  std::string symptom;  // Required
  std::string message;
  std::vector<RegisteredSoftwareInfo> software_infos;
};

struct File {
  std::string display_name;  // Required
  std::string uri;           // Required
  bool is_snapshot;          // Required
  std::string description;
  std::string content_type;
};

struct TestRunInfo {
  static TestRunInfo FromMainArgs(const std::string& name,
                                  const std::string& version, int* argc,
                                  const char* argv[]) {
    std::string command_line = argv[0];
    for (int i = 1; i < *argc; i++) {
      absl::StrAppend(&command_line, " ");
      absl::StrAppend(&command_line, argv[i]);
    }
    return {name, version, command_line};
  }

  std::string name;          // Required
  std::string version;       // Required
  std::string command_line;  // Required
};

}  // namespace ocpdiag::results

#endif  // OCPDIAG_CORE_RESULTS_OCP_STRUCTS_H_
