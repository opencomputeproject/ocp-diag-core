# OCPDiag result generation API codelab

<!--*
freshness: { owner: 'viel' reviewed: '2021-09-20' }
*-->



## Overview

This document introduces a reference API that can dump
[OCPDiag result artifacts](results.md#data-schema) and provides a walk-through of
using the API for a simple OCPDiag diagnostic test.

To learn how to write unit-tests for this library, see
[Results Generation API Testing](results_reference_api_testing.md).

## Glossary

**FRU**
:   Field-Replaceable Unit: Hardware components that datacenter techs can swap
    out (in contrast to embedded components).

## API structure and semantics

This section provides a quick primer on how to use the Result API.

See also [Results](results.md) for details of the OCPDiag data model and schema.

### Main classes

This API contains 4 main classes:

*   `ResultApi`: Factory class for managing the creation of the following 3.
*   `TestRun`: encompasses all results for a particular test. There should only
    be one per diagnostic.
*   `TestStep`: a logical subdivision of a TestRun.
*   `MeasurementSeries`: a collection of time-series measurements.

These data model constructs require both a`<classname>Start` and
`<classname>End` output artifact for proper result parsing, like these for a
`TestStep`:

```shell
{"testStepStart":{"name":"my_step"},"testStepId":"0"}
...
{"testStepEnd":{"name":"my_step","status":"COMPLETE"}
```

Three of the classes follow a dependence hierarchy in which each class is a
precursor to the next.

1.  `TestRun` is constructed by the `ResultApi::InitializeTestRun` method.
1.  `ResultApi::BeginTestStep` requires a valid `TestRun` object.
1.  `ResultApi::BeginMeasurementSeries` requires a valid `TestStep` object.

![results_api_classes](results_api_classes_external.png) ***Figure 1: OCPDiag
result API, main classes: How they relate and how artifacts are written***

### Info registration classes

There are three helper classes for managing and registering machine and software
metadata, such as host info, hardware info (gathered using the OCPDiag hardware
interface), and software info.

*   `DutInfo`: Organizes machine metadata for consumption by the TestRun.
*   `HwRecord`: Represents a HardwareInfo that has been added to a DutInfo.
    HwRecord only becomes useful after the DutInfo is *registered*. (See Figure
    2)
*   `SwRecord`: Represents a SoftwareInfo that has been added to a DutInfo.
    SwRecord only becomes useful after the DutInfo is *registered*. (See Figure
    2)

**The test author is expected to gather and register DutInfo(s) with the TestRun
during these phases before beginning any real diagnostic work:**

1.  Gathering DUT info
1.  Running test logic and diagnosing the hardware components

A `HwRecord` can only be constructed by passing a HardwareInfo proto to
`DutInfo::AddHardware(...)`. The same applies to `SwRecord`. The test author
must keep track of these records for later use in diagnostic result emissions.
Performing a diagnosis without any associated hardware info is not useful.

**Question**: Why can’t I just gather the hardware information when I actually
need it for a Diagnosis? \
**Answer**: In order to diagnose machines that panic, shut down, or are left in
a bad-state during testing, you need to inform the OCPDiag Data Schema what
hardware and software components you intend to validate before the test starts.
This information cannot be gathered after the test is run.

You can think of Records as read-only `HardwareInfo` and `SoftwareInfo` protos
that are used for associating a Diagnosis or Error artifact with that
information.

![results_api_hw_record](results_api_hw_record.png) ***Figure 2: OCPDiag result
API - Info registration classes and their use (SwRecord behaves in the same way
as HwRecord)***

### Test status and result

The OCPDiag Data Model separates **Status** and **Result**. These are always
displayed in the final TestRunEnd artifact, which provides a precise way to
interpret the test run.

`{"testRunEnd":{"name":"myTest","status":"COMPLETE","result":"PASS"}}`

*   **Status** refers only to *non-hardware related* health:
    *   Were there any ERRORs such as software, dependency, infrastructure, RPC
        errors, etc.?
    *   Did the software run to completion?
*   **Result** is strictly reserved for interpreting the health of the *hardware
    itself*:
    *   Did all hardware on the DUT pass the test?
    *   1+ fail-diagnoses results in an overall FAIL result.

A TestRun’s **Status** and **Result** are **updated automatically** to **ensure
that the overall result aligns with *what actually happened*** in the test,
rather than the test author’s interpretation of events. The following actions
can trigger changes in the Result and/or Status:

*   User skips the test with `TestRun.Skip()`:
    *   `TestStatus` is set to `SKIPPED`
    *   `TestResult` is set to `NOT_APPLICABLE`
*   User emits a Fail `Diagnosis`:
    *   `TestResult` is set to `FAIL`, unless an `Error` has already been
        emitted.
*   User emits any `Error` artifact (via `TestRun::AddError` or
    `TestStep::AddError`):
    *   `TestStatus` is set to `ERROR`.
    *   `TestResult` is set to `NOT_APPLICABLE`. ***This takes precedence over
        `Fail diagnoses and Skips`***.

NOTE: Don’t confuse `TestRun/TestStep::AddError(...)` with
`TestRun/TestStep::LogError(...)` - the latter are for Log artifacts and do not
affect the test result.

## Picking apart the code

This section discusses chunks of the source code from the previous section to
provide a better understanding of what the semantics look like in practice.

### Chunk 1: Test setup phase

A OCPDiag diagnostic test should always begin with initialization and parameter
parsing.

```c++
int main(int argc, char* argv[]) {
  ocpdiag::results::ResultApi api;

  // Initialize the TestRun object.
  absl::StatusOr<std::unique_ptr<TestRun>> test_run_or =
      api.InitializeTestRun("myTest");
  if (!test_run_or.ok()) {
    // This is the only place in the code where we need to write directly to
    // STDERR (because the TestRun object was not initialized).
    // Once you have an initialized TestRun, it is best practice to use
    // `TestRun::AddError()` or `TestStep::LogError()` instead.
    std::cerr << test_run_or.status().ToString() << std::endl;
    return EXIT_FAILURE;
  }
  std::unique_ptr<TestRun> test_run = std::move(*test_run_or);
  test_run->LogInfo("Initialized the test!");
  test_run->LogError("Example error log.");
  test_run->LogWarn("Example warning log.");
  test_run->AddTag("test-run-start tag");
  test_run->AddError("test-run-error", "Example test run error");

  // Parse the input parameters.
  ocpdiag::simple::Params params;
  auto status = ocpdiag::params::GetParams(&params);
  if (!status.ok()) {
    test_run->AddError(
        "my_test-procedural-error",
        absl::StrCat("Failed to parse parameters:", status.ToString()));
    return EXIT_FAILURE;
  }
```

1.  **Initialize the TestRun:** State this immediately after InitGoogle(). Use
    the TestRun object for emitting Error artifacts in case anything goes wrong
    before the code reaches the diagnostic phase.
1.  **Parse the input parameters:** These define how your OCPDiag test is
    customized at runtime. Parsing should happen *after* TestRun initialization
    in case there is a problem parsing the parameters; you can see this with
    `test_run->AddError`. The Error artifact is superior to writing a generic
    message to STDERR, as it povides information about what went wrong instead
    of only a statement that the test returned a non-zero exit code.

### Chunk 2: Host and hardware info gathering

If you want your test to run simultaneously on 2 DUTs, you need to build two
DutInfos to separate the data, as shown:

```c++
  // Make a valid DutInfo and register it with the TestRun.
  DutInfo dut_info("TestHost");
  // Build the hw info.

  HardwareInfo hw_info_with_fru;
  FillHwInfo(&hw_info_with_fru, false);
  HwRecord hw_record_with_fru = dut_info.AddHardware(hw_info_with_fru);

  HardwareInfo hw_info_with_component;
  FillHwInfo(&hw_info_with_component, true);
  HwRecord hw_record_with_component =
      dut_info.AddHardware(hw_info_with_component);

  // Build the sw info.
  SoftwareInfo sw_info;
  sw_info.set_name("my_test");
  sw_info.set_arena("myArena");
  sw_info.set_version("myVersion");
  SwRecord sw_record = dut_info.AddSoftware(sw_info);
  std::vector<DutInfo> dutinfos;
  dutinfos.emplace_back(std::move(dut_info));

  // Make a DutInfo with HW, but do not register it. Omitting the
  // HW DutInfo to the `dutinfos` vector ensures it does not get registered
  // with the TestRun and any HwRecords associated with it are not usable for
  // diagnostics.
  DutInfo unused_dut_info("UnregisteredHost");
  HardwareInfo bad_hw_info;
  bad_hw_info.set_arena("badArena");
  bad_hw_info.set_name("badName");
  bad_hw_info.set_manufacturer("badManufacturer");
  bad_hw_info.set_mfg_part_number("badMfgPartNum");
  bad_hw_info.set_part_type("badPartType");
  HwRecord unregistered_record = unused_dut_info.AddHardware(bad_hw_info);
```

NOTE: The generic strings added to the proto fields would normally be info
gathered through the OCPDiag Hardware Interface.

**Question:**: Why do we handle DutInfos with `unique_ptr`? \
**Answer:** So you know that once they are registered with the TestRun, you lose
ownership and may no longer modify the DutInfo.

### Chunk 3: Info is registered and the diagnostic phase begins

In Chunk 2 you created 2 DutInfo objects. For this example, pass and register
the first object normally (by adding it to the `dutinfos` vector) with the
`TestRun`. \
***Do not register the second object.***

**Chunk 5** illustrates what happens when you try to use the unregistered
`HwRecord` in a diagnosis.

WARNING: Don't forget to register your `DutInfos`!

```c++
  // Now the TestRun officially "starts" and emits the TestRunStart artifact.
  test_run->StartAndRegisterInfos(dutinfos, params);
  // It's now possible to construct TestSteps using this fully initialized and
  // started TestRun.

  // Create our first TestStep.
  absl::StatusOr<std::unique_ptr<TestStep>> step1_or =
      api.BeginTestStep(test_run.get(), "my_first_step");
  if (!step1_or.ok()) {
    test_run->AddError("my_test-procedural-error",
                       step1_or.status().ToString());
    return EXIT_FAILURE;
  }
```

1.  `test_run->Start...` marks the beginning of the "diagnostic" phase of the
    OCPDiag test. This is where you register the 1st DutInfo that you built in
    the previous chunk.
1.  Once TestRun starts you can create your first TestStep using
    `TestStep::Begin(...)`.

### Chunk 4: Files, tags, and artifact extensions

```c++
  // Add an example File.
  File file;
  file.set_upload_as_name("test_file");
  file.set_output_path(CreateTextFile());
  file.set_description("This is a test file :)");
  file.set_content_type("text/plain");
  file.set_is_snapshot(false);
  file.add_tags()->set_tag("ocpdiag_example");
  step2->AddFile(std::move(file));

  // Add an example tag during step run.
  test_run->AddTag("during-test-step tag");

  // Add example artifact extentions.
  step2->AddArtifactExtension("example-artifact-extention1",
                              hw_info_with_component);
  step2->AddArtifactExtension("example-artifact-extention2", bad_hw_info);
```

1.  An output file produced by the test binary is captured and marked for upload
    as an artifact.
2.  Adding a string tag to the Test.
3.  Adding a few generic artifact extensions using existing protos.

### Chunk 5: Diagnoses

```c++
  // Demonstrate a Diagnosis with good/bad HwRecord.
  //
  // This example adds one registered HwRecord and one unregistered one.
  // This illustrates that using an unregistered HwRecord
  // emits an Error artifact and the HwRecord is not
  // referenced in the Diagnosis result.
  step2->AddDiagnosis(
      third_party::ocpdiag::results_pb::Diagnosis_Type::Diagnosis_Type_PASS,
      "my_test-good-myHardwareUnregistered", "my hardware is good!",
      {unregistered_record});

  step2->AddDiagnosis(
      third_party::ocpdiag::results_pb::Diagnosis_Type::Diagnosis_Type_PASS,
      "my_test-good-myHardwareWithFruLocation", "my hardware is good!",
      {hw_record_with_fru});

  step2->AddDiagnosis(
      third_party::ocpdiag::results_pb::Diagnosis_Type::Diagnosis_Type_PASS,
      "my_test-good-myHardwareWithComponentLocation", "my hardware is good!",
      {hw_record_with_component});
```

1.  Adding a `Diagnosis` using an unregistered `HwRecord` produces a `Diagnosis`
    artifact and an `Error` artifact, warning that the record is unregistered.
1.  This is a well formed diagnosis; the hardware record shows the FRU-location.
1.  This is a well formed diagnosis; the hardware record shows
    component-location instead of the FRU.

### Chunk 6: Using a MeasurementSeries

```c++
  // Demonstrate MeasurementSeries
  MeasurementInfo meas_info;
  meas_info.set_name("my_series");
  meas_info.set_unit("awesomeness 1-10");
  absl::StatusOr<std::unique_ptr<MeasurementSeries>> series_or =
      api.BeginMeasurementSeries(step2.get(), hw_record_with_fru, meas_info);
  if (!series_or.ok()) {
    step2->AddError("my_test-procedural-error", series_or.status().ToString(),
                    {sw_record});
    return EXIT_FAILURE;
  }
  std::unique_ptr<MeasurementSeries> series = std::move(*series_or);
  Value val_max, val_min, val;
  val_max.set_number_value(10);
  val_min.set_number_value(1);
  val.set_number_value(10);
  MeasurementElement::Range range;
  *range.mutable_maximum() = val_max;
  *range.mutable_minimum() = val_min;
  series->AddElement(val_max, range);
  series->End();  // user may explicitly End a Series, Step, or TestRun.

  meas_info.set_name("another_series");
  absl::StatusOr<std::unique_ptr<MeasurementSeries>> series2 =
      api.BeginMeasurementSeries(step2.get(), hw_record_with_component,
                                 meas_info);
```

This chunk produces the following:

```shell
{"testStepArtifact":{"measurementSeriesStart":{"measurementSeriesId":"0","info":{"name":"my_series","unit":"awesomeness 1-10","hardwareInfoId":"0"}},"testStepId":"0"},"sequenceNumber":5,"timestamp":"2021-02-23T17:16:28.419997103Z"}
{"testStepArtifact":{"measurementElement":{"index":0,"measurementSeriesId":"0","range":{"maximum":10,"minimum":0},"value":10,"dutTimestamp":"2021-02-23T17:16:28.420027483Z"},"testStepId":"0"},"sequenceNumber":6,"timestamp":"2021-02-23T17:16:28.420027891Z"}
{"testStepArtifact":{"measurementSeriesEnd":{"measurementSeriesId":"0","totalMeasurementCount":1},"testStepId":"0"},"sequenceNumber":7,"timestamp":"2021-02-23T17:16:28.420072493Z"}
{"testStepArtifact":{"measurementSeriesStart":{"measurementSeriesId":"1","info":{"name":"another_series","unit":"awesomeness 1-10","hardwareInfoId":"0"}},"testStepId":"0"},"sequenceNumber":8,"timestamp":"2021-02-23T17:16:28.420097207Z"}
```

A MeasurementSeries consists of 3 parts:

*   A `MeasurementInfo`: context on how to interpret the series.
*   A registered `HwRecord`: The piece of hardware to which this series applies.
*   Any number of `MeasurementElements`, either with valid range or a valid
    value set.

### Chunk 7: ending the test

```c++
  // See that series2, TestStep and TestRun emit End artifacts when they go out
  // of scope.
  return EXIT_SUCCESS;
```

If you do not explicitly end your TestSteps, TestRuns, and MeasurementSeries (so
that timestamps are more accurate), they end automatically as they go out of
scope.

BEST PRACTICE: End your TestSteps, TestRuns, and MeasurementSeries explicitly
with their respective `End()` methods.

This produces:

```shell
{"testStepArtifact":{"measurementSeriesEnd":{"measurementSeriesId":"1","totalMeasurementCount":0},"testStepId":"0"},"sequenceNumber":9,"timestamp":"2021-02-23T17:16:28.420122965Z"}
{"testStepArtifact":{"testStepEnd":{"name":"my_step","status":"COMPLETE"},"testStepId":"0"},"sequenceNumber":10,"timestamp":"2021-02-23T17:16:28.420145035Z"}
{"testRunArtifact":{"testRunEnd":{"name":"myTest","status":"COMPLETE","result":"PASS"}},"sequenceNumber":11,"timestamp":"2021-02-23T17:16:28.420170255Z"}
```

## Best practices

### Threading and lifetimes

This API is designed to support multi-threaded tests. For proper results
processing, the TestRun object must last as long or longer than its associated
TestSteps. In practice this means that a child thread should be joined before
the TestRun goes out of scope.
