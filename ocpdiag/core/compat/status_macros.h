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

#ifndef MELTAN_CORE_COMPAT_STATUS_MACROS_H_
#define MELTAN_CORE_COMPAT_STATUS_MACROS_H_

#include <filesystem>

#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"

#define RETURN_IF_ERROR(s) \
  {                        \
    auto c = (s);          \
    if (!c.ok()) return c; \
  }

// `RETURN_IF_ERROR_WITH_MESSAGE` behave the same with RETURN_IF_ERROR in
// addition with a message.
//
// A message contains a key and a value. The Value is the message content.
// The key format is "ocpdiag/function name/filename#lineno.
//
// Example:
//
//   RETURN_IF_ERROR_WITH_MESSAGE(ParseConfigs(arg), "Failed to load config");
//
//   Returns a status: "INTERNAL: Failed to parse field.
//   [ocpdiag/LoadConfig/main.cc#25='Failed to load config.']"
#define RETURN_IF_ERROR_WITH_MESSAGE(s, msg)                                   \
  {                                                                            \
    auto c = (s);                                                              \
    if (!c.ok()) {                                                             \
      c.SetPayload(                                                            \
          absl::StrFormat("ocpdiag/%s/%s#%d", __func__,                         \
                          std::filesystem::path(__FILE__).filename().string(), \
                          __LINE__),                                           \
          absl::Cord(absl::string_view(msg)));                                 \
      return c;                                                                \
    }                                                                          \
  }

#define RETURN_VOID_IF_ERROR(s) \
  {                             \
    auto c = (s);               \
    if (!c.ok()) return;        \
  }

// Executes an expression `expr` that returns an `absl::StatusOr<T>`.
// On Ok, move its value into the variable defined by `var`,
// otherwise returns from the current function.
//
// Example: Declaring and initializing a new variable (ValueType can be anything
//          that can be initialized with assignment, including references):
//   ASSIGN_OR_RETURN(ValueType value, MaybeGetValue(arg));
//
// Example: Assigning to an existing variable:
//   ValueType value;
//   ASSIGN_OR_RETURN(value, MaybeGetValue(arg));
#define ASSIGN_OR_RETURN(var, expr)           \
  MELTAN_STATUS_MACROS_ASSIGN_OR_RETURN_IMPL( \
      MELTAN_STATUS_MACROS_CONCAT(_status_or_expr, __LINE__), var, expr)

#define MELTAN_STATUS_MACROS_ASSIGN_OR_RETURN_IMPL(c, v, s) \
  auto c = (s);                                             \
  if (!c.ok()) return c.status();                           \
  v = std::move(c.value());

// Helpers for concatenating values, needed to construct "unique" name
#define MELTAN_STATUS_MACROS_CONCAT(x, y) \
  MELTAN_STATUS_MACROS_CONCAT_INNER(x, y)
#define MELTAN_STATUS_MACROS_CONCAT_INNER(x, y) x##y

// `ASSIGN_OR_RETURN_WITH_MESSAGE` behave the same with ASSIGN_OR_RETURN in
// addition with a message.
//
// A message contains a key and a value. The Value is the message content.
// The key format is "ocpdiag/function name/filename#lineno.
//
// Example:
//
//   ASSIGN_OR_RETURN_WITH_MESSAGE(value, ParseConfigs(arg),
//                                 "Failed to load config");
//
//   Returns a status: "INTERNAL: Failed to parse field.
//   [ocpdiag/LoadConfig/main.cc#25='Failed to load config.']"
#define ASSIGN_OR_RETURN_WITH_MESSAGE(var, expr, msg)       \
  MELTAN_STATUS_MACROS_ASSIGN_OR_RETURN__WITH_MESSAGE_IMPL( \
      MELTAN_STATUS_MACROS_CONCAT(_status_or_expr, __LINE__), var, expr, msg)

#define MELTAN_STATUS_MACROS_ASSIGN_OR_RETURN__WITH_MESSAGE_IMPL(c, v, s, msg) \
  auto c = (s);                                                                \
  if (!c.ok()) {                                                               \
    absl::Status r(c.status());                                                \
    r.SetPayload(                                                              \
        absl::StrFormat("ocpdiag/%s/%s#%d", __func__,                           \
                        std::filesystem::path(__FILE__).filename().string(),   \
                        __LINE__),                                             \
        absl::Cord(absl::string_view(msg)));                                   \
    return r;                                                                  \
  }                                                                            \
  v = std::move(c.value());

#endif  // MELTAN_CORE_COMPAT_STATUS_MACROS_H_
