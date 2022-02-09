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

#ifndef OCPDIAG_CORE_TESTING_FAKE_TEST_RUN_H_
#define OCPDIAG_CORE_TESTING_FAKE_TEST_RUN_H_

#include <fcntl.h>

#include <memory>

#include "ocpdiag/core/results/results.h"
#include "ocpdiag/core/results/results.pb.h"

namespace ocpdiag {

// FakeTestRun allows access to the private constructor of TestRun for
// unit testing purposes.
class FakeTestRun : public ocpdiag::results::TestRun {
 public:
  FakeTestRun(std::string name, results::internal::ArtifactWriter&& aw) :
    ocpdiag::results::TestRun(name, std::move(aw)) {}
  ~FakeTestRun() override {}
};

// TestRunResultHandler is a class that will handle the creation and destruction
// of the variables needed to properly use a TestRun object.
class TestRunResultHandler {
 public:
  TestRunResultHandler(std::string name) {
    FILE* tmpfile_ptr = std::tmpfile();
    test_run_ = std::make_unique<FakeTestRun>(
        name, results::internal::ArtifactWriter{dup(fileno(tmpfile_ptr)),
                                                &json_out_});
    std::fclose(tmpfile_ptr);
  }
  ~TestRunResultHandler() {
    test_run_.reset();
  }

  FakeTestRun* GetFakeTestRun() {
    return test_run_.get();
  }
  std::string GetJsonOut() {
    return json_out_.str();
  }

 private:
  std::unique_ptr<FakeTestRun> test_run_;
  std::stringstream json_out_;
};

}  // namespace ocpdiag

#endif  // OCPDIAG_CORE_TESTING_FAKE_TEST_RUN_H_
