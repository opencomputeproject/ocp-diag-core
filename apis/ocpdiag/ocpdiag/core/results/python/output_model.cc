// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/output_model.h"

#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include "pybind11_abseil/absl_casters.h"
#include "pybind11_abseil/status_casters.h"
#include "pybind11_protobuf/native_proto_caster.h"

// These types needs to be opaque to avoid unnecessary copies.
PYBIND11_MAKE_OPAQUE(ocpdiag::results::TestRunOutput::TestStepMap);
PYBIND11_MAKE_OPAQUE(ocpdiag::results::TestRunOutput::LogsBySeverity);
PYBIND11_MAKE_OPAQUE(ocpdiag::results::TestStepOutput::MeasurementSeriesMap);

namespace ocpdiag::results {

PYBIND11_MODULE(_output_model, m) {
  pybind11::google::ImportStatusModule();
  pybind11_protobuf::ImportNativeProtoCasters();

  pybind11::bind_map<TestRunOutput::TestStepMap>(m, "TestStepMap");
  pybind11::bind_map<TestRunOutput::LogsBySeverity>(m, "LogsBySeverity");
  pybind11::bind_map<TestStepOutput::MeasurementSeriesMap>(
      m, "MeasurementSeriesMap");

  m.doc() = "Bindings for the OCPDiag test result output model";
  pybind11::class_<MeasurementSeriesOutput>(m, "MeasurementSeriesOutput")
      .def_property_readonly(
          "start", [](MeasurementSeriesOutput &series) { return series.start; })
      .def_property_readonly(
          "end", [](MeasurementSeriesOutput &series) { return series.end; })
      .def_property_readonly("measurement_elements",
                             [](MeasurementSeriesOutput &series) {
                               return series.measurement_elements;
                             });

  pybind11::class_<TestStepOutput>(m, "TestStepOutput")
      .def_property_readonly("start",
                             [](TestStepOutput &step) { return step.start; })
      .def_property_readonly("end",
                             [](TestStepOutput &step) { return step.end; })
      .def_property_readonly("logs",
                             [](TestStepOutput &step) { return step.logs; })
      .def_property_readonly("errors",
                             [](TestStepOutput &step) { return step.errors; })
      .def_property_readonly("files",
                             [](TestStepOutput &step) { return step.files; })
      .def_property_readonly(
          "artifact_extensions",
          [](TestStepOutput &step) { return step.artifact_extensions; })
      .def_property_readonly(
          "measurements",
          [](TestStepOutput &step) { return step.measurements; })
      .def_property_readonly(
          "measurement_series",
          [](TestStepOutput &step) { return step.measurement_series; })
      .def_property_readonly(
          "diagnoses", [](TestStepOutput &step) { return step.diagnoses; });

  pybind11::class_<TestRunOutput>(m, "TestRunOutput")
      .def(pybind11::init())
      .def("AddOutputArtifact", &TestRunOutput::AddOutputArtifact)
      .def_property_readonly("start",
                             [](TestRunOutput &run) { return run.start; })
      .def_property_readonly("end", [](TestRunOutput &run) { return run.end; })
      .def_property_readonly("logs",
                             [](TestRunOutput &run) { return run.logs; })
      .def_property_readonly("tags",
                             [](TestRunOutput &run) { return run.tags; })
      .def_property_readonly("steps",
                             [](TestRunOutput &run) { return run.steps; })
      .def_property_readonly("errors",
                             [](TestRunOutput &run) { return run.errors; });
}

}  // namespace ocpdiag::results
