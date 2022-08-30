// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/results.h"

#include <pybind11/pybind11.h>

#include "google/protobuf/empty.pb.h"
#include "google/protobuf/struct.pb.h"
#include "absl/flags/flag.h"
#include "ocpdiag/core/results/internal/logging.h"
#include "ocpdiag/core/results/results.pb.h"
#include "pybind11_abseil/absl_casters.h"
#include "pybind11_abseil/status_casters.h"
#include "pybind11_protobuf/wrapped_proto_caster.h"

namespace ocpdiag::results {

void SetFlags(const bool ocpdiag_copy_results_to_stdout,
              const std::string ocpdiag_results_filepath,
              const std::string machine_under_test,
              const bool ocpdiag_strict_reporting) {
  absl::SetFlag(&FLAGS_ocpdiag_copy_results_to_stdout,
                ocpdiag_copy_results_to_stdout);
  absl::SetFlag(&FLAGS_ocpdiag_results_filepath, ocpdiag_results_filepath);
  absl::SetFlag(&FLAGS_machine_under_test, machine_under_test);
  absl::SetFlag(&FLAGS_ocpdiag_strict_reporting, ocpdiag_strict_reporting);
}

PYBIND11_MODULE(_results, m) {
  pybind11::google::ImportStatusModule();
  pybind11_protobuf::ImportWrappedProtoCasters();

  using pybind11_protobuf::WithWrappedProtos;

  m.def("SetResultsLibFlags", &SetFlags);

  m.doc() = "Bindings for the OCPDiag results library";
  m.def("InitTestRun", &TestRun::Init);
  pybind11::class_<TestRun>(m, "TestRun")
      .def("StartAndRegisterInfos",
           WithWrappedProtos([](TestRun& a, absl::Span<const DutInfo> dutinfos,
                                const google::protobuf::Message* params) {
             if (params == nullptr) {
               a.StartAndRegisterInfos(dutinfos, google::protobuf::Empty());
             } else {
               a.StartAndRegisterInfos(dutinfos, *params);
             }
           }),
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
      .def("LogFatal", &TestRun::LogFatal);

  m.def("BeginTestStep", &TestStep::Begin);

  // We use a shared_ptr holding type on TestStep because we pass these objects
  // between C++ and Python as function arguments, not simply as property values
  // like the other objects. Property values use a return value policy that
  // allows sharing the object as un-owned references, but function arguments
  // don't. See
  // https://pybind11.readthedocs.io/en/stable/advanced/functions.html#return-value-policies
  // for more information.
  pybind11::class_<TestStep, std::shared_ptr<TestStep>>(m, "TestStep")
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
      .def("AddMeasurement", WithWrappedProtos(&TestStep::AddMeasurement),
           pybind11::arg("info"), pybind11::arg("elem"),
           pybind11::arg("hwRecord").none(true) = pybind11::none())
      .def("AddFile", WithWrappedProtos(&TestStep::AddFile))
      .def("AddArtifactExtension",
           WithWrappedProtos(&TestStep::AddArtifactExtension))
      .def("LogDebug", &TestStep::LogDebug)
      .def("LogInfo", &TestStep::LogInfo)
      .def("LogWarn", &TestStep::LogWarn)
      .def("LogError", &TestStep::LogError)
      .def("LogFatal", &TestStep::LogFatal)
      .def("End", &TestStep::End)
      .def("Skip", &TestStep::Skip)
      .def("Ended", &TestStep::Ended)
      .def("Status", [](TestStep& a) { return static_cast<int>(a.Status()); })
      .def("Id", &TestStep::Id);

  pybind11::class_<DutInfo>(m, "DutInfo")
      .def(pybind11::init<std::string>())
      .def("AddHardware", WithWrappedProtos(&DutInfo::AddHardware))
      .def("AddSoftware", WithWrappedProtos(&DutInfo::AddSoftware))
      .def("AddPlatformInfo", &DutInfo::AddPlatformInfo)
      .def("Registered", &DutInfo::Registered)
      .def("ToProto", WithWrappedProtos(&DutInfo::ToProto));

  pybind11::class_<HwRecord>(m, "HwRecord")
      .def("Data", WithWrappedProtos(&HwRecord::Data));
  pybind11::class_<SwRecord>(m, "SwRecord")
      .def("Data", WithWrappedProtos(&SwRecord::Data));

  m.def("BeginMeasurementSeries", WithWrappedProtos(&MeasurementSeries::Begin));
  pybind11::class_<MeasurementSeries>(m, "MeasurementSeries")
      .def(
          "AddElement",
          WithWrappedProtos(
              static_cast<void (MeasurementSeries::*)(google::protobuf::Value)>(
                  &MeasurementSeries::AddElement)),
          "Adds a element without a limit")
      .def("AddElementWithRange",
           WithWrappedProtos(
               static_cast<void (MeasurementSeries::*)(
                   google::protobuf::Value,
                   ocpdiag::results_pb::MeasurementElement::Range)>(
                   &MeasurementSeries::AddElementWithRange)),
           "Adds a range element")
      // Wrapped protos don't play well with Spans, so the (horrible) workaround
      // is to take in a vector rather than span, convert the wrapped vector
      // to a native vector, then pass that through.
      .def(
          "AddElementWithValues",
          WithWrappedProtos(
              [](MeasurementSeries& a, google::protobuf::Value v,
                 const std::vector<pybind11_protobuf::WrappedProto<
                     google::protobuf::Value, pybind11_protobuf::kValue>>& vs) {
                std::vector<google::protobuf::Value> new_values;
                new_values.reserve(vs.size());
                for (const auto& val : vs) {
                  new_values.push_back(val.proto);
                }
                a.AddElementWithValues(v, new_values);
              }),
          "Adds a value element")
      .def("Id", &MeasurementSeries::Id)
      .def("Ended", &MeasurementSeries::Ended)
      .def("End", &MeasurementSeries::End);
}

}  // namespace ocpdiag::results
