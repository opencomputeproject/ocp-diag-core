// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef OCPDIAG_LIB_RESULTS_RESULTS_H_
#define OCPDIAG_LIB_RESULTS_RESULTS_H_

#include <memory>
#include <string>
#include <utility>

#include "google/protobuf/empty.pb.h"
#include "google/protobuf/struct.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor_database.h"
#include "google/protobuf/message.h"
#include "absl/base/thread_annotations.h"
#include "absl/flags/declare.h"
#include "absl/memory/memory.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/synchronization/mutex.h"
#include "absl/types/span.h"
#include "ocpdiag/core/results/internal/file_handler.h"
#include "ocpdiag/core/results/internal/logging.h"
#include "ocpdiag/core/results/internal/utils.h"
#include "ocpdiag/core/results/results.pb.h"

ABSL_DECLARE_FLAG(bool, ocpdiag_copy_results_to_stdout);
ABSL_DECLARE_FLAG(std::string, ocpdiag_results_filepath);
ABSL_DECLARE_FLAG(std::string, machine_under_test);
ABSL_DECLARE_FLAG(bool, ocpdiag_strict_reporting);

namespace ocpdiag {
namespace results {

class DutInfo;
class HwRecord;
class MeasurementSeries;
class SwRecord;
class TestRun;
class TestStep;

namespace internal {
//
class ResultsTest;
}  // namespace internal

namespace testonly {
class FakeResultApi;
class FakeTestRun;
class FakeTestStep;
class FakeMeasurementSeries;
}  // namespace testonly

extern const char kInvalidRecordId[];
extern const char kSympProceduralErr[];

// Contains factory methods to create main Result API objects.
// Note: for unit tests, use fake or mock provided in
// 'ocpdiag/core/testing/mock_results.h'
class ResultApi {
 public:
  ResultApi() = default;
  ResultApi(const ResultApi& other) = default;
  ResultApi& operator=(const ResultApi& other) = default;
  virtual ~ResultApi() = default;

#ifndef SWIG
  // Factory method to create a TestRun.
  // NOTE: only one TestRun may exist per binary, thus this method will fail if
  // called a second time. `name`: a descriptive name for your test.
  virtual absl::StatusOr<std::unique_ptr<TestRun>> InitializeTestRun(
      std::string name);

  // Factory method to create a TestStep. Emits a TestStepStart artifact if
  // successful.
  // `testrun`: must be a pointer to a valid and started TestRun.
  // `name`: a descriptive name for your step.
  virtual absl::StatusOr<std::unique_ptr<TestStep>> BeginTestStep(
      TestRun* parent, std::string name);

  // Factory method to create a MeasurementSeries. Emits a
  // MeasurementSeriesStart artifact if successful.
  // `teststep`: must be a pointer to a valid and started TestStep.
  // `hw`: should be registered HwRecord.
  // `minfo`: a MeasurementInfo that provides context for the series.
  virtual absl::StatusOr<std::unique_ptr<MeasurementSeries>>
  BeginMeasurementSeries(TestStep* parent, const HwRecord& hw,
                         ocpdiag::results_pb::MeasurementInfo info);
#endif
};

// The class from which all result generation takes root.
// Intended use is to have one TestRun object per OCPDiag Test.
class TestRun : public internal::LoggerInterface {
 public:
#ifndef SWIG
  TestRun() = delete;
  TestRun(TestRun&&);
  TestRun& operator=(TestRun&&);
#endif
  ~TestRun() override { End(); }

  // Returns a TestRun object if successful. This is meant to be called only
  // once per test, and will fail if called a second time. `name`: a descriptive
  // name for your test.
  static absl::StatusOr<TestRun> Init(std::string name);

  // Emits a TestRunStart artifact and registers the DutInfos.
  // No additional DutInfos can be registered after this point.
  virtual void StartAndRegisterInfos(
      absl::Span<const DutInfo> dutinfos,
      const google::protobuf::Message& params = google::protobuf::Empty());

  // Emits a TestRunEnd artifact and returns overall result.
  virtual ocpdiag::results_pb::TestResult End();

  // Skips and ends the Test.
  // Should be part of, or followed by a return statement.
  virtual ocpdiag::results_pb::TestResult Skip();

  // Emits an Error artifact, associated with the TestRun.
  // This is intended for scenarios where a software error occurs
  // before the test officially starts (i.e. the TestRun::StartAndRegisterInfos
  // method has not yet been called. For example, when gathering host
  // information with the hardware interface).
  // Once the test has started, prefer to use TestStep::AddError(...).
  virtual void AddError(absl::string_view symptom, absl::string_view message);

  // Emits a Tag artifact, associated with the TestRun
  virtual void AddTag(absl::string_view tag);

  // Returns the current overall TestRun status
  virtual ocpdiag::results_pb::TestStatus Status() const;

  // Returns the current overall TestRun result
  virtual ocpdiag::results_pb::TestResult Result() const;

  // If true, it is ok to start creating TestSteps.
  virtual bool Started() const;

  // Returns true if the TestRun has ended (i.e. any of End(), Skip(), or
  // fatal error have been called)
  virtual bool Ended() const;

  // Emits a Log artifact of Debug severity, associated with the TestRun.
  void LogDebug(absl::string_view msg) override;
  // Emits a Log artifact of Info severity, associated with the TestRun.
  void LogInfo(absl::string_view msg) override;
  // Emits a Log artifact of Warn severity, associated with the TestRun.
  void LogWarn(absl::string_view msg) override;
  // Emits a Log artifact of Error severity, associated with the TestRun.
  void LogError(absl::string_view msg) override;
  // Emits a Log artifact of Fatal severity, associated with the TestRun.
  // Note: this may have downstream effects, such as terminating the program.
  void LogFatal(absl::string_view msg) override;

  // Internal use only.
  // Sets the overall TestRun Status to ERROR, if not already ended or skipped.
  void ProcessStepError();

  // Internal use only.
  // Signals TestRun to FAIL when it ends, but will be overridden if an Error
  // artifact is emitted.
  void ProcessFailDiag();

  // Internal use only.
  std::string GenerateID();

  // Internal use only.
  internal::ArtifactWriter& GetWriter() { return writer_; }

 private:
  friend testonly::FakeResultApi;
  friend testonly::FakeTestRun;
  //
  friend internal::ResultsTest;
#ifndef SWIG
  TestRun(std::string name, internal::ArtifactWriter writer);
  enum class RunState { kNotStarted, kInProgress, kEnded };
#endif
  // Emits the TestRunStart artifact without holding mutex lock.
  void EmitStartUnlocked(absl::Span<const DutInfo> dutinfos,
                         const google::protobuf::Message& params);
  void WriteLog(ocpdiag::results_pb::Log::Severity,
                absl::string_view msg) override;

  internal::ArtifactWriter writer_;
  std::string name_;
  internal::IntIncrementer step_id_;
  mutable absl::Mutex mutex_;  // mutable allows const methods to acquire mutex.
  RunState state_ ABSL_GUARDED_BY(mutex_);
  ocpdiag::results_pb::TestResult result_ ABSL_GUARDED_BY(mutex_);
  ocpdiag::results_pb::TestStatus status_ ABSL_GUARDED_BY(mutex_);
};

// TestStep is a logical subdivision of a TestRun.
class TestStep : public internal::LoggerInterface {
 public:
#ifndef SWIG
  TestStep() = delete;
  TestStep(TestStep&&);
  TestStep& operator=(TestStep&&);
#endif
  ~TestStep() override { End(); }

  // Factory to create a TestStep. Emits a TestStepStart artifact if successful.
  static absl::StatusOr<TestStep> Begin(TestRun*, std::string name);

  // Emits a Diagnosis artifact. A FAIL type also sets TestRun result to FAIL,
  // unless an Error artifact has been emitted before this.
  virtual void AddDiagnosis(ocpdiag::results_pb::Diagnosis::Type,
                            std::string symptom, std::string message,
                            absl::Span<const HwRecord>);

  // Emits an Error artifact associated with this TestStep.
  // Also Sets TestRun status to ERROR.
  virtual void AddError(absl::string_view symptom, absl::string_view message,
                        absl::Span<const SwRecord>);

  // Emits a standalone Measurement artifact.
  // Acceptable Value kinds if using ValidValues limit: NullValue, number,
  // string, bool, ListValue.
  // Acceptable Value kinds if using Range limit: number, string.
  virtual void AddMeasurement(
      ocpdiag::results_pb::MeasurementInfo,
      ocpdiag::results_pb::MeasurementElement,
      const HwRecord* hwrec);

  // Emits a File artifact.
  // For local files (on runtime-host)
  //   * set `output_path` in the File proto to absolute/relative log path.
  // For remote files (e.g. off-DUT test scenario):
  //   * `output_path` must be the absolute filepath on remote host
  //   *  make sure `node_address` contains a valid address.
  // NOTE: Unless file is in current working directory or subdirectory, it will
  // be copied there, and the File proto `output_path` will be
  // modified to the local copy.
  virtual void AddFile(ocpdiag::results_pb::File);

  // Emits an ArtifactExtension artifact
  virtual void AddArtifactExtension(std::string name,
                                    const google::protobuf::Message& extension);

  // Emits a Log artifact of Debug severity, associated with the TestStep.
  void LogDebug(absl::string_view msg) override;
  // Emits a Log artifact of Info severity, associated with the TestStep.
  void LogInfo(absl::string_view msg) override;
  // Emits a Log artifact of Warn severity, associated with the TestStep.
  void LogWarn(absl::string_view msg) override;
  // Emits a Log artifact of Error severity, associated with the TestStep.
  void LogError(absl::string_view msg) override;
  // Emits a Log artifact of Fatal severity, associated with the TestStep.
  // Note: this may have downstream effects, such as terminating the program.
  void LogFatal(absl::string_view msg) override;

  // Emits a TestStepEnd artifact
  virtual void End();

  // Skips and ends the step.
  virtual void Skip();

  // Returns true if End() or Skip() have been called
  bool Ended() const;

  // Returns current TestStep status
  ocpdiag::results_pb::TestStatus Status() const;

  // Internal use only.
  std::string Id() const { return id_; }

  // Internal use only.
  std::string GenerateID();

  // Internal use only.
  internal::ArtifactWriter& GetWriter() { return writer_; }

  // Internal use only.
  // Ensures that a MeasurementElement added to this Step is well-formed and has
  // valid Value kind.
  absl::Status ValidateMeasElem(
      const ocpdiag::results_pb::MeasurementElement&);

 private:
  friend testonly::FakeTestStep;
  //
  friend internal::ResultsTest;
  TestStep(TestRun* parent, std::string id, std::string name,
           internal::ArtifactWriter,
           std::unique_ptr<internal::FileHandler> file_handler =
               std::make_unique<internal::FileHandler>());
  void WriteLog(ocpdiag::results_pb::Log::Severity,
                absl::string_view msg) override;

  TestRun* parent_;
  internal::ArtifactWriter writer_;
  std::unique_ptr<internal::FileHandler> file_handler_;
  std::string name_;
  std::string id_;
  internal::IntIncrementer series_id_;
  mutable absl::Mutex mutex_;  // mutable allows const methods to acquire mutex.
  ocpdiag::results_pb::TestStatus status_ ABSL_GUARDED_BY(mutex_);
  bool ended_ ABSL_GUARDED_BY(mutex_);
};

// Organizes DUT related info such as hardware, software, and platform
// identifiers
class DutInfo {
 public:
  DutInfo() = default;
  explicit DutInfo(std::string name) : registered_(false) {
    proto_.set_hostname(std::move(name));
  }

  // Adds a HardwareInfo to the DutInfo, returning a record for later use in
  // Diagnosis emissions. *Note*: this does not register the info - this DutInfo
  // must be registered with the TestRun object before the record can be used.
  HwRecord AddHardware(ocpdiag::results_pb::HardwareInfo);

  // Adds a SoftwareInfo to the DutInfo, returning a record for later use in
  // Error emissions. *Note*: this does not register the info - this DutInfo
  // must be registered with the TestRun object before the record can be used.
  SwRecord AddSoftware(ocpdiag::results_pb::SoftwareInfo);

  // Adds a descriptive string about the host to the DutInfo, such as
  // platform-family or plugins.
  void AddPlatformInfo(std::string);

  // Returns true if the DutInfo has been registered with the TestRun object.
  bool Registered() const { return registered_; }

  // Returns the protobuf representation of the DutInfo
  const ocpdiag::results_pb::DutInfo& ToProto() const {
    return proto_;
  }

 private:
  ocpdiag::results_pb::DutInfo proto_;
  bool registered_;
  // Shall be globally unique within the binary, even across TestRun objects.
  static internal::IntIncrementer hw_uuid_;
  static internal::IntIncrementer sw_uuid_;
};

// A handle to a HardwareInfo that has been added to a DutInfo.
class HwRecord {
 public:
#ifndef SWIG
  // Default constructor generates an invalid record, but can be used for tests.
  explicit HwRecord() { data_.set_hardware_info_id(kInvalidRecordId); }
  HwRecord(const HwRecord&) = default;
  HwRecord& operator=(const HwRecord&) = default;
#endif

  // Returns an immutable reference to the underlying HardwareInfo proto.
  const ocpdiag::results_pb::HardwareInfo& Data() const {
    return data_;
  }

 private:
  // Only a DutInfo can create a valid HwRecord
  friend DutInfo;
  explicit HwRecord(ocpdiag::results_pb::HardwareInfo i)
      : data_(std::move(i)) {}
  ocpdiag::results_pb::HardwareInfo data_;
};

// A handle to a SoftwareInfo that has been added to a DutInfo.
class SwRecord {
 public:
#ifndef SWIG
  // Default constructor generates an invalid record, but can be used for tests.
  SwRecord() { data_.set_software_info_id(kInvalidRecordId); }
  SwRecord(const SwRecord&) = default;
  SwRecord& operator=(const SwRecord&) = default;
#endif
  // Returns an immutable reference to the underlying SoftwareInfo proto.
  const ocpdiag::results_pb::SoftwareInfo& Data() const {
    return data_;
  }

 private:
  // Only a DutInfo can create a SwRecord
  friend DutInfo;
  explicit SwRecord(ocpdiag::results_pb::SoftwareInfo i)
      : data_(std::move(i)) {}
  ocpdiag::results_pb::SoftwareInfo data_;
};

// A collection of related measurement elements.
class MeasurementSeries {
 public:
#ifndef SWIG
  MeasurementSeries() = delete;
  MeasurementSeries(MeasurementSeries&&);
  MeasurementSeries& operator=(MeasurementSeries&&);
#endif
  virtual ~MeasurementSeries() { End(); }

  // Factory method to create a MeasurementSeries. Emits a
  // MeasurementSeriesStart artifact if successful.
  static absl::StatusOr<MeasurementSeries> Begin(
      TestStep*, const HwRecord&,
      ocpdiag::results_pb::MeasurementInfo);

  // Emits a MeasurementElement artifact with valid range limit.
  // Acceptable Value kinds: string, number
  virtual void AddElementWithRange(
      google::protobuf::Value,
      ocpdiag::results_pb::MeasurementElement::Range);

  // Emits a MeasurementElement artifact with valid values limit.
  // Acceptable Value kinds: NullValue, number, string, bool, ListValue.
  virtual void AddElementWithValues(
      google::protobuf::Value,
      absl::Span<const google::protobuf::Value> valid_values);

  // Emits a MeasurementElement artifact without a limit.
  // Acceptable Value kinds: NullValue, number, string, bool, ListValue.
  virtual void AddElement(google::protobuf::Value value);

  // Emits a MeasurementSeriesEnd artifact unless already ended.
  virtual void End();

  // Returns true if End() has already been called
  virtual bool Ended() const;

  // Internal use only.
  std::string Id() const { return series_id_; }

 private:
  friend testonly::FakeMeasurementSeries;
  //
  friend internal::ResultsTest;
  MeasurementSeries(TestStep* parent, std::string step_id,
                    std::string series_id, internal::ArtifactWriter,
                    ocpdiag::results_pb::MeasurementInfo);
  // If no values have yet been added to this series, sets the value kind rule.
  // `val`: the value to be added.
  // `valid_kinds`: the kinds that are accepted in caller's context.
  absl::Status SetValueKind(
      const google::protobuf::Value& val,
      const absl::flat_hash_set<google::protobuf::Value::KindCase>&
          valid_kinds);
  // Verifies that a Value added to this series matches the 'kind' of others
  // that have been previously added.
  absl::Status CheckValueKind(const google::protobuf::Value&);

  TestStep* parent_;
  internal::ArtifactWriter writer_;
  std::string step_id_;  // Id of parent Step
  std::string series_id_;
  internal::IntIncrementer element_count_;
  mutable absl::Mutex mutex_;  // mutable allows const methods to acquire mutex.
  bool ended_ ABSL_GUARDED_BY(mutex_);
  // Ensures all elements added to this series are of the same 'kind',
  // since google.protobuf.Value is dynamically typed. This gets set by the
  // first value added to the series.
  google::protobuf::Value value_kind_rule_ ABSL_GUARDED_BY(mutex_);
  ocpdiag::results_pb::MeasurementInfo info_;
};

}  // namespace results
}  // namespace ocpdiag

#endif  // OCPDIAG_LIB_RESULTS_RESULTS_H_
