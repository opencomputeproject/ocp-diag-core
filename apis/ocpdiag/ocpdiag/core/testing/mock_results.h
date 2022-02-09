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

#ifndef OCPDIAG_CORE_TESTING_MOCK_RESULTS_H_
#define OCPDIAG_CORE_TESTING_MOCK_RESULTS_H_

#include <fcntl.h>

#include <memory>

#include "google/protobuf/struct.pb.h"
#include "gmock/gmock.h"
#include "absl/strings/string_view.h"
#include "ocpdiag/core/results/internal/logging.h"
#include "ocpdiag/core/results/results.h"
#include "ocpdiag/core/results/results.pb.h"

namespace ocpdiag {
namespace results {
namespace testonly {

using ::testing::_;

static constexpr char kFakeId[] = "fake_id";

// FakeResultApi removes the restriction on number of TestRun objects.
// In production, only one TestRun object is allowed to exist per binary.
class FakeResultApi : public ResultApi {
 public:
  FakeResultApi() = default;

  // Allows unrestricted creation of TestRun objects.
  absl::StatusOr<std::unique_ptr<TestRun>> InitializeTestRun(
      std::string name) override {
    return absl::WrapUnique(
        new TestRun(std::move(name), internal::ArtifactWriter(-1)));
  }
};

// Can be used for interaction testing and injecting failure scenarios.
class MockResultApi : public ResultApi {
 public:
  MockResultApi() = default;

  MOCK_METHOD(absl::StatusOr<std::unique_ptr<TestRun>>, InitializeTestRun,
              (std::string name), (override));
  MOCK_METHOD(absl::StatusOr<std::unique_ptr<TestStep>>, BeginTestStep,
              (TestRun*, std::string), (override));
  MOCK_METHOD(absl::StatusOr<std::unique_ptr<MeasurementSeries>>,
              BeginMeasurementSeries,
              (TestStep*, const HwRecord&,
               ocpdiag::results_pb::MeasurementInfo),
              (override));

  // Tells mock to use the fake implementation by default. These can be
  // changed individually later to stub the action if needed.
  void DelegateToFake() {
    ON_CALL(*this, InitializeTestRun(_)).WillByDefault([&](std::string s) {
      return fake_.InitializeTestRun(s);
    });
    ON_CALL(*this, BeginTestStep(_, _))
        .WillByDefault([&](TestRun* t, std::string s) {
          return fake_.BeginTestStep(t, s);
        });
    ON_CALL(*this, BeginMeasurementSeries(_, _, _))
        .WillByDefault([&](TestStep* t, const HwRecord& h,
                           ocpdiag::results_pb::MeasurementInfo i) {
          return fake_.BeginMeasurementSeries(t, h, i);
        });
  }

 private:
  FakeResultApi fake_;
};

class MockTestRun : public TestRun {
 public:
  // `name`: a descriptive name for your test.
  // `print_to_stdout`: set to true to see human-readable output artifacts.
  MockTestRun(std::string name = "mock_test_run", bool print_to_stdout = false)
      : TestRun(name, internal::ArtifactWriter(-1, print_to_stdout)) {}

  MOCK_METHOD(void, StartAndRegisterInfos,
              (absl::Span<const DutInfo>, const google::protobuf::Message&), (override));

  MOCK_METHOD(ocpdiag::results_pb::TestResult, End, (), (override));

  MOCK_METHOD(ocpdiag::results_pb::TestResult, Skip, (),
              (override));

  MOCK_METHOD(void, AddError, (absl::string_view, absl::string_view),
              (override));

  MOCK_METHOD(void, AddTag, (absl::string_view tag), (override));

  MOCK_METHOD(ocpdiag::results_pb::TestStatus, Status, (),
              (override, const));

  MOCK_METHOD(ocpdiag::results_pb::TestResult, Result, (),
              (override, const));

  MOCK_METHOD(bool, Started, (), (override, const));

  MOCK_METHOD(bool, Ended, (), (override, const));

  MOCK_METHOD(void, LogDebug, (absl::string_view), (override));
  MOCK_METHOD(void, LogInfo, (absl::string_view), (override));
  MOCK_METHOD(void, LogWarn, (absl::string_view), (override));
  MOCK_METHOD(void, LogError, (absl::string_view), (override));
  MOCK_METHOD(void, LogFatal, (absl::string_view), (override));

  // Tells mock to use the real implementation by default. These can be
  // changed individually later to stub the action if needed.
  // NOTE: only methods that may have side effects or non-void return types are
  // stubbed.
  void DelegateToReal() {
    ON_CALL(*this, StartAndRegisterInfos(_, _))
        .WillByDefault([&](absl::Span<const DutInfo> dutinfos,
                           const google::protobuf::Message& params) {
          this->TestRun::StartAndRegisterInfos(dutinfos, params);
        });
    ON_CALL(*this, End()).WillByDefault([&] { return this->TestRun::End(); });
    ON_CALL(*this, Skip()).WillByDefault([&] { return this->TestRun::Skip(); });
    ON_CALL(*this, AddError(_, _))
        .WillByDefault([&](absl::string_view s, absl::string_view msg) {
          return this->TestRun::AddError(s, msg);
        });
    ON_CALL(*this, Status()).WillByDefault([&] {
      return this->TestRun::Status();
    });
    ON_CALL(*this, Result()).WillByDefault([&] {
      return this->TestRun::Result();
    });
    ON_CALL(*this, Started()).WillByDefault([&] {
      return this->TestRun::Started();
    });
    ON_CALL(*this, Ended()).WillByDefault([&] {
      return this->TestRun::Started();
    });
  }
};

// Standalone TestStep that does not require some of the setup required in
// production code (i.e. TestRun parent)
class MockTestStep : public TestStep {
 public:
  // `name`: a descriptive name for your step.
  // `print_to_stdout`: set to true to see human-readable output artifacts.
  MockTestStep(std::string name = "mock_test_step",
               bool print_to_stdout = false)
      : TestStep(nullptr, kFakeId, name,
                 internal::ArtifactWriter(-1, print_to_stdout)) {}

  MOCK_METHOD(void, AddDiagnosis,
              (ocpdiag::results_pb::Diagnosis::Type, std::string,
               std::string, absl::Span<const HwRecord>),
              (override));

  MOCK_METHOD(void, AddError,
              (absl::string_view, absl::string_view,
               absl::Span<const SwRecord>),
              (override));

  MOCK_METHOD(void, AddMeasurement,
              (ocpdiag::results_pb::MeasurementInfo,
               ocpdiag::results_pb::MeasurementElement,
               const HwRecord*),
              (override));

  MOCK_METHOD(void, AddFile, (ocpdiag::results_pb::File),
              (override));

  MOCK_METHOD(void, AddArtifactExtension, (std::string, const google::protobuf::Message&),
              (override));

  MOCK_METHOD(void, End, (), (override));

  MOCK_METHOD(void, Skip, (), (override));

  MOCK_METHOD(void, LogDebug, (absl::string_view), (override));
  MOCK_METHOD(void, LogInfo, (absl::string_view), (override));
  MOCK_METHOD(void, LogWarn, (absl::string_view), (override));
  MOCK_METHOD(void, LogError, (absl::string_view), (override));
  MOCK_METHOD(void, LogFatal, (absl::string_view), (override));

  // Tells mock to use the real implementation by default. These can be
  // changed individually later to stub the action if needed.
  // NOTE: only methods that may have side effects or non-void return types are
  // stubbed.
  void DelegateToReal() {
    ON_CALL(*this, AddError(_, _, _))
        .WillByDefault([&](absl::string_view s, absl::string_view msg,
                           absl::Span<const SwRecord> rs) {
          this->TestStep::AddError(s, msg, rs);
        });
    ON_CALL(*this, AddMeasurement(_, _, _))
        .WillByDefault(
            [&](ocpdiag::results_pb::MeasurementInfo i,
                ocpdiag::results_pb::MeasurementElement elem,
                const ocpdiag::results::HwRecord* r) {
              this->TestStep::AddMeasurement(i, elem, r);
            });
    ON_CALL(*this, End()).WillByDefault([&] { this->TestStep::End(); });
    ON_CALL(*this, Skip()).WillByDefault([&] { this->TestStep::Skip(); });
  }
};

// Standalone MeasurementSeries that does not require some of the setup required
// in production code (i.e. TestStep parent)
class MockMeasurementSeries : public MeasurementSeries {
 public:
  // `print_to_stdout`: set to true to see human-readable output artifacts.
  // `info`: optional MeasurementInfo used to interpret the series.
  MockMeasurementSeries(TestStep* parent = nullptr,
                        bool print_to_stdout = false,
                        ocpdiag::results_pb::MeasurementInfo info =
                            ocpdiag::results_pb::MeasurementInfo{})
      : MeasurementSeries(parent, kFakeId, kFakeId,
                          internal::ArtifactWriter(-1, print_to_stdout), info) {
  }

  MOCK_METHOD(void, AddElement, (google::protobuf::Value), (override));

  MOCK_METHOD(void, AddElementWithRange,
              (google::protobuf::Value,
               ocpdiag::results_pb::MeasurementElement::Range),
              (override));

  MOCK_METHOD(void, AddElementWithValues,
              (google::protobuf::Value,
               absl::Span<const google::protobuf::Value> valid_values),
              (override));

  MOCK_METHOD(void, End, (), (override));
  MOCK_METHOD(bool, Ended, (), (override, const));

  // Tells mock to use the real implementation by default. These can be
  // changed individually later to stub the action if needed.
  // NOTE: only methods that may have side effects or non-void return types are
  // stubbed.
  void DelegateToReal() {
    ON_CALL(*this, AddElementWithRange(_, _))
        .WillByDefault(
            [&](google::protobuf::Value val,
                ocpdiag::results_pb::MeasurementElement::Range r) {
              this->MeasurementSeries::AddElementWithRange(val, r);
            });
    ON_CALL(*this, AddElementWithValues(_, _))
        .WillByDefault([&](google::protobuf::Value val,
                           absl::Span<const google::protobuf::Value> vals) {
          this->MeasurementSeries::AddElementWithValues(val, vals);
        });
    ON_CALL(*this, End()).WillByDefault([&] {
      this->MeasurementSeries::End();
    });
    ON_CALL(*this, Ended()).WillByDefault([&] {
      return this->MeasurementSeries::Ended();
    });
  }
};

}  // namespace testonly
}  // namespace results
}  // namespace ocpdiag

#endif  // OCPDIAG_CORE_TESTING_MOCK_RESULTS_H_
