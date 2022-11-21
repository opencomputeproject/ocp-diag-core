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
#include "ocpdiag/core/results/calculator.h"
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

constexpr char kSympInternalErr[] = "ocpdiag-internal-error";

namespace {
// Local utils

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
  return std::make_unique<TestStep>(name, *parent);
}

absl::StatusOr<std::unique_ptr<MeasurementSeries>>
ResultApi::BeginMeasurementSeries(
    TestStep* parent, const HwRecord& hw,
    ocpdiag::results_pb::MeasurementInfo info) {
  if (parent == nullptr)
    return absl::InvalidArgumentError("Parent cannot be null");
  // Note that there is no way to configure the enforcement of measurement
  // constraints when using ResultApi. If you need this feature, then stop using
  // the deprecated ResultApi!
  return std::make_unique<MeasurementSeries>(hw, info, *parent, false);
}

void TestRun::SetEnforceSingleton(bool enforce) {
  absl::MutexLock lock(&singleton_mutex);
  enforce_singleton = enforce;
}

TestRun::TestRun(absl::string_view name, internal::ArtifactWriter& writer,
                 std::unique_ptr<internal::FileHandler> file_handler)
    : writer_(writer),
      file_handler_(std::move(file_handler)),
      name_(name),
      state_(RunState::kNotStarted) {
  LogSink::RegisterWithAbsl();
  absl::MutexLock lock(&singleton_mutex);
  CHECK(!enforce_singleton || !singleton_initialized)
      << "Only one TestRun object can be active at a time within a program";
  singleton_initialized = true;
}

void TestRun::StartAndRegisterInfos(absl::Span<const DutInfo> infos,
                                    const google::protobuf::Message& params) {
  absl::MutexLock lock(&mutex_);
  CHECK_EQ(state_, RunState::kNotStarted)
      << "TestRun already started, or has already finished";
  state_ = RunState::kInProgress;
  EmitStartUnlocked(infos, params);
  result_calculator_.NotifyStartRun();
}

void TestRun::EmitStartUnlocked(absl::Span<const DutInfo> dutinfos,
                                const google::protobuf::Message& params) {
  rpb::TestRunStart start_pb;
  start_pb.set_name(name_);
  start_pb.set_version(ocpdiag::params::GetVersion().data());
  CHECK(start_pb.mutable_parameters()->PackFrom(params))
      << "Error processing params proto";

  // Register IDs with the Artifact Writer and merge DutInfo proto with
  // artifact
  for (const DutInfo& info : dutinfos) {
    const ocpdiag::results_pb::DutInfo& info_pb = info.ToProto();
    for (const auto& hw : info_pb.hardware_components()) {
      writer_.RegisterHwId(hw.hardware_info_id());
    }
    for (const auto& sw : info_pb.software_infos()) {
      writer_.RegisterSwId(sw.software_info_id());
    }
    *start_pb.add_dut_info() = info_pb;
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

rpb::TestResult TestRun::End() {
  absl::MutexLock lock(&mutex_);
  if (state_ == RunState::kEnded) return result_calculator_.result();

  if (state_ == RunState::kNotStarted) {
    // Emit TestRunStart automatically.
    EmitStartUnlocked({}, google::protobuf::Empty());
  }
  state_ = RunState::kEnded;
  result_calculator_.Finalize();

  // Emit TestRunEnd artifact
  rpb::TestRunEnd end_pb;
  end_pb.set_name(name_);
  end_pb.set_status(result_calculator_.status());
  end_pb.set_result(result_calculator_.result());
  rpb::TestRunArtifact run_pb;
  *run_pb.mutable_test_run_end() = std::move(end_pb);
  rpb::OutputArtifact out_pb;
  *out_pb.mutable_test_run_artifact() = std::move(run_pb);
  writer_.Write(out_pb);
  writer_.Flush();

  // Clear the singleton guard now that the test run is over.
  absl::MutexLock singleton_lock(&singleton_mutex);
  singleton_initialized = false;
  return result_calculator_.result();
}

rpb::TestResult TestRun::Skip() {
  result_calculator_.NotifySkip();
  return End();
}

//
// since registration may not have happened. Should TestRunStart/End be
// emitted too?
void TestRun::AddError(absl::string_view symptom, absl::string_view message) {
  rpb::Error err_pb;
  err_pb.set_symptom(std::string(symptom));
  err_pb.set_msg(std::string(message));
  rpb::TestRunArtifact run_pb;
  *run_pb.mutable_error() = std::move(err_pb);
  rpb::OutputArtifact out_pb;
  *out_pb.mutable_test_run_artifact() = std::move(run_pb);
  writer_.Write(out_pb);
  result_calculator_.NotifyError();
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

rpb::TestStatus TestRun::Status() const { return result_calculator_.status(); }

rpb::TestResult TestRun::Result() const { return result_calculator_.result(); }

bool TestRun::Started() const {
  absl::MutexLock lock(&mutex_);
  return state_ == RunState::kInProgress;
}

bool TestRun::Ended() const {
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

TestStep::TestStep(absl::string_view name, TestRun& test_run)
    : result_calculator_(&test_run.result_calculator_),
      writer_(&test_run.writer_),
      file_handler_(test_run.file_handler_.get()),
      name_(name),
      id_(absl::StrCat(test_run.step_id_.Next())),
      status_(rpb::TestStatus::UNKNOWN),
      ended_(false) {
  CHECK(test_run.Started())
      << "TestSteps must be created while the run is active";

  // Emit TestStepStart artifact
  rpb::TestStepArtifact step_pb;
  step_pb.mutable_test_step_start()->set_name(std::string(name));
  step_pb.set_test_step_id(Id());
  rpb::OutputArtifact out;
  *out.mutable_test_step_artifact() = std::move(step_pb);
  writer_->Write(out);
  writer_->Flush();
}

void TestStep::AddDiagnosis(rpb::Diagnosis::Type type,
                            absl::string_view symptom,
                            absl::string_view message,
                            absl::Span<const HwRecord> records) {
  if (type == rpb::Diagnosis::FAIL && result_calculator_ != nullptr) {
    result_calculator_->NotifyFailureDiagnosis();
  }
  rpb::Diagnosis diag_pb;
  diag_pb.set_symptom(std::string(symptom));
  diag_pb.set_type(type);
  diag_pb.set_msg(std::string(message));
  // Check if HW info IDs are registered.
  for (const auto& rec : records) {
    const ocpdiag::results_pb::HardwareInfo& info = rec.Data();
    CHECK(writer_->IsHwRegistered(info.hardware_info_id()))
        << "Unregistered hardware info: " << info.DebugString();
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
                        absl::Span<const SwRecord> records) {
  mutex_.Lock();
  status_ = rpb::TestStatus::ERROR;
  mutex_.Unlock();
  if (result_calculator_ != nullptr) result_calculator_->NotifyError();
  rpb::Error error_pb;
  error_pb.set_symptom(std::string(symptom));
  error_pb.set_msg(std::string(message));
  // Check if SW info IDs are registered.
  for (const auto& rec : records) {
    auto& info = rec.Data();
    CHECK(writer_->IsSwRegistered(info.software_info_id()))
        << "Unregistered software info: " << info.DebugString();
    *error_pb.add_software_info_id() = std::move(info.software_info_id());
  }
  rpb::TestStepArtifact step_pb;
  step_pb.set_test_step_id(absl::StrCat(id_));
  *step_pb.mutable_error() = std::move(error_pb);
  rpb::OutputArtifact out_pb;
  *out_pb.mutable_test_step_artifact() = std::move(step_pb);
  writer_->Write(out_pb);
}

int TestStep::Compare(const google::protobuf::Value& a,
                      const google::protobuf::Value& b) {
  CHECK_EQ(a.kind_case(), b.kind_case()) << "Values must have equal kinds";
  switch (a.kind_case()) {
    case KindCase::kNumberValue:
      if (a.number_value() > b.number_value()) return 1;
      if (a.number_value() < b.number_value()) return -1;
      return 0;
    case KindCase::kStringValue:
      return a.string_value().compare(b.string_value());
    case KindCase::kBoolValue:
      if (a.bool_value() == b.bool_value()) return 0;
      return a.bool_value() ? 1 : -1;
    default:
      break;
  }
  CHECK(false) << "Unsupported value kind: " << a.kind_case();
}

absl::Status TestStep::ValidateValueKinds(const rpb::MeasurementElement& elem) {
  constexpr absl::string_view same_kind_msg =
      "Every google.protobuf.Value proto in a MeasurementElement must "
      "be of the same kind. '%s' does not equal '%s'";
  const KindCase& val_kind = elem.value().kind_case();
  static const auto& valid_value_kinds = *new absl::flat_hash_set<KindCase>{
      KindCase::kNullValue, KindCase::kNumberValue, KindCase::kStringValue,
      KindCase::kBoolValue, KindCase::kListValue};

  RETURN_IF_ERROR(CheckValueKind(val_kind, valid_value_kinds));
  if (elem.has_valid_values()) {
    const rpb::MeasurementElement::ValidValues& values = elem.valid_values();
    for (const google::protobuf::Value& v : values.values()) {
      RETURN_IF_ERROR(CheckValueKind(v.kind_case(), valid_value_kinds));
      if (v.kind_case() != val_kind) {
        return absl::InvalidArgumentError(
            absl::StrFormat(same_kind_msg, KindToString(v.kind_case()),
                            KindToString(val_kind)));
      }
    }
  } else if (elem.has_range()) {
    static const auto& range_kinds = *new absl::flat_hash_set<KindCase>{
        KindCase::kNumberValue, KindCase::kStringValue};

    RETURN_IF_ERROR(CheckValueKind(val_kind, range_kinds));
    if (elem.range().has_minimum()) {
      const KindCase& min = elem.range().minimum().kind_case();
      RETURN_IF_ERROR(CheckValueKind(min, range_kinds));
      if (min != val_kind) {
        return absl::InvalidArgumentError(absl::StrFormat(
            same_kind_msg, KindToString(min), KindToString(val_kind)));
      }
    }
    if (elem.range().has_maximum()) {
      const KindCase& max = elem.range().maximum().kind_case();
      RETURN_IF_ERROR(CheckValueKind(max, range_kinds));
      if (max != val_kind) {
        return absl::InvalidArgumentError(absl::StrFormat(
            same_kind_msg, KindToString(max), KindToString(val_kind)));
      }
    }
  }
  return absl::OkStatus();
}

bool TestStep::ValidateRange(const google::protobuf::Value& value,
                             const rpb::MeasurementElement::Range& range,
                             bool enforce_constraints) {
  if (range.has_minimum() && Compare(value, range.minimum()) < 0) {
    if (enforce_constraints) {
      AddError(
          "error-value-too-small",
          absl::StrFormat("Value '%s' is less than the minimum of '%s'",
                          value.DebugString(), range.minimum().DebugString()),
          {});
    }
    return false;
  }
  if (range.has_maximum() && Compare(value, range.maximum()) > 0) {
    if (enforce_constraints) {
      AddError(
          "error-value-too-large",
          absl::StrFormat("Value '%s' is more than the maximum of '%s'",
                          value.DebugString(), range.maximum().DebugString()),
          {});
    }
    return false;
  }
  return true;
}

bool TestStep::ValidateValue(
    const google::protobuf::Value& value,
    absl::Span<const google::protobuf::Value> valid_values,
    bool enforce_constraints) {
  if (valid_values.empty()) return true;
  bool is_valid = false;
  for (const google::protobuf::Value& valid_value : valid_values) {
    if (Compare(value, valid_value) == 0) {
      is_valid = true;
      break;
    }
  }
  if (!is_valid && enforce_constraints) {
    AddError("error-invalid-value",
             absl::StrFormat("Value '%s' is invalid", value.DebugString()), {});
  }
  return is_valid;
}

bool TestStep::AddMeasurement(rpb::MeasurementInfo info,
                              rpb::MeasurementElement element,
                              const HwRecord* hwrec, bool enforce_constraints) {
  CHECK_OK(ValidateValueKinds(element));
  if (hwrec != nullptr) {
    // Check if HW info ID is registered.
    std::string id = hwrec->Data().hardware_info_id();
    CHECK(writer_->IsHwRegistered(id))
        << "Unregistered hardware info: " << hwrec->Data().DebugString();
    info.set_hardware_info_id(id);
  }

  // Treat out-of-range measurements as errors.
  bool valid = true;
  if (element.has_range() &&
      !ValidateRange(element.value(), element.range(), enforce_constraints)) {
    valid = false;
  }

  // The value must conform to the given valid values.
  if (element.has_valid_values()) {
    std::vector<google::protobuf::Value> valid_values{
        element.valid_values().values().begin(),
        element.valid_values().values().end()};
    if (!ValidateValue(element.value(), valid_values, enforce_constraints)) {
      valid = false;
    }
  }

  rpb::Measurement meas_pb;
  *meas_pb.mutable_info() = std::move(info);
  *meas_pb.mutable_element() = std::move(element);
  rpb::TestStepArtifact step_pb;
  *step_pb.mutable_measurement() = std::move(meas_pb);
  step_pb.set_test_step_id(id_);
  rpb::OutputArtifact out_pb;
  *out_pb.mutable_test_step_artifact() = std::move(step_pb);
  writer_->Write(out_pb);
  return valid;
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

void TestStep::AddArtifactExtension(absl::string_view name,
                                    const google::protobuf::Message& extension) {
  rpb::ArtifactExtension ext_pb;
  if (extension.GetTypeName() == ext_pb.mutable_extension()->GetTypeName()) {
    // The user is adding an 'Any' proto, so copy it.
    ext_pb.mutable_extension()->CopyFrom(extension);
  } else {
    // The user is adding some other type of proto, so put it inside the 'Any'.
    CHECK(ext_pb.mutable_extension()->PackFrom(extension))
        << absl::StrCat("Unable to process artifact extension proto: ",
                        extension.DebugString());
  }
  ext_pb.set_name(std::string(name));
  rpb::TestStepArtifact step_pb;
  *step_pb.mutable_extension() = std::move(ext_pb);
  step_pb.set_test_step_id(id_);
  rpb::OutputArtifact out_pb;
  *out_pb.mutable_test_step_artifact() = std::move(step_pb);
  MergedTypeResolver resolver(extension.GetDescriptor()->file()->pool());
  writer_->Write(out_pb, &resolver);
}

bool TestStep::Ended() const {
  absl::MutexLock lock(&mutex_);
  return ended_;
}

void TestStep::End() {
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

void TestStep::Skip() {
  {
    absl::MutexLock lock(&mutex_);
    if (status_ == rpb::TestStatus::UNKNOWN)
      status_ = ocpdiag::results_pb::SKIPPED;
  }
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

rpb::TestStatus TestStep::Status() const {
  absl::MutexLock lock(&mutex_);
  return status_;
}

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

internal::IntIncrementer& DutInfo::GetHardwareIdSource() {
  static auto& incrementer = *new internal::IntIncrementer();
  return incrementer;
}

internal::IntIncrementer& DutInfo::GetSoftwareIdSource() {
  static auto& incrementer = *new internal::IntIncrementer();
  return incrementer;
}

void DutInfo::AddPlatformInfo(absl::string_view info) {
  *proto_.mutable_platform_info()->add_info() = std::string(info);
}

HwRecord DutInfo::AddHardware(rpb::HardwareInfo info) {
  info.set_hardware_info_id(absl::StrCat(GetHardwareIdSource().Next()));
  HwRecord ret(info);
  *proto_.add_hardware_components() = std::move(info);
  return ret;
}

SwRecord DutInfo::AddSoftware(rpb::SoftwareInfo info) {
  info.set_software_info_id(absl::StrCat(GetSoftwareIdSource().Next()));
  SwRecord ret(info);
  *proto_.add_software_infos() = std::move(info);
  return ret;
}

MeasurementSeries::MeasurementSeries(
    const HwRecord& hw,
    const ocpdiag::results_pb::MeasurementInfo& info,
    TestStep& test_step, bool enforce_constraints)
    : test_step_(&test_step),
      enforce_constraints_(enforce_constraints),
      writer_(test_step.writer_),
      step_id_(test_step.Id()),
      series_id_(absl::StrCat(test_step.series_id_.Next())),
      ended_(false),
      info_(info) {
  CHECK(!test_step.Ended())
      << "MeasurementSeries must be started while the test step is active";
  CHECK(writer_->IsHwRegistered(hw.Data().hardware_info_id()))
      << "Unregistered hardware info";
  info_.set_hardware_info_id(hw.Data().hardware_info_id());

  // Emit MeasurementSeriesBegin artifact
  rpb::MeasurementSeriesStart ms_pb;
  ms_pb.set_measurement_series_id(Id());
  *ms_pb.mutable_info() = info_;
  rpb::TestStepArtifact step_pb;
  *step_pb.mutable_measurement_series_start() = std::move(ms_pb);
  step_pb.set_test_step_id(step_id_);
  rpb::OutputArtifact out_pb;
  *out_pb.mutable_test_step_artifact() = std::move(step_pb);
  writer_->Write(out_pb);
  writer_->Flush();
}

bool MeasurementSeries::AddElementWithRange(
    const google::protobuf::Value& value,
    const rpb::MeasurementElement::Range& range) {
  absl::MutexLock lock(&mutex_);
  CHECK(!ended_) << "MeasurementSeries already ended";
  if (value_kind_rule_.kind_case() == google::protobuf::Value::KIND_NOT_SET) {
    static const auto& accepts = *new absl::flat_hash_set<KindCase>{
        KindCase::kNumberValue, KindCase::kStringValue};
    SetValueKind(value, accepts);
  }
  if (range.has_maximum()) CheckValueKind(range.maximum());
  if (range.has_minimum()) CheckValueKind(range.minimum());

  rpb::MeasurementElement elem_pb;
  elem_pb.set_measurement_series_id(absl::StrCat(series_id_));
  elem_pb.set_index(element_count_.Next());
  *elem_pb.mutable_value() = value;
  *elem_pb.mutable_range() = range;
  *elem_pb.mutable_dut_timestamp() = ::ocpdiag::results::internal::Now();
  rpb::TestStepArtifact step_pb;
  *step_pb.mutable_measurement_element() = std::move(elem_pb);
  step_pb.set_test_step_id(step_id_);
  rpb::OutputArtifact out_pb;
  *out_pb.mutable_test_step_artifact() = std::move(step_pb);
  writer_->Write(out_pb);
  return test_step_->ValidateRange(value, range, enforce_constraints_);
}

void MeasurementSeries::AddElement(google::protobuf::Value value) {
  AddElementWithValues(value, {});
}

bool MeasurementSeries::AddElementWithValues(
    const google::protobuf::Value& value,
    absl::Span<const google::protobuf::Value> valid_values) {
  absl::MutexLock lock(&mutex_);
  CHECK(!ended_) << "MeasurementSeries already ended";
  if (value_kind_rule_.kind_case() == google::protobuf::Value::KIND_NOT_SET) {
    static const auto& accepts = *new absl::flat_hash_set<KindCase>{
        KindCase::kNullValue, KindCase::kNumberValue, KindCase::kStringValue,
        KindCase::kBoolValue, KindCase::kListValue};
    SetValueKind(value, accepts);
  }
  CheckValueKind(value);
  for (const google::protobuf::Value& val : valid_values) {
    CheckValueKind(val);
  }
  rpb::MeasurementElement elem_pb;
  elem_pb.set_measurement_series_id(absl::StrCat(series_id_));
  elem_pb.set_index(element_count_.Next());
  *elem_pb.mutable_value() = value;
  for (auto const& val : valid_values) {
    *elem_pb.mutable_valid_values()->add_values() = val;
  }
  *elem_pb.mutable_dut_timestamp() = ::ocpdiag::results::internal::Now();
  rpb::TestStepArtifact step_pb;
  *step_pb.mutable_measurement_element() = std::move(elem_pb);
  step_pb.set_test_step_id(step_id_);
  rpb::OutputArtifact out_pb;
  *out_pb.mutable_test_step_artifact() = std::move(step_pb);
  writer_->Write(out_pb);
  return test_step_->ValidateValue(value, valid_values, enforce_constraints_);
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

void MeasurementSeries::SetValueKind(
    const google::protobuf::Value& val,
    const absl::flat_hash_set<KindCase>& valid_kinds) {
  CHECK_EQ(value_kind_rule_.kind_case(), google::protobuf::Value::KIND_NOT_SET)
      << "trying to set MeasurementSeries Value kind when it is already set.";
  CHECK(valid_kinds.contains(val.kind_case())) << absl::StrFormat(
      "Adding value of kind '%s' to this MeasurementSeries is "
      "invalid. Expected one of: %s. Note: if you'd like to use a Struct "
      "Value type, flatten it into discrete measurement elements "
      "instead.",
      KindToString(val.kind_case()),
      absl::StrJoin(valid_kinds, ", ", value_kind_formatter()));
  value_kind_rule_ = val;
}

void MeasurementSeries::CheckValueKind(const google::protobuf::Value& val) {
  // Every protobuf value must be the same type.
  CHECK_EQ(val.kind_case(), value_kind_rule_.kind_case()) << absl::StrFormat(
      "Unexpected value type for MeasurementSeries '%s'. Got '%s', want '%s'",
      info_.name(), KindToString(val.kind_case()),
      KindToString(value_kind_rule_.kind_case()));
}

}  // namespace results
}  // namespace ocpdiag
