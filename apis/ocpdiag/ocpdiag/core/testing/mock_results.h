// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef OCPDIAG_CORE_TESTING_MOCK_RESULTS_H_
#define OCPDIAG_CORE_TESTING_MOCK_RESULTS_H_

#include <fcntl.h>

#include <memory>
#include <ostream>
#include <string>

#include "google/protobuf/struct.pb.h"
#include "gmock/gmock.h"
#include "absl/memory/memory.h"
#include "absl/strings/string_view.h"
#include "ocpdiag/core/results/internal/logging.h"
#include "ocpdiag/core/results/results.h"
#include "ocpdiag/core/results/results.pb.h"

namespace ocpdiag {
namespace results {
namespace testonly {

using ::testing::_;

static constexpr char kFakeId[] = "fake_id";

// Can be used for interaction testing and injecting failure scenarios.
//
// DEPRECATED - Use result objects directly now.
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
};

// Standalone TestRun that can be created without the use of a ResultApi object.
// Use this for expectations and stubbing.
//
// DEPRECATED - Use TestRun objects directly, test output assertions using
// OutputReceiver.
class MockTestRun : public TestRun {
 public:
  // `name`: a descriptive name for your test.
  // `json_out`: you may inject an output stream for artifact validation.
  MockTestRun(absl::string_view name = "mock_test_run")
      : TestRun([&] {
          // Disable singleton enforcement when using the mock.
          TestRun::SetEnforceSingleton(false);
          return name;
        }()) {}

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
};

// Standalone TestStep that does not require some of the setup required in
// production code (i.e. TestRun parent).
// Use this for expectations and stubbing.
//
// DEPRECATED - Use TestStep objects directly, test output assertions using
// OutputReceiver.
class MockTestStep : public TestStep {
 public:
  // `name`: a descriptive name for your step.
  // `json_out`: you may inject an output stream for artifact validation.
  MockTestStep(absl::string_view name = "mock_test_step") : TestStep() {}

  MOCK_METHOD(void, AddDiagnosis,
              (ocpdiag::results_pb::Diagnosis::Type,
               absl::string_view, absl::string_view,
               absl::Span<const HwRecord>),
              (override));

  MOCK_METHOD(void, AddError,
              (absl::string_view, absl::string_view,
               absl::Span<const SwRecord>),
              (override));

  MOCK_METHOD(bool, AddMeasurement,
              (ocpdiag::results_pb::MeasurementInfo,
               ocpdiag::results_pb::MeasurementElement,
               const HwRecord*, bool enforce_constraints),
              (override));

  MOCK_METHOD(void, AddFile, (ocpdiag::results_pb::File),
              (override));

  MOCK_METHOD(void, AddArtifactExtension,
              (absl::string_view, const google::protobuf::Message&), (override));

  MOCK_METHOD(void, End, (), (override));

  MOCK_METHOD(void, Skip, (), (override));
  MOCK_METHOD(ocpdiag::results_pb::TestStatus, Status, (),
              (const override));

  MOCK_METHOD(void, LogDebug, (absl::string_view), (override));
  MOCK_METHOD(void, LogInfo, (absl::string_view), (override));
  MOCK_METHOD(void, LogWarn, (absl::string_view), (override));
  MOCK_METHOD(void, LogError, (absl::string_view), (override));
  MOCK_METHOD(void, LogFatal, (absl::string_view), (override));
};

// Standalone MeasurementSeries that does not require some of the setup required
// in production code (i.e. TestStep parent).
// Use this for expectations and stubbing.
//
// DEPRECATED - Use MeasurementSeries objects directly, test output assertions
// using OutputReceiver.
class MockMeasurementSeries : public MeasurementSeries {
 public:
  // `info`: optional MeasurementInfo used to interpret the series.
  MockMeasurementSeries(ocpdiag::results_pb::MeasurementInfo info =
                            ocpdiag::results_pb::MeasurementInfo{})
      : MeasurementSeries() {}

  MOCK_METHOD(void, AddElement, (google::protobuf::Value), (override));

  MOCK_METHOD(
      bool, AddElementWithRange,
      (const google::protobuf::Value&,
       const ocpdiag::results_pb::MeasurementElement::Range&),
      (override));

  MOCK_METHOD(bool, AddElementWithValues,
              (const google::protobuf::Value&,
               absl::Span<const google::protobuf::Value> valid_values),
              (override));

  MOCK_METHOD(void, End, (), (override));
  MOCK_METHOD(bool, Ended, (), (override, const));
};

}  // namespace testonly
}  // namespace results
}  // namespace ocpdiag

#endif  // OCPDIAG_CORE_TESTING_MOCK_RESULTS_H_
