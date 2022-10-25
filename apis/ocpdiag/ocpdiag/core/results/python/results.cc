// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/results.h"

#include <pybind11/functional.h>
#include <pybind11/pybind11.h>

#include "google/protobuf/empty.pb.h"
#include "google/protobuf/struct.pb.h"
#include "absl/flags/flag.h"
#include "ocpdiag/core/results/internal/logging.h"
#include "ocpdiag/core/results/results.pb.h"
#include "pybind11_abseil/absl_casters.h"
#include "pybind11_abseil/status_casters.h"
#include "pybind11_protobuf/native_proto_caster.h"

namespace ocpdiag::results {

void SetFlags(const bool ocpdiag_copy_results_to_stdout,
              const std::string ocpdiag_results_filepath,
              const std::string machine_under_test,
              const bool alsologtoocpdiagresults,
              const bool ocpdiag_strict_reporting) {
  absl::SetFlag(&FLAGS_ocpdiag_copy_results_to_stdout,
                ocpdiag_copy_results_to_stdout);
  absl::SetFlag(&FLAGS_ocpdiag_results_filepath, ocpdiag_results_filepath);
  absl::SetFlag(&FLAGS_machine_under_test, machine_under_test);
  absl::SetFlag(&FLAGS_alsologtoocpdiagresults, alsologtoocpdiagresults);
  absl::SetFlag(&FLAGS_ocpdiag_strict_reporting, ocpdiag_strict_reporting);
}

PYBIND11_MODULE(_results, m) {
  pybind11::google::ImportStatusModule();
  pybind11_protobuf::ImportNativeProtoCasters();

  m.def("SetResultsLibFlags", &SetFlags);

  m.doc() = "Bindings for the OCPDiag results library";

  // DEPRECATED: Use TestRun().
  m.def(
      "InitTestRun",
      [](const std::string& name) -> absl::StatusOr<std::shared_ptr<TestRun>> {
        return ResultApi().InitializeTestRun(name);
      },
      pybind11::arg("name"));
  pybind11::class_<internal::ArtifactWriter> artifact_writer(m,
                                                             "ArtifactWriter");
  pybind11::class_<TestRun, std::shared_ptr<TestRun>>(m, "TestRun")
      .def(pybind11::init(
               // The constructor allows optionally injecting an artifactwriter.
               [](absl::string_view name, internal::ArtifactWriter* writer) {
                 if (writer) return std::make_unique<TestRun>(name, *writer);
                 return std::make_unique<TestRun>(name);
               }),
           pybind11::arg("name"),
           pybind11::arg("writer").none(true) = pybind11::none())
      .def(
          "StartAndRegisterInfos",
          [](TestRun& a, absl::Span<const DutInfo> dutinfos,
             const google::protobuf::Message* params) {
            if (params == nullptr) {
              a.StartAndRegisterInfos(dutinfos, google::protobuf::Empty());
            } else {
              a.StartAndRegisterInfos(dutinfos, *params);
            }
          },
          pybind11::arg("dutinfos"),
          pybind11::arg("params").none(true) = pybind11::none())
      .def("End", [](TestRun& a) { return static_cast<int>(a.End()); })
      .def("Skip", [](TestRun& a) { return static_cast<int>(a.Skip()); })
      .def("AddError", &TestRun::AddError)
      .def("AddTag", &TestRun::AddTag)
      .def("Status",
           [](const TestRun& a) { return static_cast<int>(a.Status()); })
      .def("Result", &TestRun::Result)
      .def("Started", &TestRun::Started)
      .def("Ended", &TestRun::Ended)
      .def("LogDebug", &TestRun::LogDebug)
      .def("LogInfo", &TestRun::LogInfo)
      .def("LogWarn", &TestRun::LogWarn)
      .def("LogError", &TestRun::LogError)
      .def("LogFatal", &TestRun::LogFatal)
      // These support using the TestRun as a Python context manager.
      .def("__enter__", [](TestRun* self) { return self; })
      .def("__exit__", [](TestRun* self, pybind11::args) { self->End(); });

  // DEPRECATED: Use TestRun::BeginTestStep().
  m.def(
      "BeginTestStep",
      [](TestRun* parent, std::string name) {
        return ResultApi().BeginTestStep(parent, name);
      },
      pybind11::arg("parent"), pybind11::arg("name"));

  pybind11::class_<TestStep>(m, "TestStep")
      .def(pybind11::init<absl::string_view, TestRun&>(), pybind11::arg("name"),
           pybind11::arg("test_run"))
      .def(
          "AddDiagnosis",
          [](TestStep& a, const int symptom_type, std::string symptom,
             std::string message, absl::Span<const HwRecord> records) {
            return a.AddDiagnosis(
                ocpdiag::results_pb::Diagnosis::Type(symptom_type),
                symptom, message, records);
          },
          pybind11::arg("type"), pybind11::arg("symptom"),
          pybind11::arg("message"),
          pybind11::arg("records") = absl::Span<const HwRecord>{})
      .def("AddError", &TestStep::AddError, pybind11::arg("symptom"),
           pybind11::arg("message"),
           pybind11::arg("records") = absl::Span<const SwRecord>{})
      .def("AddMeasurement", &TestStep::AddMeasurement, pybind11::arg("info"),
           pybind11::arg("elem"),
           pybind11::arg("hwRecord").none(true) = pybind11::none())
      .def("AddFile", &TestStep::AddFile)
      .def("AddArtifactExtension", &TestStep::AddArtifactExtension)
      .def("LogDebug", &TestStep::LogDebug)
      .def("LogInfo", &TestStep::LogInfo)
      .def("LogWarn", &TestStep::LogWarn)
      .def("LogError", &TestStep::LogError)
      .def("LogFatal", &TestStep::LogFatal)
      .def("End", &TestStep::End)
      .def("Skip", &TestStep::Skip)
      .def("Ended", &TestStep::Ended)
      .def("Status", [](TestStep& a) { return static_cast<int>(a.Status()); })
      .def("Id", &TestStep::Id)
      // These support using the TestStep as a Python context manager.
      .def("__enter__", [](TestStep* self) { return self; })
      .def("__exit__", [](TestStep* self, pybind11::args) { self->End(); });

  pybind11::class_<DutInfo>(m, "DutInfo")
      .def(pybind11::init<std::string>())
      .def("AddHardware", &DutInfo::AddHardware)
      .def("AddSoftware", &DutInfo::AddSoftware)
      .def("AddPlatformInfo", &DutInfo::AddPlatformInfo)
      .def("Registered", &DutInfo::Registered)
      .def("ToProto", &DutInfo::ToProto);

  pybind11::class_<HwRecord>(m, "HwRecord").def("Data", &HwRecord::Data);
  pybind11::class_<SwRecord>(m, "SwRecord").def("Data", &SwRecord::Data);

  // DEPRECATED: Use TestStep::BeginMeasurementSeries().
  m.def(
      "BeginMeasurementSeries",
      [](TestStep* parent, const HwRecord& hwrecord,
         ocpdiag::results_pb::MeasurementInfo info) {
        return ResultApi().BeginMeasurementSeries(parent, hwrecord, info);
      },
      pybind11::arg("parent"), pybind11::arg("hwrecord"),
      pybind11::arg("info"));
  pybind11::class_<MeasurementSeries>(m, "MeasurementSeries")
      .def(pybind11::init<
               const HwRecord&,
               const ocpdiag::results_pb::MeasurementInfo&,
               TestStep&>(),
           pybind11::arg("hwrec"), pybind11::arg("info"),
           pybind11::arg("test_step"))
      .def("AddElement",
           static_cast<void (MeasurementSeries::*)(google::protobuf::Value)>(
               &MeasurementSeries::AddElement),
           "Adds a element without a limit")
      .def("AddElementWithRange", &MeasurementSeries::AddElementWithRange,
           "Adds a range element")
      .def("AddElementWithValues", &MeasurementSeries::AddElementWithValues,
           "Adds a value element")
      .def("Id", &MeasurementSeries::Id)
      .def("Ended", &MeasurementSeries::Ended)
      .def("End", &MeasurementSeries::End)
      .def("__enter__", [](MeasurementSeries* self) { return self; })
      .def("__exit__",
           [](MeasurementSeries* self, pybind11::args) { self->End(); });
}

}  // namespace ocpdiag::results
