// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/results.h"

#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "google/protobuf/any.pb.h"
#include "google/protobuf/empty.pb.h"
#include "google/protobuf/struct.pb.h"
#include "google/protobuf/timestamp.pb.h"
#include "google/protobuf/type.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/util/type_resolver.h"
#include "google/protobuf/util/type_resolver_util.h"
#include "absl/base/attributes.h"
#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_set.h"
#include "absl/flags/flag.h"
#include "absl/log/check.h"
#include "absl/log/log_entry.h"
#include "absl/log/log_sink.h"
#include "absl/log/log_sink_registry.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/synchronization/mutex.h"
#include "absl/types/span.h"
#include "ocpdiag/core/compat/status_macros.h"
#include "ocpdiag/core/params/utils.h"
#include "ocpdiag/core/results/internal/file_handler.h"
#include "ocpdiag/core/results/internal/logging.h"
#include "ocpdiag/core/results/internal/utils.h"
#include "ocpdiag/core/results/results.pb.h"

ABSL_FLAG(bool, ocpdiag_copy_results_to_stdout, true,
          "Prints human-readable result artifacts to stdout in addition to "
          "default output");

ABSL_FLAG(
    std::string, ocpdiag_results_filepath, "",
    "Fully-qualified file path where binary-proto result data gets written.");

ABSL_FLAG(
    std::string, machine_under_test, "local",
    "Machine under test. If the test binary is running on the same machine as "
    "the machine under test, just keep the default \"local\".");

ABSL_FLAG(
    std::string, nodes_under_test, "",
    "Nodes under test. The list of nodes in the target machine to test. The "
    "default is \"\".");

ABSL_FLAG(bool, alsologtoocpdiagresults, false,
          "If set to true, absl and Ecclesia logger will be directed to OCPDiag "
          "results and ABSL default logging destination.");

//
ABSL_FLAG(bool, ocpdiag_strict_reporting, true,
          "Whether to require a global devpath to be reported in"
          "third_party.ocpdiag.results_pb.HardwareInfo");

namespace ocpdiag {
namespace results {

namespace rpb = ::ocpdiag::results_pb;
using KindCase = google::protobuf::Value::KindCase;

constexpr char kSympProceduralErr[] = "ocpdiag-procedural-error";
constexpr char kSympInternalErr[] = "ocpdiag-internal-error";
constexpr char kSympUnregHw[] = "unregistered-hardware-info";
constexpr char kSympUnregSw[] = "unregistered-software-info";

namespace {
// Local utils

static const absl::flat_hash_set<KindCase> ValidValueKinds = {
    KindCase::kNullValue, KindCase::kNumberValue, KindCase::kStringValue,
    KindCase::kBoolValue, KindCase::kListValue};
static const absl::flat_hash_set<KindCase> RangeKinds = {
    KindCase::kNumberValue, KindCase::kStringValue};

std::string KindToString(KindCase k) {
  switch (k) {
    case KindCase::KIND_NOT_SET:
      return "kind not set";
    case KindCase::kNullValue:
      return "NullValue";
    case KindCase::kNumberValue:
      return "double";
    case KindCase::kStringValue:
      return "string";
    case KindCase::kBoolValue:
      return "bool";
    case KindCase::kStructValue:
      return "Struct";
    case KindCase::kListValue:
      return "ListValue";
    default:
      return "unknown kind";
  }
}

// Predicate for absl::StrJoin
struct value_kind_formatter {
  void operator()(std::string* out, KindCase k) {
    out->append(KindToString(k));
  }
};

// Checks Value kind against a list of valid kinds.
absl::Status CheckValueKind(const KindCase& k,
                            const absl::flat_hash_set<KindCase>& valid) {
  constexpr absl::string_view invalid_kind_msg =
      "MeasurementElement value of kind '%s' is invalid in this context. "
      "Expected one of: %s. Note: if you'd like to use a Struct "
      "Value type, flatten it into discrete measurement elements "
      "instead.";
  if (!valid.contains(k)) {
    return absl::InvalidArgumentError(
        absl::StrFormat(invalid_kind_msg, KindToString(k),
                        absl::StrJoin(valid, ", ", value_kind_formatter())));
  }
  return absl::OkStatus();
}

constexpr const char* kTypeUrlPrefix = "type.googleapis.com";

class MergedTypeResolver final : public google::protobuf::util::TypeResolver {
 public:
  using Status = google::protobuf::util::Status;
  MergedTypeResolver(const google::protobuf::DescriptorPool* pool)
      : resolver_(google::protobuf::util::NewTypeResolverForDescriptorPool(kTypeUrlPrefix,
                                                                 pool)) {}
  ~MergedTypeResolver() = default;

  Status ResolveMessageType(const std::string& type_url,
                            google::protobuf::Type* message_type) override {
    if (internal::GeneratedResolver()
            ->ResolveMessageType(type_url, message_type)
            .ok()) {
      return Status();
    }
    return resolver_->ResolveMessageType(type_url, message_type);
  }

  Status ResolveEnumType(const std::string& type_url,
                         google::protobuf::Enum* enum_type) override {
    if (internal::GeneratedResolver()
            ->ResolveEnumType(type_url, enum_type)
            .ok()) {
      return Status();
    }
    return resolver_->ResolveEnumType(type_url, enum_type);
  }

 private:
  std::unique_ptr<google::protobuf::util::TypeResolver> resolver_;
};

// Custom ABSL LogSink that redirect the ABSL log to the global ArtifactWriter.
class LogSink : public absl::LogSink {
 public:
  // Registers the LogSink with ABSL. You only need to call this once.
  static void RegisterWithAbsl() {
    // The LogSink is stored as a global static variable, which is initialized
    // once in a thread-safe manner.
    static LogSink* log_sink = []() {
      auto* ret = new LogSink();
      if (absl::GetFlag(FLAGS_alsologtoocpdiagresults)) {
        absl::AddLogSink(ret);
      }
      return ret;
    }();
    (void)log_sink;  // suppress unused variable warning
  }

  // Logs the given message to the global artifact writer.
  static void LogToArtifactWriter(
      absl::string_view msg,
      ocpdiag::results_pb::Log::Severity severity) {
    rpb::Log log_pb;
    log_pb.set_text(std::string(msg));
    log_pb.set_severity(severity);
    rpb::TestRunArtifact run_pb;
    *run_pb.mutable_log() = std::move(log_pb);
    rpb::OutputArtifact out_pb;
    *out_pb.mutable_test_run_artifact() = std::move(run_pb);

    internal::GetGlobalArtifactWriter().Write(out_pb);
  }

  // Log function that directly logs the message with ArtifactWriter. This will
  // allow logging without TestRun or TestStep.
  void Send(const absl::LogEntry& entry) final {
    absl::string_view text = entry.text_message_with_prefix();
    switch (entry.log_severity()) {
      case absl::LogSeverity::kInfo:
        LogToArtifactWriter(text, rpb::Log::INFO);
        return;
      case absl::LogSeverity::kWarning:
        LogToArtifactWriter(text, rpb::Log::WARNING);
        return;
      case absl::LogSeverity::kError:
        LogToArtifactWriter(text, rpb::Log::ERROR);
        return;
      case absl::LogSeverity::kFatal:
        LogToArtifactWriter(text, rpb::Log::FATAL);
        return;
    }
    std::cerr << "Unknown ABSL log severity: " << entry.log_severity()
              << std::endl;
  }
};

// This ensures we can only have one test run object at a time.
ABSL_CONST_INIT absl::Mutex singleton_mutex(absl::kConstInit);
bool singleton_initialized ABSL_GUARDED_BY(singleton_mutex) = false;
bool enforce_singleton ABSL_GUARDED_BY(singleton_mutex) = true;

}  // namespace

namespace internal {

internal::ArtifactWriter& GetGlobalArtifactWriter() {
  static auto& writer = *[] {
    absl::StatusOr<int> fd = internal::OpenAndGetDescriptor(
        absl::GetFlag(FLAGS_ocpdiag_results_filepath).c_str());
    CHECK_OK(fd.status());
    std::ostream* stream = absl::GetFlag(FLAGS_ocpdiag_copy_results_to_stdout)
                               ? &std::cout
                               : nullptr;
    return new internal::ArtifactWriter(*fd, stream);
  }();
  return writer;
}

}  // namespace internal

absl::StatusOr<std::unique_ptr<TestRun>> ResultApi::InitializeTestRun(
    std::string name) {
  return std::make_unique<TestRun>(name);
}

absl::StatusOr<std::unique_ptr<TestStep>> ResultApi::BeginTestStep(
    TestRun* parent, std::string name) {
  if (parent == nullptr)
    return absl::InvalidArgumentError("TestRun cannot be nullptr");
  return parent->BeginTestStep(name);
}

absl::StatusOr<std::unique_ptr<MeasurementSeries>>
ResultApi::BeginMeasurementSeries(
    TestStep* parent, const HwRecord& hw,
    ocpdiag::results_pb::MeasurementInfo info) {
  if (parent == nullptr)
    return absl::InvalidArgumentError("Parent cannot be null");
  return parent->BeginMeasurementSeries(hw, info);
}

void TestRun::SetEnforceSingleton(bool enforce) {
  absl::MutexLock lock(&singleton_mutex);
  enforce_singleton = enforce;
}

TestRun::TestRun(absl::string_view name,
                 internal::ArtifactWriter &writer,
                 std::unique_ptr<internal::FileHandler> file_handler)
    : writer_(writer),
      file_handler_(std::move(file_handler)),
      name_(name),
      state_(RunState::kNotStarted),
      result_(ocpdiag::results_pb::TestResult::NOT_APPLICABLE),
      status_(ocpdiag::results_pb::TestStatus::UNKNOWN) {
  LogSink::RegisterWithAbsl();
  absl::MutexLock lock(&singleton_mutex);
  CHECK(!enforce_singleton || !singleton_initialized)
      << "Only one TestRun object can be active at a time within a program";
  singleton_initialized = true;
}

void TestRun::StartAndRegisterInfos(absl::Span<const DutInfo> infos,
                                    const google::protobuf::Message& params)
    ABSL_LOCKS_EXCLUDED(mutex_) {
  absl::MutexLock lock(&mutex_);
  if (state_ != RunState::kNotStarted) {
    std::cerr << "Ignoring request to start test \"" << name_
              << "\"; it has already started or finished." << std::endl;
    return;
  }
  state_ = RunState::kInProgress;
  EmitStartUnlocked(infos, params);
}

void TestRun::EmitStartUnlocked(absl::Span<const DutInfo> dutinfos,
                                const google::protobuf::Message& params) {
  rpb::TestRunStart start_pb;
  start_pb.set_name(name_);
  start_pb.set_version(std::string(ocpdiag::params::GetVersion()));
  if (params.GetTypeName() != start_pb.mutable_parameters()->GetTypeName()) {
    start_pb.mutable_parameters()->PackFrom(params);
if (false) {
      std::cerr << "Unable to process params proto: " << params.DebugString()
                << std::endl;
    }
  } else {
    start_pb.mutable_parameters()->CopyFrom(params);
  }

  // Register IDs with the Artifact Writer and merge DutInfo proto with
  // artifact
  for (auto& info : dutinfos) {
    auto info_pb = info.ToProto();
    for (const auto& hw : info_pb.hardware_components()) {
      writer_.RegisterHwId(hw.hardware_info_id());
    }
    for (const auto& sw : info_pb.software_infos()) {
      writer_.RegisterSwId(sw.software_info_id());
    }
    start_pb.mutable_dut_info()->Add(std::move(info_pb));
  }
  //
  rpb::TestRunArtifact run_pb;
  *run_pb.mutable_test_run_start() = std::move(start_pb);
  rpb::OutputArtifact out_pb;
  *out_pb.mutable_test_run_artifact() = std::move(run_pb);
  MergedTypeResolver resolver(params.GetDescriptor()->file()->pool());
  writer_.Write(out_pb, &resolver);
  writer_.Flush();
}

rpb::TestResult TestRun::End() ABSL_LOCKS_EXCLUDED(mutex_) {
  absl::MutexLock lock(&mutex_);
  switch (state_) {
    case RunState::kEnded:
      break;
    case RunState::kNotStarted: {
      // Emit TestRunStart and set status to SKIPPED if no previous errors.
      EmitStartUnlocked({}, google::protobuf::Empty());
      if (status_ != rpb::TestStatus::ERROR) {
        // ERROR status takes highest priority, and so should not be
        // overridden.
        status_ = rpb::TestStatus::SKIPPED;
        result_ = rpb::TestResult::NOT_APPLICABLE;
      }
      ABSL_FALLTHROUGH_INTENDED;  // Also emit TestRunEnd
    }
    case RunState::kInProgress: {
      // Calculate final test result and status.
      if (status_ == rpb::TestStatus::UNKNOWN) {
        status_ = rpb::TestStatus::COMPLETE;
        if (result_ == rpb::TestResult::NOT_APPLICABLE) {
          result_ = rpb::TestResult::PASS;
        }
      }
      state_ = RunState::kEnded;

      // Emit TestRunEnd artifact
      rpb::TestRunEnd end_pb;
      end_pb.set_name(name_);
      end_pb.set_status(status_);
      end_pb.set_result(result_);
      rpb::TestRunArtifact run_pb;
      *run_pb.mutable_test_run_end() = std::move(end_pb);
      rpb::OutputArtifact out_pb;
      *out_pb.mutable_test_run_artifact() = std::move(run_pb);
      writer_.Write(out_pb);
      writer_.Flush();
      break;
    }
  }

  // Clear the singleton guard now that the test run is over.
  absl::MutexLock singleton_lock(&singleton_mutex);
  singleton_initialized = false;
  return result_;
}

rpb::TestResult TestRun::Skip() ABSL_LOCKS_EXCLUDED(mutex_) {
  absl::ReleasableMutexLock l(&mutex_);
  if (status_ != rpb::TestStatus::UNKNOWN) {
    return result_;
  }
  status_ = rpb::TestStatus::SKIPPED;
  result_ = rpb::TestResult::NOT_APPLICABLE;
  l.Release();
  return End();
}

//
// since registration may not have happened. Should TestRunStart/End be
// emitted too?
void TestRun::AddError(absl::string_view symptom, absl::string_view message)
    ABSL_LOCKS_EXCLUDED(mutex_) {
  rpb::Error err_pb;
  err_pb.set_symptom(std::string(symptom));
  err_pb.set_msg(std::string(message));
  rpb::TestRunArtifact run_pb;
  *run_pb.mutable_error() = std::move(err_pb);
  rpb::OutputArtifact out_pb;
  *out_pb.mutable_test_run_artifact() = std::move(run_pb);
  writer_.Write(out_pb);
  absl::MutexLock lock(&mutex_);
  status_ = rpb::TestStatus::ERROR;
  result_ = rpb::TestResult::NOT_APPLICABLE;
}

void TestRun::AddTag(absl::string_view tag) {
  rpb::Tag tag_pb;
  tag_pb.set_tag(std::string(tag));
  rpb::TestRunArtifact run_pb;
  *run_pb.mutable_tag() = std::move(tag_pb);
  rpb::OutputArtifact out_pb;
  *out_pb.mutable_test_run_artifact() = std::move(run_pb);
  writer_.Write(out_pb);
}

rpb::TestStatus TestRun::Status() const ABSL_LOCKS_EXCLUDED(mutex_) {
  absl::MutexLock lock(&mutex_);
  return status_;
}

rpb::TestResult TestRun::Result() const ABSL_LOCKS_EXCLUDED(mutex_) {
  absl::MutexLock lock(&mutex_);
  return result_;
}

bool TestRun::Started() const ABSL_LOCKS_EXCLUDED(mutex_) {
  absl::MutexLock lock(&mutex_);
  return state_ == RunState::kInProgress;
}

bool TestRun::Ended() const ABSL_LOCKS_EXCLUDED(mutex_) {
  absl::MutexLock lock(&mutex_);
  return state_ == RunState::kEnded;
}

void TestRun::LogDebug(absl::string_view msg) {
  WriteLog(rpb::Log::DEBUG, msg);
}
void TestRun::LogInfo(absl::string_view msg) { WriteLog(rpb::Log::INFO, msg); }
void TestRun::LogWarn(absl::string_view msg) {
  WriteLog(rpb::Log::WARNING, msg);
}
void TestRun::LogError(absl::string_view msg) {
  WriteLog(rpb::Log::ERROR, msg);
}
void TestRun::LogFatal(absl::string_view msg) {
  WriteLog(rpb::Log::FATAL, msg);
}

void TestRun::ProcessStepError() ABSL_LOCKS_EXCLUDED(mutex_) {
  absl::MutexLock lock(&mutex_);
  if (status_ == rpb::TestStatus::UNKNOWN) {
    status_ = rpb::TestStatus::ERROR;
    result_ = rpb::TestResult::NOT_APPLICABLE;
  }
}

void TestRun::ProcessFailDiag() ABSL_LOCKS_EXCLUDED(mutex_) {
  absl::MutexLock lock(&mutex_);
  if (result_ == rpb::TestResult::NOT_APPLICABLE &&
      status_ == rpb::TestStatus::UNKNOWN) {
    result_ = rpb::TestResult::FAIL;
  }
}

std::string TestRun::GenerateID() { return absl::StrCat(step_id_.Next()); }

void TestRun::WriteLog(ocpdiag::results_pb::Log::Severity severity,
                       absl::string_view msg) {
  rpb::Log log_pb;
  log_pb.set_text(std::string(msg));
  log_pb.set_severity(severity);
  rpb::TestRunArtifact run_pb;
  *run_pb.mutable_log() = std::move(log_pb);
  rpb::OutputArtifact out_pb;
  *out_pb.mutable_test_run_artifact() = std::move(run_pb);
  writer_.Write(out_pb);
}

std::unique_ptr<TestStep> TestRun::BeginTestStep(absl::string_view name) {
  CHECK(Started()) << "TestSteps cannot be created until the TestRun has "
                      "started, nor after TestRun has ended.";

  // Emit TestStepStart artifact
  std::string id = GenerateID();
  rpb::TestStepStart start_pb;
  start_pb.set_name(std::string(name));
  rpb::TestStepArtifact step_pb;
  *step_pb.mutable_test_step_start() = std::move(start_pb);
  step_pb.set_test_step_id(id);
  rpb::OutputArtifact out;
  *out.mutable_test_step_artifact() = std::move(step_pb);
  writer_.Write(out);
  writer_.Flush();

  return absl::WrapUnique(
      new TestStep(*this, id, name, writer_, *file_handler_));
}

TestStep::TestStep(TestRun& parent, absl::string_view id,
                   absl::string_view name, internal::ArtifactWriter& writer,
                   internal::FileHandler& file_handler)
    : parent_(&parent),
      writer_(&writer),
      file_handler_(&file_handler),
      name_(std::move(name)),
      id_(std::move(id)),
      status_(rpb::TestStatus::UNKNOWN),
      ended_(false) {}

void TestStep::AddDiagnosis(rpb::Diagnosis::Type type, std::string symptom,
                            std::string message,
                            absl::Span<const HwRecord> records) {
  if (type == rpb::Diagnosis::FAIL && parent_ != nullptr) {
    parent_->ProcessFailDiag();
  }
  rpb::Diagnosis diag_pb;
  diag_pb.set_symptom(symptom);
  diag_pb.set_type(type);
  diag_pb.set_msg(message);
  // Check if HW info IDs are registered.
  for (const auto& rec : records) {
    auto info = rec.Data();
    if (!writer_->IsHwRegistered(info.hardware_info_id())) {
      std::string msg = absl::StrCat(
          "The following hardware will be omitted from the diagnosis, ",
          "as it was not previously registered with the TestRun: ",
          info.DebugString());
      // This is considered a procedural test error.
      AddError(kSympUnregHw, msg, {});
      continue;
    }
    *diag_pb.add_hardware_info_id() = std::move(info.hardware_info_id());
  }
  rpb::TestStepArtifact step_pb;
  step_pb.set_test_step_id(absl::StrCat(id_));
  *step_pb.mutable_diagnosis() = std::move(diag_pb);
  rpb::OutputArtifact out_pb;
  *out_pb.mutable_test_step_artifact() = std::move(step_pb);
  writer_->Write(out_pb);
}

void TestStep::AddError(absl::string_view symptom, absl::string_view message,
                        absl::Span<const SwRecord> records)
    ABSL_LOCKS_EXCLUDED(mutex_) {
  mutex_.Lock();
  status_ = rpb::TestStatus::ERROR;
  mutex_.Unlock();
  if (parent_ != nullptr) {
    parent_->ProcessStepError();
  }
  rpb::Error error_pb;
  error_pb.set_symptom(std::string(symptom));
  error_pb.set_msg(std::string(message));
  // Check if SW info IDs are registered.
  for (const auto& rec : records) {
    auto info = rec.Data();
    if (!writer_->IsSwRegistered(info.software_info_id())) {
      auto msg = absl::StrCat(
          "The following software will be omitted from the diagnosis, ",
          "as it was not previously registered with the TestRun: ",
          info.DebugString());
      // This is considered a procedural test error. So we emit an additional
      // Error artifact.
      AddError(kSympUnregSw, msg, {});
      continue;
    }
    *error_pb.add_software_info_id() = std::move(info.software_info_id());
  }
  rpb::TestStepArtifact step_pb;
  step_pb.set_test_step_id(absl::StrCat(id_));
  *step_pb.mutable_error() = std::move(error_pb);
  rpb::OutputArtifact out_pb;
  *out_pb.mutable_test_step_artifact() = std::move(step_pb);
  writer_->Write(out_pb);
}

absl::Status TestStep::ValidateMeasurementElement(
    const rpb::MeasurementElement& elem) {
  constexpr absl::string_view same_kind_msg =
      "Every google.protobuf.Value proto in a MeasurementElement must "
      "be of the same kind. '%s' does not equal '%s'";
  const KindCase& val_kind = elem.value().kind_case();
  RETURN_IF_ERROR(CheckValueKind(val_kind, ValidValueKinds));
  if (elem.has_valid_values()) {
    const rpb::MeasurementElement::ValidValues& values = elem.valid_values();
    for (const google::protobuf::Value& v : values.values()) {
      RETURN_IF_ERROR(CheckValueKind(v.kind_case(), ValidValueKinds));
      if (v.kind_case() != val_kind) {
        return absl::InvalidArgumentError(
            absl::StrFormat(same_kind_msg, KindToString(v.kind_case()),
                            KindToString(val_kind)));
      }
    }
  } else if (elem.has_range()) {
    RETURN_IF_ERROR(CheckValueKind(val_kind, RangeKinds));
    if (elem.range().has_minimum()) {
      const KindCase& min = elem.range().minimum().kind_case();
      RETURN_IF_ERROR(CheckValueKind(min, RangeKinds));
      if (min != val_kind) {
        return absl::InvalidArgumentError(absl::StrFormat(
            same_kind_msg, KindToString(min), KindToString(val_kind)));
      }
    }
    if (elem.range().has_maximum()) {
      const KindCase& max = elem.range().maximum().kind_case();
      RETURN_IF_ERROR(CheckValueKind(max, RangeKinds));
      if (max != val_kind) {
        return absl::InvalidArgumentError(absl::StrFormat(
            same_kind_msg, KindToString(max), KindToString(val_kind)));
      }
    }
  }
  return absl::OkStatus();
}

void TestStep::AddMeasurement(rpb::MeasurementInfo info,
                              rpb::MeasurementElement elem,
                              const HwRecord* hwrec) {
  if (absl::Status s = ValidateMeasurementElement(elem); !s.ok()) {
    AddError(kSympProceduralErr, s.message(), {});
    return;
  }
  if (hwrec != nullptr) {
    // Check if HW info ID is registered.
    std::string id = hwrec->Data().hardware_info_id();
    if (!writer_->IsHwRegistered(id)) {
      std::string msg = absl::StrCat(
          "The following hardware will be omitted from the Measurement "
          "result, ",
          "as it was not previously registered with the TestRun: ",
          hwrec->Data().DebugString());
      // This is considered a procedural test error.
      AddError(kSympUnregHw, msg, {});
    } else {
      info.set_hardware_info_id(id);
    }
  }

  rpb::Measurement meas_pb;
  *meas_pb.mutable_info() = std::move(info);
  *meas_pb.mutable_element() = std::move(elem);
  rpb::TestStepArtifact step_pb;
  *step_pb.mutable_measurement() = std::move(meas_pb);
  step_pb.set_test_step_id(id_);
  rpb::OutputArtifact out_pb;
  *out_pb.mutable_test_step_artifact() = std::move(step_pb);
  writer_->Write(out_pb);
}

void TestStep::AddFile(rpb::File file) {
  const std::string& node_addr = file.node_address();
  if (!node_addr.empty()) {
    // File is on remote node. Copy to CWD.
    absl::Status status = file_handler_->CopyRemoteFile(file);
    if (!status.ok()) {
      AddError(kSympInternalErr, status.message(), {});
      return;
    }
  } else if (absl::StartsWith(file.output_path(), "../")) {
    // file is relative and unlikely to be in CWD or subdir. Copy to CWD.
    absl::Status status = file_handler_->CopyLocalFile(file);
    if (!status.ok()) {
      AddError(kSympInternalErr, status.message(), {});
      return;
    }
  } else if (absl::StartsWith(file.output_path(), "/")) {
    // file is absolute local and might be in CWD. Compare.
    std::error_code err;
    std::string cwd = std::filesystem::current_path(err);
    if (err) {
      AddError(kSympInternalErr,
               absl::StrCat("Failed to get working directory: ", err.message()),
               {});
      return;
    }
    if (!absl::StartsWith(file.output_path(), cwd)) {
      // file is local and not in CWD or subdirectory. Copy to CWD.
      absl::Status status = file_handler_->CopyLocalFile(file);
      if (!status.ok()) {
        AddError(kSympInternalErr, status.message(), {});
        return;
      }
    }
  }
  rpb::OutputArtifact out_pb;
  rpb::TestStepArtifact* step_pb = out_pb.mutable_test_step_artifact();
  *step_pb->mutable_file() = std::move(file);
  step_pb->set_test_step_id(id_);
  writer_->Write(out_pb);
}

void TestStep::AddArtifactExtension(std::string name,
                                    const google::protobuf::Message& extension) {
  rpb::ArtifactExtension ext_pb;
  if (extension.GetTypeName() != ext_pb.mutable_extension()->GetTypeName()) {
    ext_pb.mutable_extension()->PackFrom(extension);
if (false) {
      std::string msg =
          absl::StrCat("Unable to process artifact extension proto: ",
                       extension.DebugString());
      AddError(kSympUnregSw, msg, {});
    }
  } else {
    ext_pb.mutable_extension()->CopyFrom(extension);
  }
  ext_pb.set_name(std::move(name));
  rpb::TestStepArtifact step_pb;
  *step_pb.mutable_extension() = std::move(ext_pb);
  step_pb.set_test_step_id(id_);
  rpb::OutputArtifact out_pb;
  *out_pb.mutable_test_step_artifact() = std::move(step_pb);
  MergedTypeResolver resolver(extension.GetDescriptor()->file()->pool());
  writer_->Write(out_pb, &resolver);
}

bool TestStep::Ended() const ABSL_LOCKS_EXCLUDED(mutex_) {
  absl::MutexLock lock(&mutex_);
  return ended_;
}

void TestStep::End() ABSL_LOCKS_EXCLUDED(mutex_) {
  absl::MutexLock lock(&mutex_);
  // The writer can be null if we are a mock.
  if (ended_ || !writer_) return;
  ended_ = true;
  if (status_ == rpb::TestStatus::UNKNOWN) {
    status_ = rpb::TestStatus::COMPLETE;
  }

  // Emit TestStepEnd artifact
  rpb::TestStepEnd end_pb;
  end_pb.set_name(name_);
  end_pb.set_status(status_);
  rpb::TestStepArtifact step_pb;
  *step_pb.mutable_test_step_end() = std::move(end_pb);
  step_pb.set_test_step_id(absl::StrCat(id_));
  rpb::OutputArtifact out_pb;
  *out_pb.mutable_test_step_artifact() = std::move(step_pb);
  writer_->Write(out_pb);
  writer_->Flush();
}

void TestStep::Skip() ABSL_LOCKS_EXCLUDED(mutex_) {
  absl::ReleasableMutexLock l(&mutex_);
  if (status_ == rpb::TestStatus::UNKNOWN) {
    status_ = ocpdiag::results_pb::SKIPPED;
  }
  l.Release();
  End();
}

void TestStep::LogDebug(absl::string_view msg) {
  WriteLog(rpb::Log::DEBUG, msg);
}
void TestStep::LogInfo(absl::string_view msg) { WriteLog(rpb::Log::INFO, msg); }
void TestStep::LogWarn(absl::string_view msg) {
  WriteLog(rpb::Log::WARNING, msg);
}
void TestStep::LogError(absl::string_view msg) {
  WriteLog(rpb::Log::ERROR, msg);
}
void TestStep::LogFatal(absl::string_view msg) {
  WriteLog(rpb::Log::FATAL, msg);
  //
}

rpb::TestStatus TestStep::Status() const ABSL_LOCKS_EXCLUDED(mutex_) {
  absl::MutexLock lock(&mutex_);
  return status_;
}

std::string TestStep::GenerateID() { return absl::StrCat(series_id_.Next()); }

void TestStep::WriteLog(ocpdiag::results_pb::Log::Severity severity,
                        absl::string_view msg) {
  rpb::Log log;
  log.set_text(std::string(msg));
  log.set_severity(severity);
  rpb::TestStepArtifact step;
  *step.mutable_log() = std::move(log);
  step.set_test_step_id(id_);
  rpb::OutputArtifact out;
  *out.mutable_test_step_artifact() = std::move(step);
  writer_->Write(out);
}

// Initialize static members
internal::IntIncrementer DutInfo::hw_uuid_;
internal::IntIncrementer DutInfo::sw_uuid_;

void DutInfo::AddPlatformInfo(std::string info) {
  *proto_.mutable_platform_info()->mutable_info()->Add() = std::move(info);
}

HwRecord DutInfo::AddHardware(rpb::HardwareInfo info) {
  info.set_hardware_info_id(absl::StrCat(hw_uuid_.Next()));
  HwRecord ret(info);
  *proto_.add_hardware_components() = std::move(info);
  return ret;
}

SwRecord DutInfo::AddSoftware(rpb::SoftwareInfo info) {
  info.set_software_info_id(absl::StrCat(sw_uuid_.Next()));
  SwRecord ret(info);
  *proto_.add_software_infos() = std::move(info);
  return ret;
}

MeasurementSeries::MeasurementSeries(
    TestStep& parent, absl::string_view step_id, absl::string_view series_id,
    internal::ArtifactWriter& writer,
    ocpdiag::results_pb::MeasurementInfo info)
    : parent_(&parent),
      writer_(&writer),
      step_id_(std::move(step_id)),
      series_id_(std::move(series_id)),
      ended_(false),
      info_(std::move(info)) {}

std::unique_ptr<MeasurementSeries> TestStep::BeginMeasurementSeries(
    const HwRecord& record,
    ocpdiag::results_pb::MeasurementInfo info) {
  CHECK(!Ended()) << "MeasurementSeries cannot be started after the parent "
                     "TestStep has ended";

  if (!writer_->IsHwRegistered(record.Data().hardware_info_id())) {
    std::string msg = absl::StrFormat(
        "The MeasurementSeries (%s) is ill-formed; the associated hardware "
        "info is not registered with the TestRun: %s",
        info.DebugString(), record.Data().DebugString());
    // This is considered a procedural test error.
    AddError(kSympUnregHw, msg, {});
  }
  info.set_hardware_info_id(record.Data().hardware_info_id());

  // Emit MeasurementSeriesBegin artifact
  std::string series_id = GenerateID();
  std::string step_id = Id();
  rpb::MeasurementSeriesStart ms_pb;
  ms_pb.set_measurement_series_id(series_id);
  *ms_pb.mutable_info() = info;
  rpb::TestStepArtifact step_pb;
  *step_pb.mutable_measurement_series_start() = std::move(ms_pb);
  step_pb.set_test_step_id(step_id);
  rpb::OutputArtifact out_pb;
  *out_pb.mutable_test_step_artifact() = std::move(step_pb);
  writer_->Write(out_pb);
  writer_->Flush();

  return absl::WrapUnique(
      new MeasurementSeries(*this, step_id, series_id, *writer_, info));
}

void MeasurementSeries::AddElementWithRange(
    google::protobuf::Value value, rpb::MeasurementElement::Range range) {
  rpb::MeasurementElement elem_pb;
  absl::ReleasableMutexLock rl(&mutex_);
  if (value_kind_rule_.kind_case() == google::protobuf::Value::KIND_NOT_SET) {
    static const absl::flat_hash_set<KindCase> accepts = {
        KindCase::kNumberValue, KindCase::kStringValue};
    if (absl::Status s = SetValueKind(value, accepts); !s.ok()) {
      if (parent_) {
        parent_->AddError(kSympProceduralErr, s.message(), {});
      } else {
        std::cerr << kSympProceduralErr << ": " << s.message() << std::endl;
      }
      return;
    }
  }
  rl.Release();
  if (range.has_maximum()) {
    RETURN_VOID_IF_ERROR(CheckValueKind(range.maximum()));
  }
  if (range.has_minimum()) {
    RETURN_VOID_IF_ERROR(CheckValueKind(range.minimum()));
  }
  elem_pb.set_measurement_series_id(absl::StrCat(series_id_));
  elem_pb.set_index(element_count_.Next());
  *elem_pb.mutable_value() = std::move(value);
  *elem_pb.mutable_range() = std::move(range);
  *elem_pb.mutable_dut_timestamp() = ::ocpdiag::results::internal::Now();
  absl::MutexLock ml(&mutex_);
  if (ended_) {
    std::cerr << "Ignored measurement element \"" << elem_pb.DebugString()
              << "\" because the MeasurementSeries has already ended"
              << std::endl;
    return;
  }
  rpb::TestStepArtifact step_pb;
  *step_pb.mutable_measurement_element() = std::move(elem_pb);
  step_pb.set_test_step_id(step_id_);
  rpb::OutputArtifact out_pb;
  *out_pb.mutable_test_step_artifact() = std::move(step_pb);
  writer_->Write(out_pb);
}

void MeasurementSeries::AddElement(google::protobuf::Value value) {
  AddElementWithValues(value, {});
}

void MeasurementSeries::AddElementWithValues(
    google::protobuf::Value value,
    absl::Span<const google::protobuf::Value> valid_values) {
  absl::ReleasableMutexLock rl(&mutex_);
  if (value_kind_rule_.kind_case() == google::protobuf::Value::KIND_NOT_SET) {
    static const absl::flat_hash_set<KindCase> accepts = {
        KindCase::kNullValue, KindCase::kNumberValue, KindCase::kStringValue,
        KindCase::kBoolValue, KindCase::kListValue};
    if (absl::Status s = SetValueKind(value, accepts); !s.ok()) {
      if (parent_) {
        parent_->AddError(kSympProceduralErr, s.message(), {});
      } else {
        std::cerr << kSympProceduralErr << ": " << s.message() << std::endl;
      }
      return;
    }
  }
  rl.Release();
  RETURN_VOID_IF_ERROR(CheckValueKind(value));
  for (const google::protobuf::Value& val : valid_values) {
    RETURN_VOID_IF_ERROR(CheckValueKind(val));
  }
  rpb::MeasurementElement elem_pb;
  elem_pb.set_measurement_series_id(absl::StrCat(series_id_));
  elem_pb.set_index(element_count_.Next());
  *elem_pb.mutable_value() = std::move(value);
  for (auto const& val : valid_values) {
    *elem_pb.mutable_valid_values()->add_values() = val;
  }
  *elem_pb.mutable_dut_timestamp() = ::ocpdiag::results::internal::Now();
  absl::MutexLock ml(&mutex_);
  if (ended_) {
    std::cerr << "Ignored measurement element \"" << elem_pb.DebugString()
              << "\" because the MeasurementSeries has already ended"
              << std::endl;
    return;
  }
  rpb::TestStepArtifact step_pb;
  *step_pb.mutable_measurement_element() = std::move(elem_pb);
  step_pb.set_test_step_id(step_id_);
  rpb::OutputArtifact out_pb;
  *out_pb.mutable_test_step_artifact() = std::move(step_pb);
  writer_->Write(out_pb);
}

void MeasurementSeries::End() {
  absl::MutexLock lock(&mutex_);
  // The writer can be null if we are a mock.
  if (ended_ || !writer_) return;

  // Emit MeasurementSeriesEnd artifact
  rpb::MeasurementSeriesEnd ms_pb;
  ms_pb.set_measurement_series_id(series_id_);
  ms_pb.set_total_measurement_count(element_count_.Next());
  rpb::TestStepArtifact step_pb;
  *step_pb.mutable_measurement_series_end() = std::move(ms_pb);
  step_pb.set_test_step_id(step_id_);
  rpb::OutputArtifact out_pb;
  *out_pb.mutable_test_step_artifact() = std::move(step_pb);
  writer_->Write(out_pb);
  writer_->Flush();
  ended_ = true;
}

bool MeasurementSeries::Ended() const {
  absl::MutexLock lock(&mutex_);
  return ended_;
}

absl::Status MeasurementSeries::SetValueKind(
    const google::protobuf::Value& val,
    const absl::flat_hash_set<KindCase>& valid_kinds)
    ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_) {
  if (value_kind_rule_.kind_case() != google::protobuf::Value::KIND_NOT_SET) {
    return absl::InternalError(
        "trying to set MeasurementSeries Value kind when it is already set.");
  }
  if (!valid_kinds.contains(val.kind_case())) {
    return absl::InvalidArgumentError(absl::StrFormat(
        "Adding value of kind '%s' to this MeasurementSeries is "
        "invalid. Expected one of: %s. Note: if you'd like to use a Struct "
        "Value type, flatten it into discrete measurement elements "
        "instead.",
        KindToString(val.kind_case()),
        absl::StrJoin(valid_kinds, ", ", value_kind_formatter())));
  }
  value_kind_rule_ = val;
  return absl::OkStatus();
}

absl::Status MeasurementSeries::CheckValueKind(
    const google::protobuf::Value& val) ABSL_LOCKS_EXCLUDED(mutex_) {
  absl::MutexLock lock(&mutex_);
  if (val.kind_case() != value_kind_rule_.kind_case()) {
    const std::string err = absl::StrFormat(
        "MeasurementSeries \"%s\" Value \"%s\": Every google.protobuf.Value "
        "proto added to a MeasurementSeries must be of the same type. Got "
        "'%s', previously got '%s'",
        info_.name(), val.DebugString(), KindToString(val.kind_case()),
        KindToString(value_kind_rule_.kind_case()));
    if (parent_) {
      parent_->AddError(kSympProceduralErr, err, {});
    } else {
      std::cerr << kSympProceduralErr << ": " << err << std::endl;
    }
    return absl::InvalidArgumentError(err);
  }
  return absl::OkStatus();
}

}  // namespace results
}  // namespace ocpdiag
