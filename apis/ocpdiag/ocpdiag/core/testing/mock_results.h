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
#include "ocpdiag/core/hwinterface/lib/off_dut_machine_interface/mock_remote.h"
#include "ocpdiag/core/results/internal/logging.h"
#include "ocpdiag/core/results/internal/mock_file_handler.h"
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
class MockResultApi : public FakeResultApi {
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

  // Tells mock to use the fake implementation by default. The above methods can
  // still be stubbed individually for customized behavior. This is useful for
  // injecting behavior in one spot, when otherwise you want it to behave as
  // normal.
  // WARNING: this may override any preexisting expectations or stubs on this
  // mock.
  void DelegateToFake() {
    ON_CALL(*this, InitializeTestRun(_)).WillByDefault([&](std::string s) {
      return this->FakeResultApi::InitializeTestRun(s);
    });
    ON_CALL(*this, BeginTestStep(_, _))
        .WillByDefault([&](TestRun* t, std::string s) {
          return this->FakeResultApi::BeginTestStep(t, s);
        });
    ON_CALL(*this, BeginMeasurementSeries(_, _, _))
        .WillByDefault([&](TestStep* t, const HwRecord& h,
                           ocpdiag::results_pb::MeasurementInfo i) {
          return this->FakeResultApi::BeginMeasurementSeries(t, h, i);
        });
  }
};

// Standalone TestRun that can be created without the use of a ResultApi object.
class FakeTestRun : public TestRun {
 public:
  // `name`: a descriptive name for your test.
  // `json_out`: you may inject an output stream for artifact validation.
  FakeTestRun(std::string name, std::ostream* json_out = nullptr)
      : TestRun(name, internal::ArtifactWriter(-1, json_out)) {}
};

// Standalone TestRun that can be created without the use of a ResultApi object.
// Use this for expectations and stubbing.
class MockTestRun : public FakeTestRun {
 public:
  // `name`: a descriptive name for your test.
  // `json_out`: you may inject an output stream for artifact validation.
  MockTestRun(std::string name = "mock_test_run",
              std::ostream* json_out = nullptr)
      : FakeTestRun(name, json_out) {}

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

  // Tells mock to use the real implementation by default. The above methods can
  // still be stubbed individually for customized behavior. This is useful for
  // injecting behavior in one spot, when otherwise you want it to behave as
  // normal.
  // NOTE: only methods that may have side effects or non-void return types are
  // stubbed.
  // WARNING: this may override any preexisting expectations or stubs on this
  // mock.
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
// production code (i.e. TestRun parent). Use in place of a real TestRun in unit
// tests.
class FakeTestStep : public TestStep {
 public:
  // `name`: a descriptive name for your step.
  // `json_out`: you may inject an output stream for artifact validation.
  FakeTestStep(std::string name = "mock_test_step",
               std::ostream* json_out = nullptr)
      : TestStep(nullptr, kFakeId, name, internal::ArtifactWriter(-1, json_out),
                 absl::make_unique<internal::MockFileHandler>()) {}
};

// Standalone TestStep that does not require some of the setup required in
// production code (i.e. TestRun parent).
// Use this for expectations and stubbing.
class MockTestStep : public FakeTestStep {
 public:
  // `name`: a descriptive name for your step.
  // `json_out`: you may inject an output stream for artifact validation.
  MockTestStep(std::string name = "mock_test_step",
               std::ostream* json_out = nullptr)
      : FakeTestStep(name, json_out) {}

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

  // Tells mock to use the real implementation by default. The above methods can
  // still be stubbed individually for customized behavior. This is useful for
  // injecting behavior in one spot, when otherwise you want it to behave as
  // normal.
  // NOTE: only methods that may have side effects or non-void return types are
  // stubbed.
  // WARNING: this may override any preexisting expectations or stubs on this
  // mock.
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
class FakeMeasurementSeries : public MeasurementSeries {
 public:
  // `json_out`: you may inject an output stream for artifact validation.
  // `info`: optional MeasurementInfo used to interpret the series.
  FakeMeasurementSeries(TestStep* parent = nullptr,
                        std::ostream* json_out = nullptr,
                        ocpdiag::results_pb::MeasurementInfo info =
                            ocpdiag::results_pb::MeasurementInfo{})
      : MeasurementSeries(parent, kFakeId, kFakeId,
                          internal::ArtifactWriter(-1, json_out), info) {}
};

// Standalone MeasurementSeries that does not require some of the setup required
// in production code (i.e. TestStep parent).
// Use this for expectations and stubbing.
class MockMeasurementSeries : public FakeMeasurementSeries {
 public:
  // `json_out`: you may inject an output stream for artifact validation.
  // `info`: optional MeasurementInfo used to interpret the series.
  MockMeasurementSeries(TestStep* parent = nullptr,
                        std::ostream* json_out = nullptr,
                        ocpdiag::results_pb::MeasurementInfo info =
                            ocpdiag::results_pb::MeasurementInfo{})
      : FakeMeasurementSeries(parent, json_out) {}

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

  // Tells mock to use the real implementation by default. The above methods can
  // still be stubbed individually for customized behavior. This is useful for
  // injecting behavior in one spot, when otherwise you want it to behave as
  // normal.
  // NOTE: only methods that may have side effects or non-void return types are
  // stubbed.
  // WARNING: this may override any preexisting expectations or stubs on this
  // mock.
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
