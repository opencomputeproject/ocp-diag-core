// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <pybind11/pybind11.h>

#include "meltan/lib/results/python/results_testutils.h"
#include "pybind11_protobuf/wrapped_proto_caster.h"

namespace meltan::results::internal {

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

}  // namespace meltan::results::internal
