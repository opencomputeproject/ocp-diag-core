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

#ifndef MELTAN_CORE_TESTING_PARSE_TEXT_PROTO_H_
#define MELTAN_CORE_TESTING_PARSE_TEXT_PROTO_H_

#include "google/protobuf/message.h"
#include "google/protobuf/text_format.h"
#include "gtest/gtest.h"
#include "absl/strings/string_view.h"

namespace ocpdiag::testing {

// Replacement for Google ParseTextProtoOrDie.
// Only to be used in unit tests.
// Usage: MyMessage msg = ParseTextProtoOrDie(my_text_proto);
class ParseTextProtoOrDie {
 public:
  ParseTextProtoOrDie(std::string text_proto)
      : text_proto_(std::move(text_proto)) {}

  template <class T>
  operator T() {
    T message;
    if (!google::protobuf::TextFormat::ParseFromString(text_proto_,
                                             &message)) {
      ADD_FAILURE() << "Failed to parse textproto: " << text_proto_;
      abort();
    }
    return message;
  }

 private:
  std::string text_proto_;
};

}  // namespace ocpdiag::testing

#endif  // MELTAN_CORE_TESTING_PARSE_TEXT_PROTO_H_
