// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/params/ocpdiag_parameter_parser.h"

#include <fcntl.h>

#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor_database.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/extension_set.h"
#include "google/protobuf/message.h"
#include "google/protobuf/util/json_util.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/cord.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "ocpdiag/core/compat/status_converters.h"
#include "ocpdiag/core/params/ocpdiag_params.pb.h"
#include "ocpdiag/core/params/utils.h"

namespace ocpdiag {
namespace {

constexpr absl::string_view kEnvironmentHelp[] = {
    "OCPDIAG_STDIN ("
    "By default, only read JSON params from redirected stdin in an interactive "
    "terminal environment. When set, always block reading JSON params from "
    "stdin.);",
};

// MaybeArrayAssignment tries to distinguish between a user-provided primitive
// value or a JSON array string.
bool MaybeArrayAssignment(const google::protobuf::FieldDescriptor* field,
                          absl::string_view value) {
  return field->is_repeated() && !value.empty() && value.front() == '[' &&
         value.back() == ']';
}

// MaybeMapAssignment tries to distinguish between a user-provided primitive
// value or a JSON object to be used as a map.
bool MaybeMapAssignment(const google::protobuf::FieldDescriptor* field,
                        absl::string_view value) {
  return field->is_map() && !value.empty() && value.front() == '{' &&
         value.back() == '}';
}

// Retrieves a message field from inside another message.
google::protobuf::Message* GetMessageMember(google::protobuf::Message* message,
                                  const google::protobuf::FieldDescriptor* field,
                                  std::optional<size_t> index) {
  const google::protobuf::Reflection* reflection = message->GetReflection();
  if (field->is_repeated()) {
    if (!index.has_value()) {
      return nullptr;
    }
    return reflection->MutableRepeatedMessage(message, field, *index);
  }
  return reflection->MutableMessage(message, field);
}

// These macros eliminate the boilerplate for the 8 primitive types that
// would otherwise need to be copied in the field assignment logic.
#define SET_PROTO_TYPE(type_name, field, index, value)                 \
  if (field->is_repeated()) {                                          \
    reflection->SetRepeated##type_name(message, field, *index, value); \
  } else {                                                             \
    reflection->Set##type_name(message, field, value);                 \
  }

#define PROTO_NUMERIC_CASE(type, type_name, parse, index)            \
  type value;                                                        \
  if (!parse(text, &value)) {                                        \
    return absl::InvalidArgumentError(                               \
        absl::StrCat("\"", text, "\" cannot parse to ##type_name")); \
  }                                                                  \
  SET_PROTO_TYPE(type_name, field, index, value);

// AssignValue parses the passed text value, and assigns it to the passed field,
// and at the given index if it's repeated.
absl::Status AssignValue(google::protobuf::Message* message,
                         const google::protobuf::FieldDescriptor* field,
                         std::optional<size_t> index, absl::string_view text) {
  const google::protobuf::Reflection* reflection = message->GetReflection();
  // Assigning an empty string clears the entry.
  if (text.empty()) {
    reflection->ClearField(message, field);
    return absl::OkStatus();
  }
  // Assign complete JSON composite types.
  if ((!index.has_value() && MaybeArrayAssignment(field, text)) ||
      MaybeMapAssignment(field, text)) {
    reflection->ClearField(message, field);
    std::string msg_text =
        absl::StrCat("{\"", field->json_name(), "\":", text, "}");
    google::protobuf::io::ArrayInputStream msg_stream(msg_text.data(), msg_text.size());
    return ocpdiag::params::MergeFromJsonStream(&msg_stream, message);
  }
  // Consolidate bounds checking for repeated types.
  if (field->is_repeated() &&
      (!index.has_value() ||
       *index >= static_cast<size_t>(reflection->FieldSize(*message, field)))) {
    return absl::InvalidArgumentError("Field index is out of range.");
  }
  switch (field->cpp_type()) {
    case ::google::protobuf::FieldDescriptor::CPPTYPE_BOOL: {
      PROTO_NUMERIC_CASE(bool, Bool, absl::SimpleAtob, index);
      break;
    }
    case ::google::protobuf::FieldDescriptor::CPPTYPE_INT32: {
      PROTO_NUMERIC_CASE(int32_t, Int32, absl::SimpleAtoi, index);
      break;
    }
    case ::google::protobuf::FieldDescriptor::CPPTYPE_INT64: {
      PROTO_NUMERIC_CASE(int64_t, Int64, absl::SimpleAtoi, index);
      break;
    }
    case ::google::protobuf::FieldDescriptor::CPPTYPE_UINT32: {
      PROTO_NUMERIC_CASE(uint32_t, UInt32, absl::SimpleAtoi, index);
      break;
    }
    case ::google::protobuf::FieldDescriptor::CPPTYPE_UINT64: {
      PROTO_NUMERIC_CASE(uint64_t, UInt64, absl::SimpleAtoi, index);
      break;
    }
    case ::google::protobuf::FieldDescriptor::CPPTYPE_FLOAT: {
      PROTO_NUMERIC_CASE(float, Float, absl::SimpleAtof, index);
      break;
    }
    case ::google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE: {
      PROTO_NUMERIC_CASE(double, Double, absl::SimpleAtod, index);
      break;
    }
    case ::google::protobuf::FieldDescriptor::CPPTYPE_STRING: {
      SET_PROTO_TYPE(String, field, index, std::string{text});
      break;
    }
    case ::google::protobuf::FieldDescriptor::CPPTYPE_ENUM: {
      const google::protobuf::EnumDescriptor* enum_type = field->enum_type();
      const google::protobuf::EnumValueDescriptor* enum_value =
          enum_type->FindValueByName(std::string(text));
      int32_t enum_number;
      if (enum_value == nullptr && absl::SimpleAtoi(text, &enum_number)) {
        enum_value = enum_type->FindValueByNumber(enum_number);
      }
      if (enum_value == nullptr) {
        return absl::InvalidArgumentError(
            absl::StrCat(text, " is not a valid enumeration for type ",
                         enum_type->full_name(), "."));
      }
      SET_PROTO_TYPE(Enum, field, index, enum_value);
      break;
    }
    case ::google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: {
      google::protobuf::Message* value = GetMessageMember(message, field, index);
      value->Clear();
      if (absl::Status parse = AsAbslStatus(
              google::protobuf::util::JsonStringToMessage(std::string(text), value));
          !parse.ok()) {
        return parse;
      }
      break;
    }
  }
  return absl::OkStatus();
}
#undef PROTO_NUMERIC_ASSIGNMENT
#undef SET_PROTO_TYPE

// Parses a numeric index from a single entry in the full field path.
// The passed entry is altered on success to omit the index component.
absl::StatusOr<size_t> ParsePathIndex(absl::string_view& entry,
                                      absl::string_view path) {
  size_t index_start = entry.find_first_of('[');
  if (index_start == absl::string_view::npos) {
    return absl::InvalidArgumentError(absl::StrCat(
        "Mismatched index brackets in \"", entry, "\" from \"", path, "\"."));
  }
  size_t index_size = entry.size() - index_start;
  // Strip brackets from index.
  absl::string_view num = entry.substr(index_start + 1, index_size - 2);
  size_t offset = 0;
  if (!absl::SimpleAtoi(num, &offset)) {
    return absl::InvalidArgumentError(absl::StrCat("Failed to parse \"", num,
                                                   "\" as integer in \"", entry,
                                                   "\" from \"", path, "\"."));
  }
  entry.remove_suffix(index_size);
  return offset;
}

// Retreives a field by name from the given message, and checks that it matches
// the expected cardinality for the assigned value.
absl::StatusOr<const google::protobuf::FieldDescriptor*> GetFieldDescriptor(
    google::protobuf::Message* message, absl::string_view name, absl::string_view path,
    bool repeated, absl::string_view value) {
  const google::protobuf::Descriptor* desc = message->GetDescriptor();
  const google::protobuf::FieldDescriptor* field =
      desc->FindFieldByName(std::string(name));
  if (field == nullptr) {
    return absl::InvalidArgumentError(
        absl::StrCat("Cannot find field named \"", name, "\" in message \"",
                     message->GetTypeName(), "\" from path \"", path, "\""));
  }
  if (repeated == field->is_repeated()) {
    return field;
  }
  if (repeated) {
    return absl::InvalidArgumentError(absl::StrCat(
        "Tried to index the singular field \"", name, "\" in message \"",
        message->GetTypeName(), "\" from path \"", path, "\""));
  }
  if (field->is_map() && MaybeMapAssignment(field, value)) {
    return field;
  }
  if (!MaybeArrayAssignment(field, value)) {
    return absl::InvalidArgumentError(absl::StrCat(
        "Missing index for repeated field \"", name, "\" in message \"",
        message->GetTypeName(), "\" from path \"", path, "\""));
  }
  return field;
}

// AssignPath uses reflection on the passed 'root' message to set the value
// of a field. It accepts dot-delimited paths, including array indices
// (i.e. '[#]'), and can assign primitive values (strings, numbers, bools) as
// well as composite types written in JSON (e.g. arrays '[1, 2, 3]', maps and
// objects '{ "foo":1, "bar":2 }').
absl::Status AssignPath(google::protobuf::Message* root, absl::string_view path,
                        absl::string_view value) {
  google::protobuf::Message* message = nullptr;
  const google::protobuf::FieldDescriptor* field = nullptr;
  std::optional<size_t> index;
  google::protobuf::Message* next = root;
  for (absl::string_view name :
       absl::StrSplit(path, '.', absl::SkipWhitespace())) {
    if (next == nullptr) {
      return absl::InvalidArgumentError(
          absl::StrCat("Non-message internal node \"", field->name(),
                       "\" in path \"", path, "\"."));
    }
    message = next;
    index.reset();
    if (name.back() == ']') {
      absl::StatusOr<size_t> offset = ParsePathIndex(name, path);
      if (!offset.ok()) {
        return offset.status();
      }
      index = *offset;
    }
    absl::StatusOr<const google::protobuf::FieldDescriptor*> lookup =
        GetFieldDescriptor(message, name, path, index.has_value(), value);
    if (!lookup.ok()) {
      return lookup.status();
    }
    field = *lookup;
    next = nullptr;
    if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
      next = GetMessageMember(message, field, index);
    }
  }
  return AssignValue(message, field, index, value);
}

// PopulateTypeDatabase fills the descriptor database with information from the
// passed descriptor set filename. It returns the 'root' file of the set (e.g.
// not imported by any other files in the collection).
absl::StatusOr<absl::string_view> PopulateTypeDatabase(
    const char* filename, google::protobuf::SimpleDescriptorDatabase* database) {
  int desc_fd = open(filename, O_RDONLY);
  if (desc_fd < 0) {
    return absl::UnknownError(
        absl::StrCat("Failed to open descriptor file: ", filename));
  }
  google::protobuf::FileDescriptorSet descriptors;
  google::protobuf::io::FileInputStream desc_stream{desc_fd};
  descriptors.ParseFromZeroCopyStream(&desc_stream);
  desc_stream.Close();

  std::vector<absl::string_view> filenames;
  absl::flat_hash_set<absl::string_view> imported;
  for (auto* files = descriptors.mutable_file(); !files->empty();) {
    google::protobuf::FileDescriptorProto* file = files->ReleaseLast();
    filenames.emplace_back(file->name());
    for (const std::string& import : file->dependency()) {
      imported.insert(import);
    }
    database->AddAndOwn(file);
  }
  absl::string_view root_file;
  for (absl::string_view file : filenames) {
    if (imported.count(file) == 0) {
      if (!root_file.empty()) {
        return absl::InvalidArgumentError(absl::StrCat(
            "Provided proto descriptor set has multiple root files:\n",
            root_file, "\n", file));
      }
      root_file = file;
    }
  }
  return root_file;
}

// GetParamsDescriptor traverses the given proto file in the descriptor pool,
// and determines which contained message is the parameter message.
// If the file specifies the (ocpdiag.options).params_message file option, then
// it will use that as the params message. If it's missing and there's only
// a single message in the root file, it will use that.
absl::StatusOr<const google::protobuf::Descriptor*> GetParamsDescriptor(
    const google::protobuf::DescriptorPool* pool, absl::string_view root_file) {
  const google::protobuf::FileDescriptor* file =
      pool->FindFileByName(std::string(root_file));
  if (file == nullptr) {
    return absl::InvalidArgumentError(
        absl::StrCat("Root file \"", root_file, "\" not found in pool."));
  }
  const google::protobuf::Descriptor* param_type = nullptr;
  const google::protobuf::FileOptions& options = file->options();
  if (!options.HasExtension(ocpdiag::options)) {
    if (file->message_type_count() == 1) {
      return file->message_type(0);
    }
    return absl::InvalidArgumentError(
        "Parameters schema does not have a single message, "
        "and does not provide an option to identify the correct one.");
  }
  const ocpdiag::FileOptions& ocpdiag_options =
      options.GetExtension(ocpdiag::options);
  param_type = pool->FindMessageTypeByName(ocpdiag_options.params_message());
  if (param_type == nullptr) {
    return absl::InvalidArgumentError(
        absl::StrCat("OCPDiag file option (ocpdiag.options).params_message=\"",
                     ocpdiag_options.params_message(),
                     "\" is not a fully-qualified message "
                     "name."));
  }
  return param_type;
}

// OverrideFlagParams will attempt to use all passed flags to assign values
// in the given params message. Any successfully assigned flag will be removed
// from the flags argument. Any failed assignment will be returned in the status
// vector, so the caller can emit them in case the remaining flags are invalid.
std::vector<absl::Status> OverrideFlagParams(
    google::protobuf::Message* params,
    std::vector<OCPDiagParameterParser::FlagArg>& flags) {
  std::vector<absl::Status> errors;
  std::vector<OCPDiagParameterParser::FlagArg>::iterator end = flags.begin();
  for (auto i = flags.begin(); i != flags.end(); ++i) {
    if (absl::Status err = AssignPath(params, i->key, i->value); !err.ok()) {
      std::swap(*end, *i);
      ++end;
      errors.emplace_back(std::move(err));
    }
  }
  flags.resize(end - flags.begin());
  return errors;
}

// PrettyAppend will append the passed string to the given cord, making sure to
// wrap at word boundaries to be within 80 columns, and indent 6 spaces on each
// new line. It returns the current length of the line for repeated calls.
// A leading space is added when space is true, or if the passed string has a
// leading space.
size_t PrettyAppend(absl::Cord* formatted, size_t cursor,
                    absl::string_view to_append, bool space = false) {
  static constexpr size_t kMaxLineLen = 80;
  std::vector<absl::string_view> pieces;
  for (auto line : absl::StrSplit(to_append, absl::ByAnyChar("\n\r"))) {
    if (!pieces.empty()) {
      pieces.push_back("\n");
    }
    for (auto word :
         absl::StrSplit(line, absl::ByAnyChar(" \t"), absl::SkipEmpty())) {
      pieces.push_back(word);
    }
  }
  bool word_prior = space || (to_append.front() == ' ');
  for (absl::string_view piece : pieces) {
    bool newline = cursor == 0;
    if (piece == "\n") {
      formatted->Append("\n");
      cursor = 0;
      continue;
    }
    if (!newline && cursor + piece.size() >= kMaxLineLen) {
      formatted->Append("\n");
      cursor = 0;
      newline = true;
    }
    if (newline) {
      formatted->Append("      ");
      cursor += 6;
    } else if (word_prior) {
      formatted->Append(" ");
      ++cursor;
    }
    formatted->Append(piece);
    cursor += piece.size();
    word_prior = true;
  }
  return cursor;
}

// FormatOverrideFlag will create the help text for a single field flag entry.
absl::Cord FormatOverrideFlag(
    absl::Span<const google::protobuf::FieldDescriptor*> ancestry,
    const google::protobuf::FieldDescriptor* field, bool array_entry = false) {
  absl::Cord help;
  help.Append("    --");
  for (const google::protobuf::FieldDescriptor* ancestor : ancestry) {
    help.Append(ancestor->name());
    if (ancestor->is_repeated()) {
      help.Append("[#]");
    }
    help.Append(".");
  }
  help.Append(field->name());
  // Add the index for invidual entry flags.
  if (field->is_repeated() && array_entry) {
    help.Append("[#]");
  }
  // Use proto file comments as flag documentation.
  // In bazel, these are only inluded in the decriptor when run with
  // --protocopt=--include_source_info. Unfortunately, there doesn't seem to
  // be a way to set this in the proto_library rule.
  size_t cursor = PrettyAppend(&help, help.size(), " (");
  if (google::protobuf::SourceLocation location; field->GetSourceLocation(&location) &&
                                       !(location.leading_comments.empty() &&
                                         location.trailing_comments.empty())) {
    auto trim = [](absl::string_view trim) {
      trim.remove_prefix(trim.find_first_not_of(" \t\n\r"));
      trim.remove_suffix(trim.size() - trim.find_last_not_of(" \t\n\r"));
      return trim;
    };
    cursor = PrettyAppend(&help, cursor, trim(location.leading_comments));
    cursor = PrettyAppend(&help, cursor, trim(location.trailing_comments));
  }
  cursor = PrettyAppend(&help, cursor, "); type:");
  absl::string_view type_name;
  switch (field->type()) {
    case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
      type_name = field->message_type()->full_name();
      break;
    case google::protobuf::FieldDescriptor::TYPE_ENUM:
      type_name = field->enum_type()->full_name();
      break;
    default:
      type_name = field->type_name();
  }
  // Indicate the type is an array if this flag is not for an individual entry.
  if (field->is_repeated() && !array_entry) {
    cursor =
        PrettyAppend(&help, cursor, absl::StrCat("[", type_name, "];"), true);
  } else {
    cursor = PrettyAppend(&help, cursor, absl::StrCat(type_name, ";"), true);
  }
  // Add a second flag entry for overriding individual array values.
  if (field->is_repeated() && !array_entry) {
    help.Append("\n");
    help.Append(FormatOverrideFlag(ancestry, field, true));
  }
  return help;
}

// OverriderHelpPrinter is a depth-first recursive descriptor search. It outputs
// the flag help for every encountered field. It won't continue down recursive
// types.
absl::Cord OverrideHelpPrinter(
    const google::protobuf::Descriptor* param,
    std::vector<const google::protobuf::FieldDescriptor*>& ancestry) {
  absl::Cord output;
  for (int i = 0; i < param->field_count(); ++i) {
    const google::protobuf::FieldDescriptor* field = param->field(i);
    output.Append("\n");
    output.Append(FormatOverrideFlag(absl::MakeSpan(ancestry), field));
    if (field->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
      bool recursive = false;
      for (const google::protobuf::FieldDescriptor* ancestor : ancestry) {
        if (ancestor->message_type() == field->message_type()) {
          recursive = true;
          break;
        }
      }
      if (!recursive) {
        ancestry.push_back(field);
        output.Append(OverrideHelpPrinter(field->message_type(), ancestry));
        ancestry.pop_back();
      }
    }
  }
  return output;
}

// GenerateMessageOverrideHelp creates an additional help text section with
// arguments for each overridable field in the parameter descriptor.
absl::Cord GenerateMessageOverrideHelp(const google::protobuf::Descriptor* param,
                                       const google::protobuf::Message& defaults) {
  std::vector<const google::protobuf::FieldDescriptor*> ancestry;
  absl::Cord help = OverrideHelpPrinter(param, ancestry);
  if (!help.empty()) {
    help.Prepend("  Flags from parameter overrides:");
    google::protobuf::util::JsonPrintOptions options;
    options.always_print_primitive_fields = true;
    options.add_whitespace = true;
    options.preserve_proto_field_names = true;
    std::string defaults_json;
    AsAbslStatus(
        google::protobuf::util::MessageToJsonString(defaults, &defaults_json, options))
        .IgnoreError();
    help.Append("\n\n  Environment Variables:");
    for (absl::string_view env_var : kEnvironmentHelp) {
      help.Append("\n    ");
      PrettyAppend(&help, 4, env_var);
    }
    help.Append("\n\n  Default Parameters:\n");
    for (auto line : absl::StrSplit(defaults_json, absl::ByAnyChar("\n\r"))) {
      if (!line.empty()) {
        help.Append("    ");
        help.Append(line);
        help.Append("\n");
      }
    }
  }
  return help;
}

// Packs arguments into the ExecArgs output.
// Assumes that the 1st positional argument is the executable name.
void PackExecArgs(const OCPDiagParameterParser::Arguments& args,
                  OCPDiagParameterParser::ExecArgs& output) {
  size_t extra_args = 0;
  for (const OCPDiagParameterParser::FlagArg& flag : args.flags) {
    extra_args += flag.sources.size();
  }
  output.execv.reserve(args.passthrough.size() + extra_args + 2);
  output.execv.push_back(args.unparsed[OCPDiagParameterParser::kTestExecutable]);
  for (const char* const arg : args.passthrough) {
    output.execv.push_back(arg);
  }
  for (const OCPDiagParameterParser::FlagArg& arg : args.flags) {
    for (const char* const charg : arg.sources) {
      output.execv.push_back(charg);
    }
  }
  output.execv.push_back(nullptr);
}

}  // namespace

absl::StatusOr<OCPDiagParameterParser::ExecArgs>
OCPDiagParameterParser::PrepareExec(Arguments args,
                                   google::protobuf::io::ZeroCopyInputStream* json_stream,
                                   bool json_newlines) {
  absl::Span<const char* const> pos_args = args.unparsed;
  if (pos_args.size() < kOptionalJSONDefaults ||
      pos_args.size() > kMaxNumArgs) {
    return absl::InvalidArgumentError(absl::StrCat(
        "Invalid arguments. Usage:\n",
        !pos_args.empty() ? pos_args[kLauncherName] : "ocpdiag_launcher",
        " ExecPath FileDescriptorSet [JSON defaults] [--flags] "
        "[-- [test args]]"));
  }

  // The database, pool, and factory all need to remain valid as long as
  // any instance of the parameter is still alive.
  google::protobuf::SimpleDescriptorDatabase database;
  absl::StatusOr<absl::string_view> root_file =
      PopulateTypeDatabase(pos_args[kFileDescriptors], &database);
  if (!root_file.ok()) {
    return root_file.status();
  }

  // Get the descriptor for the parameter messages.
  google::protobuf::DescriptorPool pool{&database};
  absl::StatusOr<const google::protobuf::Descriptor*> param_type =
      GetParamsDescriptor(&pool, *root_file);
  if (!param_type.ok()) {
    return param_type.status();
  }

  // Create the internal params representation.
  google::protobuf::DynamicMessageFactory factory{&pool};
  std::unique_ptr<google::protobuf::Message> params{
      factory.GetPrototype(*param_type)->New()};

  // Populate the message from defaults, the user-provided file,
  // and command-line flags.
  if (pos_args.size() > kOptionalJSONDefaults) {
    if (absl::Status defaults = params::JsonFileToMessage(
            pos_args[kOptionalJSONDefaults], params.get());
        !defaults.ok()) {
      return defaults;
    }
  }

  // If 'help' was requested, then construct the override arguments and prepare
  // to exec the test with only the '--help' argument.
  for (const FlagArg& flag : args.flags) {
    if (flag.key == "help" || flag.key == "helpfull") {
      OCPDiagParameterParser::ExecArgs help;
      help.execv.resize(3);
      help.execv[0] = pos_args[kTestExecutable];
      help.execv[1] = flag.sources[0];
      help.execv[2] = nullptr;
      help.post_output =
          std::string{GenerateMessageOverrideHelp(*param_type, *params)};
      return help;
    }
  }

  if (absl::Status from_file =
          ocpdiag::params::MergeFromJsonStream(json_stream, params.get());
      !from_file.ok()) {
    return from_file;
  }

  // If a param doesn't exist, it's ambiguous whether it was meant as a regular
  // flag. In the event that the test dies before consuming its parameters,
  // assume that it was due to flag parsing errors and then emit these.
  std::vector<absl::Status> deferred_status =
      OverrideFlagParams(params.get(), args.flags);

  // Realize the completely-merged parameters.
  OCPDiagParameterParser::ExecArgs output;
  google::protobuf::util::JsonPrintOptions print_options;
  print_options.add_whitespace = json_newlines;
  print_options.preserve_proto_field_names = true;
  if (absl::Status err = AsAbslStatus(google::protobuf::util::MessageToJsonString(
          *params, &output.json_params, print_options));
      !err.ok()) {
    return err;
  }

  // Construct the execv arguments.
  PackExecArgs(args, output);

  return output;
}

OCPDiagParameterParser::Arguments OCPDiagParameterParser::ParseArgs(
    int argc, const char* argv[]) {
  Arguments out;
  int nonflags = 0;
  const char** cursor = argv;
  const char** const end = argv + argc;
  for (; cursor != end; ++cursor) {
    const char* arg = *cursor;
    // Ignore non-flags.
    if (*arg != '-') {
      argv[nonflags] = arg;
      ++nonflags;
      continue;
    }

    // Consume up to two prefixed hyphens.
    ++arg;
    if (*arg == '-') {
      ++arg;
      if (*arg == 0) {
        // Found '--' argument; all following arguments are passthrough.
        ++cursor;
        break;
      }
    }

    // Accept either --key=value or --key value flags.
    // For the space-delimited version, value cannot start with a hyphen.
    FlagArg flag;
    flag.key = arg;
    flag.sources = absl::MakeSpan(cursor, 1);
    if (size_t split = flag.key.find_first_of('=');
        split == absl::string_view::npos) {
      // Omit value if it is absent or another flag-like string.
      if (cursor + 1 != end && *cursor[1] != '-') {
        flag.sources = absl::MakeSpan(cursor, 2);
        ++cursor;
        flag.value = *cursor;
      }
    } else {
      flag.value = flag.key.substr(split + 1);
      flag.key = flag.key.substr(0, split);
    }
    out.flags.emplace_back(std::move(flag));
  }
  out.unparsed = absl::Span<const char* const>(argv, nonflags);
  out.passthrough = absl::Span<const char* const>(cursor, end - cursor);
  return out;
}

}  // namespace ocpdiag
