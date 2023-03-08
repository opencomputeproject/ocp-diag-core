// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/data_model/input_model_helpers.h"

#include "gtest/gtest.h"

namespace ocpdiag::results {

namespace {

TEST(TestInputModelHelpers,
     CommandLineStringSuccessfullyGeneratedFromMainArgs) {
  const char* argv[] = {"diagname", "--flag", "flag_value"};
  EXPECT_EQ(CommandLineStringFromMainArgs(3, argv),
            "diagname --flag flag_value");
}

TEST(TestInputModelHelpers,
     ParameterJsonSuccessfullyGeneratedFromMultipleArgs) {
  const char* argv[] = {"diagname", "--flag", "flag_value", "-f2", "val 2"};
  EXPECT_EQ(ParameterJsonFromMainArgs(5, argv),
            R"json({"flag":"flag_value","f2":"val 2"})json");
}

TEST(TestInputModelHelpers,
     ParameterJsonSuccessfullyGeneratedFromOneArg) {
  const char* argv[] = {"diagname", "--flag", "flag_value"};
  EXPECT_EQ(ParameterJsonFromMainArgs(3, argv),
            R"json({"flag":"flag_value"})json");
}

TEST(TestInputModelHelpers, ParameterJsonSuccessfullyGeneratedFromNoArgs) {
  const char* argv[] = {"diagname"};
  EXPECT_EQ(ParameterJsonFromMainArgs(1, argv), R"json({})json");
}

}  // namespace

}  // namespace ocpdiag::results
