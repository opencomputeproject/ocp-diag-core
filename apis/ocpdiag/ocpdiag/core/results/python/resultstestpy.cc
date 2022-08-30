// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include <pybind11/pybind11.h>

#include "ocpdiag/core/results/python/results_testutils.h"
#include "pybind11_protobuf/wrapped_proto_caster.h"

namespace ocpdiag::results::internal {

PYBIND11_MODULE(resultstestpy, m) {
  pybind11_protobuf::ImportWrappedProtoCasters();

  m.doc() = "testing utilities for the results python codegen";
  pybind11::class_<ResultsTest>(m, "ResultsTest")
      .def(pybind11::init())
      .def("InitializeTestRun", &ResultsTest::InitializeTestRun)
      .def("CreateTestStep", &ResultsTest::CreateTestStep,
           pybind11::arg("runner"), pybind11::arg("name"),
           pybind11::arg("customId") = ResultsTest::kStepIdDefault)
      .def("CreateMeasurementSeries",
           pybind11_protobuf::WithWrappedProtos(
               &ResultsTest::CreateMeasurementSeries))
      .def("RegisterHwId", &ResultsTest::RegisterHwId)
      .def("RegisterSwId", &ResultsTest::RegisterSwId)
      .def("GetJsonReadableOutput", &ResultsTest::GetJsonReadableOutput);
}

}  // namespace ocpdiag::results::internal
