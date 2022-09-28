// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/params/fake_params.h"

#include <pybind11/pybind11.h>

#include "pybind11_abseil/absl_casters.h"
#include "pybind11_abseil/status_casters.h"
#include "pybind11_protobuf/native_proto_caster.h"

namespace ocpdiag::params {

PYBIND11_MODULE(_fake_params, m) {
  pybind11::google::ImportStatusModule();
  pybind11_protobuf::ImportNativeProtoCasters();

  m.doc() = "Bindings for the OCPDiag fake params library";

  pybind11::class_<ParamsCleanup>(m, "ParamsCleanup")
      .def("__enter__", [](ParamsCleanup* self) { return self; })
      .def("__exit__",
           [](ParamsCleanup* self, pybind11::args) { self->Cleanup(); });

  m.def("FakeParams", &FakeParams, pybind11::arg("params"));
}

}  // namespace ocpdiag::params
