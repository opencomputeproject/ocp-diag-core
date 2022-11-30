// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/output_receiver.h"

#include <pybind11/pybind11.h>

#include "ocpdiag/core/results/recordio_iterator.h"
#include "ocpdiag/core/results/results.pb.h"
#include "pybind11_abseil/absl_casters.h"
#include "pybind11_abseil/status_casters.h"
#include "pybind11_protobuf/native_proto_caster.h"

namespace ocpdiag::results {

using ::ocpdiag::results_pb::OutputArtifact;

PYBIND11_MODULE(_output_receiver, m) {
  pybind11::google::ImportStatusModule();
  pybind11_protobuf::ImportNativeProtoCasters();

  m.doc() = "Bindings for the OCPDiag output receiver library";

  pybind11::class_<RecordIoIterator<OutputArtifact>>(m,
                                                     "OutputArtifactIterator")
      .def("__next__", [](RecordIoIterator<OutputArtifact> &self) {
        // Note that Python calls __next__() on the iterator before using it the
        // first time, which is different than C++ where begin() returns an
        // already-valid iterator. This simply means that we have to
        // post-increment the iterator when wrapping into Python.
        if (!self) throw pybind11::stop_iteration();
        auto ret = std::move(*self);
        ++self;
        return ret;
      });
  pybind11::class_<RecordIoContainer<OutputArtifact>>(m,
                                                      "OutputArtifactContainer")
      .def_property_readonly("file_path",
                             &RecordIoContainer<OutputArtifact>::file_path)
      .def("__iter__", [](RecordIoContainer<OutputArtifact> &self) {
        return self.begin();
      });
  pybind11::class_<OutputReceiver>(m, "OutputReceiver")
      .def(pybind11::init())
      .def_property_readonly("test_run", &OutputReceiver::test_run)
      .def_property_readonly("raw", &OutputReceiver::raw)
      .def_property_readonly("artifact_writer",
                             &OutputReceiver::artifact_writer);
}

}  // namespace ocpdiag::results
