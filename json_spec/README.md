# OCP Test and Validation Output Specification

---

# TODO: revision below? (Siamak)

---

### Version 2.0

## Introduction

The [**OCP Test and Validation Specification**](#def-spec) establishes a standardized format for the structuring and transmission of test information emitted from a [**diagnostic**](#def-diagnostic), enabling the analysis and final determination of [**verdict**](#def-verdict) on the overall health of the [**hardware or software under test**](#def-dut).

This [**specification**](#def-spec) aims to be extensible, while still providing sufficient consistency across multiple [**diagnostic tests**](#def-diagnostic) to integrate with test executive platforms and result storage systems. The level of data is intended to enable statistical process control and predictive modeling for machine learning applications. This allows for systems that support the root cause analysis of failures, and repair prediction for manufacturing hardware defects and fleet repair systems.


### <a name="objectives">Objectives of the OCP Test and Validation Specification</a>

The high level goals of the [**specification**](#def-spec) are to document the [_schema_](#outputartifacts) and the format for the transfer of information from a [**diagnostic**](#def-diagnostic) to a [**test executive**](#def-testexecutive), persistent data store, or analysis/statistical process control system.

The [_schema_](#outputartifacts) covers the organization and contents of the data being emitted from the [**diagnostic**](#def-diagnostic), whereas the format defines the way that the data is represented.

The two separate elements compose the overall representation of the data and seek to address the following high level objectives:


#### Objective 1: Standardization

This [**specification**](#def-spec) has been developed by evaluating multiple proprietary test and diagnostic formats that have been leveraged across the industry at various points in time and at different organizations. By expressing a standard format for information generated during a test execution, it allows a single [**diagnostic test**](#def-diagnostic) to be integrated into different data collection systems for later analysis. This specification aims to enable the critical diagnostic and test information to be exported to different data and analysis tools and systems for offline analysis, alerting, and debugging.


#### Objective 2: Structured Output

All consumers of test data are inherently aware of the importance of having well structured data, but in many cases, to get to this result, a "wrapping" function is required that is unique to each different [**diagnostic**](#def-diagnostic) or test, as well as each environment where that test will be leveraged. This results in a combinatorial explosion of fragile, low-value integration effort. For example, consider storage [**diagnostics**](#def-diagnostic) that are essentially wrappers around common testing utilities such as _fio_ or networking [**diagnostics**](#def-diagnostic) that are wrappers around _netperf_ or _iperf_ utilities. Utilities that grep for logging artifacts or other unstructured output are fragile and lead to test escapes.

The authors of this [**specification**](#def-diagnostic) are equally aware that each different organization interested in test and validation data will most likely have their own representation of test data that may differ from the specification presented here for many potential reasons:

* The organization has a significant investment in an existing test data schema.
* The organization's data systems are tightly coupled with other downstream systems such as manufacturing execution systems, workflow scheduling tools, conformance databases, etc.

The objective for the [**specification**](#def-spec) here is that it serves as a feature complete, structured representation of test diagnostic information that can be easily transformed into a proprietary format with the expectation that this work is _non-recurring_ engineering. An organization should be able to develop a common library of data transformation logic to go from this [_schema_](#outputartifacts) to their own schema which can be leveraged for each test or [**diagnostic**](#def-diagnostic) that is developed in the [**OCP specification**](#def-spec). In addition, although the destination format may not have all the features and properties of the data [_schema_](#outputartifacts) presented here, an objective is that the [**specification**](#def-spec) presented for this format is at parity or a higher level of fidelity than the destination system so that no data is lost in execution.


#### Objective 3: Portability

As the [**hardware under test**](#def-dut) continues to increase in complexity, the development time and non-recurring engineering investment in hardware health and validation [**diagnostics**](#def-diagnostic) and tests continues to scale roughly linearly with the increases in hardware and software complexity. As a result, it is critical that the engineering effort spent on [**diagnostic**](#def-diagnostic) and test development is able to be leveraged in many different environments, for many different functions including, but not limited to:

* Design Validation
    * HALT/HASS Testing
    * Margin Testing
* Compliance and Standards Testing
    * EMI Compliance
    * Environmental Testing
* Manufacturing Testing
    * L5/L6 level manufacturing testing "Board Level Testing"
    * L10 level "Machine Level Testing"
    * L11 level "Rack Level Testing"
* Data Center Testing
    * Incoming Quality Assurance Testing
    * Repair and Maintenance Testing
* Repair and reverse logistic operations
    * Hardware RMA
    * Failure Analysis

All of these functions are typically performed in different environments with different capabilities and IT systems. As a result, the [**OCP Test and Validation specification**](#def-spec) has embraced JSON as a common interoperability format for the invocation and export of information for [**diagnostics**](#def-diagnostic) executed on [**hardware or software under test**](#def-dut). This design decision does come with trade-offs, which will be discussed in a later section of this document.

In addition, [**diagnostic tests**](#def-diagnostic) are developed using a multitude of different languages and tools, and the necessary rich libraries to support the JSON dialect exist for common implementation languages for [**diagnostics**](#def-diagnostic) such as C, C++, Python, Go, Rust, Java, and Scala. The authors of this specification feel that JSON is still currently the most portable format for data transmission across languages that may be used for [**diagnostic**](#def-diagnostic) development and the development of systems that process and store such information.


####  Objective 4: Extensibility

The authors acknowledge that specialized needs regarding output will always exist, and that the [**specification**](#def-spec) should be extensible in a way that presents new types of data and information to be represented in a well structured manner, without impacting existing parsers and consumers of the information.

As a result, _extensibility_ has been included as a goal and feature of the test data [_schema_](#outputartifacts) presented in this document. As with all design decisions, this feature comes with a trade-off where proprietary or unsupported extensions across different [**diagnostic tests**](#def-diagnostic) can fragment the specification or lead to interoperability challenges. Support for versioning and change management are critical for the health of an ecosystem of [**diagnostics**](#def-diagnostic) built around supporting this [**specification**](#def-spec).


#### Objective 5: Support for incremental result reporting and very long running tests

The duration of [**diagnostic**](#def-diagnostic) execution can vary across different applications such as manufacturing testing vs. design validation. It's not unusual for some [**diagnostics**](#def-diagnostic) to complete their execution in milliseconds whereas others may complete in multiple days, or even weeks, as the case of ongoing reliability testing (ORT).

When executing long running tests, it is essential that the diagnostic output format supports the concept of streaming results, so that additional information can be emitted and parsed by the consumers of the diagnostic information as it is generated and transmitted. This is required for several reasons:

* If the diagnostic execution is terminated or fails to complete, the loss of large amounts of testing information may incur a financial or schedule burden on a project or program.
* A test execution environment may wish to stop a running [**diagnostic**](#def-diagnostic) in the event of a particular failure condition or threshold that has been exceeded.

A single large JSON-encoded result from [**diagnostics**](#def-diagnostic) does not support this requirement, and in order for a JSON document to be parseable, it must be complete. To support the execution of long running tests, the [**specification**](#def-spec) outlined here supports emitting diagnostic information as individual self-contained [**artifacts**](#def-artifact).

<a name="ldjson"></a>
The technology which lends itself best to this requirement is [**Line Delimited JSON**](#def-ldjson) ([definition](https://en.wikipedia.org/wiki/JSON_streaming#:~:text=Line%2Ddelimited%20JSON%20can%20be,if%20embedded%20newlines%20are%20supported.)) which makes use of the fact that the JSON format does not allow return or newline characters within primitive values (in strings, these shall be escaped as \r and \n respectively) and most JSON formatters default to not including any whitespace, including returns and newlines. These features allow the newline character or return and newline character sequence to be used as a delimiter. The use of the newline as a delimiter also enables this format to work very well with traditional line-oriented Unix tools such as grep, jq, less, etc.

Some of the [**artifacts**](#def-artifact) are stateful, having a beginning and an end message. For example, to build a [_MeasurementSeries_](#measurementseriesstart) (a time-series list of measurements), the diagnostic output model includes the [_measurementSeriesStart_](#measurementseriesstart) and [_measurementSeriesEnd_](#measurementseriesend) messages which are accompanied by additional [_MeasurementSeriesElements_](#measurementserieselement) to populate the information in the [_measurementSeries_](#measurementseriesstart) container object. The ending message is specifically designed not to allow for information loss if not present. Its omission, however, can be used to determine that there was a transmission error during the output.

<sup>For the purpose of this document's readability: while the <a href="#def-ldjson"><strong>LDJSON</strong></a> output is optimal for automated parsing, the lack of white-space does make it more difficult for human reviewers. The examples included have been converted to a 'pretty printed' format for easier consumption.</sup>

<sup>For example, a <a href="#def-spec"><strong>specification</strong></a> compliant JSON message to transmit the version of the specification format would be emitted as shown, with no newline or carriage return characters.</sup>

<sup>

```json
{"schemaVersion":{"major":2,"minor":0},"timestamp":"2022-07-25T01:33:46.953314Z","sequenceNumber":0}
```

</sup>

<sup>This will be rendered in the <a href="#def-spec"><strong>specification</strong></a> using the common 'pretty print' format:</sup>

<sup>

```json
{
  "schemaVersion": {
    "major": 2,
    "minor": 0
  },
  "timestamp": "2022-07-25T01:33:46.953314Z",
  "sequenceNumber": 0
}
```

</sup>


## <a name="definitions">Definitions used in this Specification</a>

<table>
    <tr>
        <td><strong>Term</strong></td>
        <td><strong>Definition</strong></td>
    </tr>
    <tr>
        <td><a name="def-spec"><strong>OCP Test and Validation Output Specification</strong></a></td>
        <td>The output [_schema_](#outputartifacts) as described in this document. This term may be used in various formulations throughout this document (eg. OCP spec, OCP schema, specification).</td>
    </tr>
    <tr>
        <td><a name="def-ldjson"><strong>LDJSON</strong></a></td>
        <td>Line delimited JSON, a restricted JSON format where every newline is escaped, allowing for the newline character to delimit individual JSON documents (<a href="#ldjson">explanation</a>)</td>
    </tr>
    <tr>
        <td><a name="def-artifact"><strong>Artifact</strong></a></td>
        <td>A <a href="#def-ldjson"><strong>LDJSON</strong></a> document which represents a self-contained message relaying information from a diagnostic test.</td>
    </tr>
    <tr>
        <td><a name="def-attribute"><strong>Attribute</strong></a></td>
        <td>An element or field of a message.</td>
    </tr>
    <tr>
        <td><a name="def-diagnosis"><strong>Diagnosis</strong></a></td>
        <td>An evaluation of the overall conformance of a <a href="#def-dut"><strong>unit under test</strong></a> accompanied by the supporting evidence to support the diagnosis (i.e. pass/fail).</td>
    </tr>
    <tr>
        <td><a name="def-diagnostic"><strong>Diagnostic</strong></a></td>
        <td>A software package used to determine conformance to a set of criteria for a <a href="#def-dut"><strong>hardware or software artifact under test</strong></a>.</td>
    </tr>
    <tr>
        <td><a name="def-dut"><strong>DUT</strong></a></td>
        <td>Device Under Test. The device under test may be referred to as "system under test", "component under test" or other equivalent formulations throughout this document.</td>
    </tr>
    <tr>
        <td><a name="def-nonconformant"><strong>Nonconformant</strong></a></td>
        <td>A failure to conform or meet an expected criteria for a test or <a href="#measurement"><em>measurement</em></a>. Opposite of <strong>conformant</strong>.</td>
    </tr>
    <tr>
        <td><a name="def-symptom"><strong>Symptom</strong></a></td>
        <td>An observed property or parametric <a href="#measurement"><em>measurement</em></a> which is regarded as indicating an undesirable condition of <a href="#def-nonconformant"><strong>non-conformance</strong></a> or impaired functionality.    </td>
    </tr>
    <tr>
        <td><a name="def-testexecutive"><strong>Test executive</strong></a></td>
        <td>Sequencing software used to run diagnostic packages. It may have additional responsibilities such as allocating resources for testing, aggregating/storing results, etc.</td>
    </tr>
    <tr>
        <td><a name="def-testrun"><strong>Test Run</strong></a></td>
        <td>A test run is a high level container for all the execution <a href="#def-artifact"><strong>artifacts</strong></a> that are emitted by a <a href="#def-diagnostic"><strong>diagnostic test</strong></a>.</td>
    </tr>
    <tr>
        <td><a name="def-teststep"><strong>Test Step</strong></a></td>
        <td>A test step is a high level container for all the execution artifacts that are emitted by a particular section of a <a href="#def-testrun"><strong>test run</strong></a>. A <a href="#def-testrun"><strong>test run</strong></a> consists of 0-n test steps.</td>
    </tr>
    <tr>
        <td><a name="def-uniqueidentifier"><strong>Unique Identifier</strong></a></td>
        <td>A string (as defined by JSON data types) that uniquely identifies the message it is in. An entire diagnostic output is considered to be the scope of the uniqueness property.</td>
    </tr>
    <tr>
        <td><a name="def-verdict"><strong>Verdict</strong></a></td>
        <td>A short pareto-able string indicating the determination of hardware health for a particular <a href="#def-teststep"><strong>test step</strong></a>. A single <a href="#def-diagnosis"><strong>diagnosis</strong></a> can contain one verdict.</td>
    </tr>
</table>


## Data Types

The following data types are used in this [**specification**](#def-spec) and are constrained by the JSON [specification](https://www.json.org/json-en.html#spec) itself.

<table>
    <tr>
        <td><strong>Data Type</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td>Array</td>
        <td>A list of elements consisting of the same data type.</td>
    </tr>
    <tr>
        <td>Boolean</td>
        <td>A single bit data type consisting of true or false.</td>
    </tr>
    <tr>
        <td>Null</td>
        <td>A nullable value</td>
    </tr>
    <tr>
        <td>Number</td>
        <td>An integer or floating point value expressed in base 10.</td>
    </tr>
    <tr>
        <td>Object</td>
        <td>A set of key/value pairs. Each key in the key/value pair of an object shall be a unique string.</td>
    </tr>
    <tr>
        <td>String</td>
        <td>Sequence of characters contained in double quotes. Strings support escape sequences which are outlined in the following section.</td>
    </tr>
</table>

This [**specification**](#def-spec) attempts to allow the use of common JSON parsing libraries that are available across multiple languages. This design decision comes with certain concessions that will be discussed below, such as loss of precision for numeric types. We will also address how these limitations of JSON data types can be mitigated. Alternative extensions to JSON such as OData were considered, but these come at the cost of non-standard parsing libraries.


### String Escape Sequences

The following escape sequences are supported in the diagnostic output, which are the standard JSON escape sequences. There is no expansion of the JSON specification for escaping.

<table>
    <tr>
        <td><strong>Character</strong></td>
        <td><strong>Escaped Representation</strong></td>
    </tr>
    <tr>
        <td>Backspace</td>
        <td>\b</td>
    </tr>
    <tr>
        <td>Form-Feed</td>
        <td>\f</td>
    </tr>
    <tr>
        <td>New-Line</td>
        <td>\n</td>
    </tr>
    <tr>
        <td>Carriage Return</td>
        <td>\r</td>
    </tr>
    <tr>
        <td>Tab</td>
        <td>\t</td>
    </tr>
    <tr>
        <td>Double Quote</td>
        <td>\"</td>
    </tr>
    <tr>
        <td>Backslash</td>
        <td>\\</td>
    </tr>
</table>


### Date/Time Strings

JSON does not define a native date/time type.

All date-time strings are expressed in an ISO-8601 datetime format with _optional_ timezone offset information. The format of a date-time string is expressed as `YYYY-MM-DDTHH:MM:SS` with the precision defined by the operating environment.

The time reference from [**diagnostics**](#def-diagnostic) can come from multiple sources of varying accuracy. In a manufacturing scenario, timestamps may originate from a [**device under test**](#def-dut) whose clock is not properly set, and may not be set to a timezone correctly. In a data center production environment, timing sources may be very accurate NIST traceable timings.

As a result, no guidance is provided from this [**specification**](#def-spec) regarding the source or accuracy of timestamp information. The only guidance provided is that the implementation _shall_ conform to this format, and _should_ be referenced to UTC time when possible.

**Example without time zone information**

`2022-01-05T06:37:41.211845017`


#### Time Zone support

Time zone support is considered optional, but when used the [**specification**](#def-spec) provides guidance that UTC time _should_ be used but does not specify it as a requirement. This will be done with the format `YYYY-MM-DDTHH:MM:SSZ`.

**Example with universal time zone**

`2022-01-05T06:37:41.211845017Z`

When appending timezone information to the [**specification**](#def-spec), it should follow the standard `YYYY-MM-DDThh:mm:ss[.mmm]` TZD (eg `2012-03-29T10:05:45-06:00`) with the TZD offset specifying the difference between the local timezone from UTC (Coordinated Universal Time) with format `±HH:MM`.

**Example with time zone information specified**

`2022-01-05T06:37:41.211845017-06:00`


### JSON and numeric precision

The JSON specification does not include precision requirements for numeric values. This should not be interpreted as JSON having infinite precision, but that precision is coordinated between the producing and consuming endpoints, which are free to choose the values appropriate for their requirements.

The level of numeric precision required for [**diagnostic testing**](#def-diagnostic) may exceed the limitations of common JSON parsers, which typically convert the _number_ value type of JSON message to an appropriate native value type of the implementation language. When serializing to JSON, consider any such limitations in applications that may consume your JSON. In particular, it is common for JSON numbers to be deserialized into [IEEE 754](https://en.wikipedia.org/wiki/IEEE_754) double precision numbers and thus subject to that representation's range and precision limitations.

It is however _recommended_ that the numeric values in the produced [**LDJSON**](#def-ldjson) are convertible to double precision floats.


### <a name="enums">Enumerations used in the specification</a>

The following enumerations are used in this [**specification**](#def-spec). In each case, their value is represented in JSON as an upper case string matching the enumeration value.


#### DiagnosisType

Specifies the type or outcome value of a [_Diagnosis_](#diagnosis). A diagnosis type has the following allowable values.

<table>
    <tr>
        <td><strong>Value</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><strong>PASS</strong></td>
        <td>The <a href="#def-diagnosis"><strong>diagnosis</strong></a> relates to hardware or software that was found to be <a href="#def-nonconformant"><strong>conformant</strong></a>.</td>
    </tr>
    <tr>
        <td><strong>FAIL</strong></td>
        <td>The <a href="#def-diagnosis"><strong>diagnosis</strong></a> relates to hardware or software that was found to be <a href="#def-nonconformant"><strong>non-conformant</strong></a>.</td>
    </tr>
    <tr>
        <td><strong>UNKNOWN</strong> </td>
        <td>The <a href="#def-diagnostic"><strong>diagnostic</strong></a> evaluated the <a href="#def-dut"><strong>component under test</strong></a> and could not determine whether it is functioning within specification.</td>
    </tr>
</table>


#### Severity

Specifies the seriousness of a particular diagnostic [_log_](#log) message. These values generally correspond with common log levels found in frequently used logging frameworks.

<table>
    <tr>
        <td><strong>Value</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><strong>INFO</strong></td>
        <td>Indicates normal or expected program execution.</td>
    </tr>
    <tr>
        <td><strong>DEBUG</strong></td>
        <td>Verbose information that is useful when a problem with a <a href="#def-diagnostic"><strong>diagnostic</strong></a> is being debugged by a developer or expert user.</td>
    </tr>
    <tr>
        <td><strong>WARNING</strong></td>
        <td>Indicates that an unexpected event has occurred or a more serious problem may occur in the future.</td>
    </tr>
    <tr>
        <td><strong>ERROR</strong></td>
        <td>A condition or event has occurred that has prevented the <a href="#def-diagnostic"><strong>diagnostic</strong></a> from executing normally.</td>
    </tr>
    <tr>
        <td><strong>FATAL</strong></td>
        <td>A serious error has occurred, and the <a href="#def-diagnostic"><strong>diagnostic</strong></a> shall terminate or otherwise be unable to complete execution.</td>
    </tr>
</table>


#### SoftwareType

Specifies the type of software that is represented by a [_SoftwareInfo_](#softwareinfo) message.

<table>
    <tr>
        <td><strong>Value</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><strong>UNSPECIFIED</strong></td>
        <td>Software element is not categorized.</td>
    </tr>
    <tr>
        <td><strong>FIRMWARE</strong></td>
        <td>Non-volatile software used to control component behavior on hardware e.g. SSD, NIC, BIC, BMC. This is typically programmed to a non-volatile storage device such as a SPI flash, EEPROM, etc.</td>
    </tr>
    <tr>
        <td><strong>SYSTEM</strong></td>
        <td>Operating system, drivers, kernel, or any other software that interfaces with hardware to enable utilities and applications to execute.</td>
    </tr>
    <tr>
        <td><strong>APPLICATION</strong></td>
        <td>Any other software type targeted to end-users. e.g. web browser, IDE, document creation or system software used to configure or optimize the interface to hardware through the operating system (e.g. vulnerability detection tool, storage analyzer tool) </td>
    </tr>
</table>


#### SubcomponentType

The [_SubcomponentType_](#subcomponenttype) enumeration provides additional context on the fields in a [_Subcomponent_](#subcomponent) message and how they should be interpreted. The [_subcomponent_](#subcomponent) is an optional message which can be added to a [_Diagnosis_](#diagnosis), [_Measurement_](#measurement), or [_MeasurementSeriesStart_](#measurementseriesstart) message to help provide additional isolation to a particular element of the referenced hardware info object.

<table>
    <tr>
        <td><strong>Value</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><strong>UNSPECIFIED</strong></td>
        <td><a href="#def-diagnostic"><strong>Diagnostic</strong></a> has not provided information on the subcomponent type.</td>
    </tr>
    <tr>
        <td><strong>ASIC</strong></td>
        <td>The <a href="#subcomponent"><em>subcomponent</em></a> refers to a particular device or ASIC on the referenced hardware device.</td>
    </tr>
    <tr>
        <td><strong>ASIC-SUBSYSTEM</strong></td>
        <td>The <a href="#subcomponent"><em>subcomponent</em></a> refers to a particular subsystem or functional block of an ASIC on the referenced hardware device.</td>
    </tr>
    <tr>
        <td><strong>BUS</strong></td>
        <td>The <a href="#subcomponent"><em>subcomponent</em></a> refers to a collection of pins/signals that define a particular bus on the referenced hardware device.</td>
    </tr>
    <tr>
        <td><strong>FUNCTION</strong></td>
        <td>The <a href="#subcomponent"><em>subcomponent</em></a> refers to a logical (non-physical) function on the referenced hardware device.</td>
    </tr>
    <tr>
        <td><strong>CONNECTOR</strong></td>
        <td>The <a href="#subcomponent"><em>subcomponent</em></a> refers to a particular port or off-device connector on the referenced hardware device.</td>
    </tr>
</table>


#### TestResult

The final determination of the execution outcome of a [**diagnostic test**](#def-diagnostic). This is a final determination of the overall status of the [**system under test**](#def-dut) being assessed.

<table>
    <tr>
        <td><strong>Value</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><strong>NOT_APPLICABLE</strong></td>
        <td>The test status of a <a href="#def-diagnostic"><strong>diagnostic</strong></a> was not complete so no final test result <a href="#def-verdict"><strong>verdict</strong></a> can be successfully rendered.</td>
    </tr>
    <tr>
        <td><strong>PASS</strong></td>
        <td>Test has detected no <a href="#def-nonconformant"><strong>non-conformances</strong></a> during execution.</td>
    </tr>
    <tr>
        <td><strong>FAIL</strong></td>
        <td>Test has detected <a href="#def-nonconformant"><strong>non-conformances</strong></a> related to the <a href="#def-dut"><strong>hardware or software under test</strong></a> during execution.</td>
    </tr>
</table>


#### TestStatus

The final execution status of a [**diagnostic test**](#def-diagnostic). This is a final determination of the overall execution status of the [**diagnostic**](#def-diagnostic). The [_TestResult_](#testresult) specifies the outcome of the test, and the [_TestStatus_](#teststatus) provides information concerning its execution completion state.

<table>
    <tr>
        <td><strong>Value</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><strong>COMPLETE</strong></td>
        <td>The <a href="#def-diagnostic"><strong>diagnostic</strong></a> completed execution normally.</td>
    </tr>
    <tr>
        <td><strong>ERROR</strong></td>
        <td>The <a href="#def-diagnostic"><strong>diagnostic</strong></a> did not complete execution normally due to an exception.</td>
    </tr>
    <tr>
        <td><a name="teststatus-skip"><strong>SKIP</strong></a></td>
        <td>The <a href="#def-diagnostic"><strong>diagnostic</strong></a> was skipped or did not run to normal completion as part of its execution.</td>
    </tr>
</table>


##### Valid Test Result and Test Status Combinations

The following combinations of [_TestResult_](#testresult) and [_TestStatus_](#teststatus) are considered valid for a [**diagnostic**](#def-diagnostic) compliant with this [**specification**](#def-spec).

<table>
    <tr>
        <td><strong>Test Status</strong></td>
        <td><strong>Test Result</strong></td>
        <td><strong>Example Use-Case</strong></td>
    </tr>
    <tr>
        <td><strong>SKIP</strong></td>
        <td><strong>NOT_APPLICABLE</strong></td>
        <td>
            The <a href="#def-diagnostic"><strong>diagnostic</strong></a> did not find applicable hardware to test or exercise, or an external request to abort the <a href="#def-diagnostic"><strong>diagnostic</strong></a> was received.<br/>
            The <a href="#teststatus-skip"><strong>SKIP</strong></a> status exists so that a final passing <a href="#testresult"><em>TestResult</em></a> where no testing took place is not ambiguous with a <a href="#testresult"><em>TestResult</em></a> where hardware was tested and found to be <a href="#def-nonconformant"><strong>conformant</strong></a>.
        </td>
    </tr>
    <tr>
        <td><strong>ERROR</strong></td>
        <td><strong>NOT_APPLICABLE</strong></td>
        <td>The <a href="#def-diagnostic"><strong>diagnostic</strong></a> encountered an error that prevented normal execution and determination of a pass/fail status.</td>
    </tr>
    <tr>
        <td><strong>COMPLETE</strong></td>
        <td><strong>PASS</strong></td>
        <td>The <a href="#def-diagnostic"><strong>diagnostic</strong></a> was completed successfully, and no <a href="#def-nonconformant"><strong>non-conformances</strong></a> were detected on the tested hardware resulting in a passing <a href="#def-diagnosis"><strong>diagnosis</strong></a>.</td>
    </tr>
    <tr>
        <td><strong>COMPLETE</strong></td>
        <td><strong>FAIL</strong></td>
        <td>The <a href="#def-diagnostic"><strong>diagnostic</strong></a> was completed successfully, and one or more <a href="#def-nonconformant"><strong>non-conformances</strong></a> were detected on the tested hardware resulting in a failing <a href="#def-diagnosis"><strong>diagnosis</strong></a>.</td>
    </tr>
</table>

Any other combination of [_TestResult_](#testresult) and [_TestStatus_](#teststatus) shall be considered invalid with this [**specification**](#def-spec).


#### ValidatorType

The following [_validator_](#validator) comparison types provide arithmetic and string comparison operations that can be used to assess a [_measurement_](#measurement) or [_measurement series element_](#measurementserieselement) against a set of expected values or valid thresholds. The [_validators_](#validator) are defined in such a way that the [_measurement_](#measurement) value is considered to be on the left side of the comparison operator, and the comparison value is on the right of the operator.

For example a [LESS_THAN](#validatortype-lessthan) validator will validate that a [_measurement_](#measurement) is "less than" the comparison value.

<table>
    <tr>
        <td><strong>Value</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><strong>EQUAL</strong></td>
        <td>Validates that a <a href="#measurement"><em>measurement</em></a> and a comparison value are equal.</td>
    </tr>
    <tr>
        <td><strong>NOT_EQUAL</strong></td>
        <td>Validates that a <a href="#measurement"><em>measurement</em></a> and a comparison value are not equal.</td>
    </tr>
    <tr>
        <td><a name="validatortype-lessthan"><strong>LESS_THAN</strong></a></td>
        <td>Validates that a <a href="#measurement"><em>measurement</em></a> is less than a comparison value.</td>
    </tr>
    <tr>
        <td><strong>LESS_THAN_OR_EQUAL</strong></td>
        <td>Validates that a <a href="#measurement"><em>measurement</em></a> is less than or equal to a comparison value.</td>
    </tr>
    <tr>
        <td><strong>GREATER_THAN</strong></td>
        <td>Validates that a <a href="#measurement"><em>measurement</em></a> is greater than a comparison value.</td>
    </tr>
    <tr>
        <td><strong>GREATER_THAN_OR_EQUAL</strong></td>
        <td>Validates that <a href="#measurement"><em>measurement</em></a> is greater than or equal to a comparison value.</td>
    </tr>
    <tr>
        <td><strong>REGEX_MATCH</strong></td>
        <td>Validates that a <a href="#measurement"><em>measurement</em></a> is found to match one or more regex strings contained by the <a href="#validator"><em>validator</em></a>.</td>
    </tr>
    <tr>
        <td><strong>REGEX_NO_MATCH</strong></td>
        <td>Validates that a <a href="#measurement"><em>measurement</em></a> does not match any regex strings contained by the <a href="#validator"><em>validator</em></a>.</td>
    </tr>
    <tr>
        <td><strong>IN_SET</strong></td>
        <td>Validates that a <a href="#measurement"><em>measurement</em></a> exists within a set of allowable values contained by the <a href="#validator"><em>validator</em></a>.</td>
    </tr>
    <tr>
        <td><strong>NOT_IN_SET</strong></td>
        <td>Validates that a <a href="#measurement"><em>measurement</em></a> does not exist within a set of allowables values contained by the <a href="#validator"><em>validator</em></a>.</td>
    </tr>
</table>


### Identifiers

Due to the streaming nature of the [**specification**](#def-spec), a common implementation pattern is the adoption of an identifier to indicate a relationship between an emitted artifact and a feature of the [**system under test**](#def-dut). Examples include identifiers for hardware components and software elements of the [**device under test**](#def-dut).

In both cases, these hardware/software related identifiers are emitted at the beginning of the test output so that they can be referenced from subsequent artifacts.

In addition, each [_Test Step_](#test-step-artifacts), [_measurement_](#measurement) and time series [_measurement element_](#measurementserieselement) also have an id attribute to allow other [**artifacts**](#def-artifact) to reference them by association, such as an [_error_](#error) being associated to a particular software element, or a [_diagnosis_](#diagnosis) being associated to one or more [hardware components](#hardwareinfo).

The [**specification**] provides guidance that implementations _should_ use [**unique**](#def-uniqueidentifier) increasing zero based integers for identifiers, but only specifies that they _shall_ be [**unique**](#def-uniqueidentifier) within the context of the overall [_Test Run_](#test-run-artifacts) execution [**artifact**](#def-artifact). Although GUID's are an acceptable implementation, there is no expectation of any global uniqueness across multiple [**test runs**](#def-testrun).


### Optional Values

Due to the verbosity of streaming [**LDJSON**](#def-ldjson), any optional fields in message types specified below, where the field is specifically called out as _optional_, may omit the field from a generated artifact message. An implementation where an empty field is included and defined as _null_ shall also be considered a compliant implementation. The choice between these two approaches for handling optional fields is considered an implementation detail, and any parsers that consume the diagnostic output format _shall_ be compatible with both implementations to be considered compliant with the [**specification**](#def-spec).

The following examples below are _both_ considered valid representations of an optional field for a message defined in this specification.

#### Example 1: Specifying a null value for an optional field.

```json
{
  "requiredField": true,
  "optionalField": null
}
```

#### Example 2: Omitting an optional field

```json
{
  "requiredField": true
}
```


## OutputArtifacts

All output [**artifacts**](#def-artifact) are contained by an output artifact object, which is a top level container for all output messages.

The [_OutputArtifact_](#outputartifacts) is the single parseable output object from a [**diagnostic**](#def-diagnostic) and represents the atomic unit of transmission from a [**diagnostic**](#def-diagnostic) output. The [_OutputArtifact_](#outputartifacts) can contain one of the following descendant [**artifacts**](#def-artifact):


### Direct Descendants of the OutputArtifact

<table>
    <tr>
        <td><strong>Object</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><a href="#schemaversion"><em>SchemaVersionArtifact</em></a></td>
        <td>An informational element conveying details about the data to be exported, primarily the version of the emitted data to follow. </td>
    </tr>
    <tr>
        <td><a href="#test-run-artifacts"><em>TestRunArtifact</em></a></td>
        <td>An informational element conveying details about a top level <a href="#def-testrun"><strong>test run</strong></a>. A <a href="#def-testrun"><strong>test run</strong></a> consists of zero or more <a href="#def-teststep"><strong>test steps</strong></a>.</td>
    </tr>
    <tr>
        <td><a href="#test-step-artifacts"><em>TestStepArtifact</em></a></td>
        <td>An informational element conveying details about a <a href="#def-teststep"><strong>test step</strong></a> which was executed as part of a <a href="#def-testrun"><strong>test run</strong></a>.</td>
    </tr>
</table>


### OutputArtifact Attributes

All top level [_OutputArtifacts_](#outputartifacts) contain the following attributes in addition to one artifact message which may be a [_SchemaVersionArtifact_](#schemaversion), [_TestRunArtifact_](#test-run-artifacts), or [_TestStepArtifact_](#test-step-artifacts) message.

<table>
    <tr>
        <td><strong>Attribute</strong></td>
        <td><strong>Type</strong></td>
        <td><strong>Required</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><em>sequenceNumber</em></td>
        <td>number (integer)</td>
        <td><strong>Yes</strong></td>
        <td>A monotonically increasing field indicating the order of output returned from a <a href="#def-diagnostic"><strong>diagnostic</strong></a>. A sequence number is valuable for determining if information regarding a <a href="#def-diagnostic"><strong>diagnostic's</strong></a> execution has been lost in transmission or transformation and for re-sequencing possibly out of order transmissions.</td>
    </tr>
    <tr>
        <td><em>timestamp</em></td>
        <td>string</td>
        <td><strong>Yes</strong></td>
        <td>A timestamp indicating when the information was generated by the <a href="#def-diagnostic"><strong>diagnostic</strong></a> returned in an ISO date-time format (see <a href="#datetime-strings">Date/Time strings</a> for details).</td>
    </tr>
    <tr>
        <td><strong><em>&lt;artifact&gt;</em></strong></td>
        <td>JSON: object<br/>Message: various</td>
        <td><strong>Yes</strong></td>
        <td>A <a href="#schemaversion"><em>SchemaVersionArtifact</em></a>, <a href="#test-run-artifacts"><em>TestRunArtifact</em></a>, or <a href="#test-step-artifacts"><em>TestStepArtifact</em></a> message.</td>
    </tr>
</table>

A diagram of the complete result structure of this [**specification**](#def-spec) for data messages is shown below.

---

# TODO:

![schema_diagram](images/schema_diagram.png "Full schema diagram")

---


### OutputArtifact Descendents

#### SchemaVersion

##### Description

The [_schema_](#outputartifacts) version is used to indicate the version of the [**OCP Test and Validation schema**](#def-spec) to the consuming client. This message _shall_ be the first message emitted by the [**diagnostic**](#def-diagnostic) and will be used to control the parsing of all following output artifact messages.

The format and content of the [_schemaVersion_](#schemaversion) message is intended to be static and will be consistent across all future versions of the [**OCP Test and Validation specification**](#def-spec).

The schema version is defined by two fields, a major and minor version.

A major change to the [_schema_](#outputartifacts) represents a breaking change that affects the parsing of the ensuing information. Examples would include renaming/deletion of fields, removal of enumeration values, or a change to cardinality of parent/child relationships of messages. A change to the major version automatically resets the minor version back to a value of zero.

A minor change to the [_schema_](#outputartifacts) represents a non-breaking change such as the addition of a field or a clarification of documentation/intent which would not compromise the accuracy of any existing [**diagnostics**](#def-diagnostic) or [**tests**](#def-diagnostic) that adhere to the [_schema_](#outputartifacts).

##### Attributes

<table>
    <tr>
        <td><strong>Attribute</strong></td>
        <td><strong>JSON Type</strong></td>
        <td><strong>Required</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><em>major</em></td>
        <td>number (integer)</td>
        <td><strong>Yes</strong></td>
        <td>Major version of the <a href="#def-spec"><strong>OCP Test and Validation output specification</strong></a>.</td>
    </tr>
    <tr>
        <td><em>minor</em></td>
        <td>number (integer)</td>
        <td><strong>Yes</strong></td>
        <td>Minor version of the <a href="#def-spec"><strong>OCP test and Validation output specification</strong></a>.</td>
    </tr>
</table>

##### Example

```json
{
  "schemaVersion": {
    "major": 2,
    "minor": 0
  },
  "timestamp": "2022-07-25T01:33:46.953314Z",
  "sequenceNumber": 0
}
```


#### Test Run Artifacts

A [_testRunArtifact_](#test-run-artifacts) contains a single field which is a message of the valid types for a test run.

<table>
    <tr>
        <td><strong>Attribute</strong></td>
        <td><strong>Type</strong></td>
        <td><strong>Required</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><strong><em>&lt;artifact&gt;</em></strong></td>
        <td>JSON: object<br/>Message: various</td>
        <td><strong>Yes</strong></td>
        <td>The <a href="#def-artifact"><strong>artifact</strong></a> output message.</td>
    </tr>
</table>

The following message types are valid messages that may be populated in the artifact field of a [_testRunArtifact_](#test-run-artifacts) message.

<table>
    <tr>
        <td><strong>Object</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><a href="#log"><em>Log</em></a></td>
        <td>An entry of free-form text documenting the output of a diagnostic execution typically meant for the purpose of debugging or tracing program execution.</td>
    </tr>
    <tr>
        <td><a href="#error"><em>Error</em></a></td>
        <td>An exception or unexpected event has occurred in the <a href="#def-diagnostic"><strong>diagnostic's</strong></a> execution</td>
    </tr>
    <tr>
        <td><a href="#testrunstart"><em>TestRunStart</em></a></td>
        <td>Contains information about the invocation or start of a particular <a href="#def-testrun"><strong>test run</strong></a>.</td>
    </tr>
    <tr>
        <td><a href="#testrunend"><em>TestRunEnd</em></a></td>
        <td>Contains information about the completion and final status of a particular <a href="#def-testrun"><strong>test run</strong></a>.</td>
    </tr>
</table>


#### Test Step Artifacts

A [_testStepArtifact_](#test-step-artifacts) refers to data and information associated with a particular sub-sequence of a [**diagnostic's**](#def-diagnostic) overall execution.

<table>
    <tr>
        <td><strong>Attribute</strong></td>
        <td><strong>Type</strong></td>
        <td><strong>Required</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><em>testStepId</em></td>
        <td>string</td>
        <td><strong>Yes</strong></td>
        <td>A <a href="#def-uniqueidentifier"><strong>unique identifier</strong></a> for the <a href="#def-teststep"><strong>test step</strong></a>. This identifier is used to associate any additional messages with the <a href="#def-teststep"><strong>test step</strong></a>.</td>
    </tr>
    <tr>
        <td><strong><em>&lt;artifact&gt;</em></strong></td>
        <td>JSON: object<br/>Message: various</td>
        <td><strong>Yes</strong></td>
        <td>The artifact test step message.</td>
    </tr>
</table>

The following artifacts are supported attributes of a test step artifact:

<table>
    <tr>
        <td><strong>Object</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><a href="#teststepstart"><em>TestStepStart</em></a></td>
        <td>Information pertaining to the starting condition of a <a href="#def-teststep"><strong>test step</strong></a>.</td>
    </tr>
    <tr>
        <td><a href="#teststepend"><em>TestStepEnd</em></a></td>
        <td>Information pertaining to the ending condition of a <a href="#def-teststep"><strong>test step</strong></a>.</td>
    </tr>
    <tr>
        <td><a href="#measurement"><em>Measurement</em></a></td>
        <td>Information concerning a discrete <strong>measurement</strong> or telemetry from a <a href="#def-dut"><strong>device under test</strong></a>.</td>
    </tr>
    <tr>
        <td><a href="#measurementseriesstart"><em>MeasurementSeriesStart</em></a></td>
        <td>Information concerning the beginning of a <strong>measurement series</strong>.</td>
    </tr>
    <tr>
        <td><a href="#measurementseriesend"><em>MeasurementSeriesEnd</em></a></td>
        <td>Information concerning the ending of a <strong>measurement series</strong>.</td>
    </tr>
    <tr>
        <td><a href="#measurementserieselement"><em>MeasurementSeriesElement</em></a></td>
        <td>Information concerning a discrete observation contained in a <strong>measurement</strong> or <strong>measurement series</strong>.</td>
    </tr>
    <tr>
        <td><a href="#diagnosis"><em>Diagnosis</em></a></td>
        <td>The identification of the status of a <a href="#def-dut"><strong>device or system under test</strong></a> based on the examination of measurements and <a href="#def-symptom"><strong>symptoms</strong></a>.</td>
    </tr>
    <tr>
        <td><a href="#error"><em>Error</em></a></td>
        <td>Information concerning an invalid state or condition that was encountered during the execution of a <a href="#def-diagnostic"><strong>diagnostic</strong></a>.</td>
    </tr>
    <tr>
        <td><a href="#file"><em>File</em></a></td>
        <td>An arbitrary collection of related information that was gathered in a file produced by the execution of a <a href="#def-diagnostic"><strong>diagnostic</strong></a>. The content type is specified by the meta data included in this artifact type. Storage backend of the file may vary.</td>
    </tr>
    <tr>
        <td><a href="#log"><em>Log</em></a></td>
        <td>An entry of free-form text documenting the output of a <a href="#def-diagnostic"><strong>diagnostic</strong></a> execution typically meant for the purpose of debugging or tracing program execution.</td>
    </tr>
    <tr>
        <td><a href="#extension"><em>Extension</em></a></td>
        <td>An extensible data type that allows for new objects to be added to the <a href="#def-diagnostic"><strong>diagnostic</strong></a> specification that will be compatible with existing parsers.</td>
    </tr>
</table>


### Artifact Message Details

The following section of the [**specification**](#def-spec) presents an alphabetical comprehensive list of all output [**artifacts**](#def-artifact) and examples of each element.

#### Diagnosis

##### Description

A [_diagnosis_](#diagnosis) gives the verdict of the health status for hardware [**components under test**](#def-dut).

##### Attributes

<table>
    <tr>
        <td><strong>Attribute</strong></td>
        <td><strong>Type</strong></td>
        <td><strong>Required</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><em>verdict</em></td>
        <td>string</td>
        <td><strong>Yes</strong></td>
        <td>A short string describing the overall determination of the <a href="#def-diagnostic"><strong>diagnostic</strong></a> (e.g. cpu-good, hdd-overtemp, etc). These short human readable strings are typically used to aggregate counts of observed <a href="#def-symptom"><strong>symptoms</strong></a>.</td>
    </tr>
    <tr>
        <td><em>type</em></td>
        <td>JSON: string<br/>Message: enum <a href="#diagnosistype">DiagnosisType</a></td>
        <td><strong>Yes</strong></td>
        <td>The overall assessment of the result of the diagnostic execution, expressed as a <a href="#diagnosistype"><em>DiagnosisType</em></a> enum value.</td>
    </tr>
    <tr>
        <td><em>message</em></td>
        <td>string</td>
        <td>No</td>
        <td>Detailed explanation why the <a href="#diagnosis"><em>Diagnosis</em></a> gives the above verdict of the hardware health.</td>
    </tr>
    <tr>
        <td><em>hardwareInfoId</em></td>
        <td>string</td>
        <td>No</td>
        <td>Hardware identifier to which the <a href="#diagnosis"><em>diagnosis</em></a> applies. The referenced info ID information can be found in the <a href="#dutinfo"><em>DutInfo</em></a> of <a href="#testrunstart"><em>TestRunStart</em></a>.</td>
    </tr>
    <tr>
        <td><em>subcomponent</em></td>
        <td>JSON: object<br/>Message: <a href="#subcomponent"><em>Subcomponent</em></a></td>
        <td>No</td>
        <td>Provides additional information about a specific functional <a href="#subcomponent"><em>subcomponent</em></a> of the hardware referenced by ID. If present, the <a href="#diagnosis"><em>diagnosis</em></a> applies to this <a href="#subcomponent"><em>subcomponent</em></a> instead of the parent hardware component.</td>
    </tr>
</table>


##### Example

```json
{
  "testStepArtifact": {
    "diagnosis": {
      "verdict": "mlc-intranode-bandwidth-pass",
      "type": "PASS",
      "message": "intranode bandwidth within threshold.",
      "hardwareInfoId": "1",
      "subcomponent": {
        "type": "BUS",
        "name": "QPI1",
        "location": "CPU-3-2-3",
        "version": "1",
        "revision": "0"
      }
    },
    "testStepId": "1"
  },
  "sequenceNumber": 1,
  "timestamp": "2022-07-25T02:15:58.296032Z"
}
```


#### DutInfo

##### Description

The DUT info contains information about the [**device under test**](#def-dut) as determined by the [**diagnostic**](#def-diagnostic). It encompasses information about the device itself, its hardware and software configurations.

##### Attributes

<table>
    <tr>
        <td><strong>Attribute</strong></td>
        <td><strong>Type</strong></td>
        <td><strong>Required</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><em>dutInfoId</em></td>
        <td>string</td>
        <td><strong>Yes</strong></td>
        <td>A <a href="#def-uniqueidentifier"><strong>unique identifier</strong></a> for the <a href="#def-dut"><strong>device under test</strong></a>.</td>
    </tr>
    <tr>
        <td><em>name</em></td>
        <td>string</td>
        <td>No</td>
        <td>The name of the <a href="#def-dut"><strong>device under test</strong></a> against which the <a href="#def-diagnostic"><strong>diagnostic</strong></a> is currently executing.</td>
    </tr>
    <tr>
        <td><em>metadata</em></td>
        <td>JSON: object<br/>Message: <a href="#metadata"><em>Metadata</em></a></td>
        <td>No</td>
        <td>An arbitrary non-nested JSON object containing metadata about the <a href="#def-dut"><strong>DUT</strong></a>.</td>
    </tr>
    <tr>
        <td><em>platformInfos</em></td>
        <td>JSON: array<br/>Message: list of <a href="#platforminfo"><em>PlatformInfo</em></a></td>
        <td>No</td>
        <td>A list of <a href="#platforminfo"><em>platformInfo</em></a> objects containing information about the platform under test.</td>
    </tr>
    <tr>
        <td><em>hardwareInfos</em></td>
        <td>JSON: array<br/>Message: list of <a href="#hardwareinfo"><em>HardwareInfo</em></a></td>
        <td>No</td>
        <td>A list of hardware components discovered by the <a href="#def-diagnostic"><strong>diagnostic</strong></a> which will be enumerated with <a href="#def-uniqueidentifier"><strong>unique IDs</strong></a> that can be used for later reference in the <a href="#def-diagnostic"><strong>diagnostic</strong></a> output.  This is an array of <a href="#hardwareinfo"><em>HardwareInfo</em></a> messages, of which the elements are detailed below.</td>
    </tr>
    <tr>
        <td><em>softwareInfos</em></td>
        <td>JSON: array<br/>Message: list of <a href="#softwareinfo"><em>SoftwareInfo</em></a></td>
        <td>No</td>
        <td>A list of software components discovered by the <a href="#def-diagnostic"><strong>diagnostic</strong></a> which will be enumerated with <a href="#def-uniqueidentifier"><strong>unique IDs</strong></a> that can be used for later reference in <a href="#def-diagnostic"><strong>diagnostic</strong></a> output. This is an array of <a href="#softwareinfo"><em>SoftwareInfo</em></a> messages, of which the elements are detailed below.</td>
    </tr>
</table>


##### Example

```json
{
  "testRunArtifact": {
    "testRunStart": {
      "name": "mlc_test",
      "version": "1.0",
      "commandLine": "mlc/mlc --use_default_thresholds=true --data_collection_mode=true",
      "parameters": {
        "max_bandwidth": 7200.0,
        "mode": "fast_mode",
        "data_collection_mode": true,
        "min_bandwidth": 700.0,
        "use_default_thresholds": true
      },
      "dutInfo": {
        "dutInfoId": "1",
        "name": "ocp_lab_0222",
        "platformInfos": [
          {
            "info": "memory_optimized"
          }
        ],
        "softwareInfos": [
          {
            "softwareInfoId": "1",
            "computerSystem": "primary_node",
            "softwareType": "FIRMWARE",
            "name": "bmc_firmware",
            "version": "10",
            "revision": "11"
          }
        ],
        "hardwareInfos": [
          {
            "hardwareInfoId": "1",
            "computerSystem": "primary_node",
            "manager": "bmc0",
            "name": "primary node",
            "location": "MB/DIMM_A1",
            "odataId": "/redfish/v1/Systems/System.Embedded.1/Memory/DIMMSLOTA1",
            "partNumber": "P03052-091",
            "serialNumber": "HMA2022029281901",
            "manufacturer": "hynix",
            "manufacturerPartNumber": "HMA84GR7AFR4N-VK",
            "partType": "DIMM",
            "version": "1",
            "revision": "2"
          }
        ]
      }
    }
  },
  "sequenceNumber": 1,
  "timestamp": "2022-07-25T04:28:12.903943Z"
}
```


#### Error

##### Description

An [_error_](#error) is an output [**artifact**](#def-artifact) that reports a software, firmware, test or any other hardware-unrelated issues. [_Errors_](#error) can be child [**artifacts**](#def-artifact) of either [_TestRunArtifact_](#test-run-artifacts) or [_TestStepArtifact_](#test-step-artifacts) messages.

##### Attributes

<table>
    <tr>
        <td><strong>Attribute</strong></td>
        <td><strong>Type</strong></td>
        <td><strong>Required</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><em>symptom</em></td>
        <td>string</td>
        <td><strong>Yes</strong></td>
        <td>A short string of the software issued <a href="#def-verdict"><strong>verdict</strong></a>.</td>
    </tr>
    <tr>
        <td><em>message</em></td>
        <td>string</td>
        <td>No</td>
        <td>Detailed explanation why the <a href="#error"><em>Error</em></a> gives the above symptom.</td>
    </tr>
    <tr>
        <td><em>softwareInfoIds</em></td>
        <td>JSON: array<br/>Message: list of string</td>
        <td>No</td>
        <td>A list of <a href="#def-uniqueidentifier"><strong>unique IDs</strong></a> of <a href="#softwareinfo"><em>software components</em></a> or <a href="#softwareinfo"><em>firmware components</em></a> to which the <a href="#error"><em>Error</em></a> is associated. The software ID information can be found in the <a href="#dutinfo"><em>DutInfo</em></a> field of <a href="#testrunstart"><em>TestRunStart</em></a>.</td>
    </tr>
</table>


##### Example

```json
{
  "testStepArtifact": {
    "error": {
      "symptom": "bad-return-code",
      "message": "software exited abnormally.",
      "softwareInfoIds": [
        "3",
        "4"
      ]
    },
    "testStepId": "1"
  },
  "sequenceNumber": 1,
  "timestamp": "2022-07-25T03:06:11.254865Z"
}
```

##### Error artifact vs. logging messages with a severity level of "error"

This [**specification**](#def-spec) includes support for logging with a severity level of "error" which closely follows the standard conventions of most native logging facilities for various development languages and log collection systems. In addition, it also supports the concept of an error that prevents successful completion of a [**diagnostic**](#def-diagnostic).

One common scenario where this is significant is in the area of a "cleanup" or "tear-down" section of a test where an error may occur, but the logic to make a hardware health determination has been completed successfully. In this event, an [_Error_](#error) message _should not_ be emitted by the [**diagnostic**](#def-diagnostic), but a [_Log_](#log) message with the severity of **Error** is appropriate.

##### Guidance on identifying software artifacts that were generated as part of the diagnostic execution

In some edge-cases, an [_error_](#error) artifact requires being associated with a software entity that was produced as an artifact of the [**diagnostic's**](#def-diagnostic) execution itself. An example might be a particular executable which was compiled during the [**diagnostic's**](#def-diagnostic) runtime, but did not exist at the invocation of the [**diagnostic**](#def-diagnostic), so a [_SoftwareInfoId_](#softwareinfo) may not be known at the [**diagnostic's**](#def-diagnostic) execution time. In this event, the [**specification**](#def-spec) guidance is that the error should be directly attributed to the diagnostic software artifact itself, which can be included as an artifact in the [_softwareInfo_](#softwareinfo) objects in the [_DutInfo_](#dutinfo) artifact.


#### Extension

##### Description

An extension message that allows returning structured data in a manner that should be compatible with all conformant parsers.

The [_extension_](#extension) message type allows a [**test step**](#def-teststep) to record any data record as long as it can be expressed as a valid JSON object.

##### Attributes

<table>
    <tr>
        <td><strong>Attribute</strong></td>
        <td><strong>Type</strong></td>
        <td><strong>Required</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><em>name</em></td>
        <td>string</td>
        <td><strong>Yes</strong></td>
        <td>Identifier for the <a href="#extension"><em>extension</em></a> message.</td>
    </tr>
    <tr>
        <td><em>content</em></td>
        <td>JSON: object<br/>Message: <em>not specified</em></td>
        <td><strong>Yes</strong></td>
        <td>Object message for the extension, this can be <strong><em>any</em></strong> valid nested JSON object.</td>
    </tr>
</table>


##### Example

```json
{
  "testStepArtifact": {
    "extension": {
      "name": "extension example",
      "content": {
        "@type": "ExampleExtension",
        "stringField": "example string",
        "numberField": "42"
      }
    },
    "testStepId": "2321"
  },
  "sequenceNumber": 600,
  "timestamp": "2022-07-26T02:34:06.249683Z"
}
```


#### File

##### Description

Represents a file containing information that was generated or collected during the execution of a [**diagnostic**](#def-diagnostic). This [**specification**](#def-spec) does not dictate the manner in which the file is persisted or stored.  The [**specification**](#def-spec) only includes a mechanism for indicating the location of a file, and the type of information that is expected to be contained by the file.

As a file can contain any type of structured or unstructured output, the 'contentType' field is provided to allow the user to specify a valid 'MIME-TYPE' which can be used to indicate the content for any decoders such as images, sound-files, or other well known media types.

##### Content Types

The content type is an optional field that is provided to allow a consuming framework to know the format of the file that may be useful in rendering the final output to the user.

For a compliant list of defined MIME types by the W3 please refer to [**w3docs mime types**](https://www.w3docs.com/learn-html/mime-types.html).

##### Uniform Resource Identifier (URI)

The required URI parameter is used to identify how the file can be accessed remotely or locally. The URI specification is covered in the [RFC 3986 Uniform Resource Identifier (URI) Generic Syntax](https://datatracker.ietf.org/doc/html/rfc3986) document.

For local files, the URI should take the format of a File Uri as defined in [RFC 8089](https://datatracker.ietf.org/doc/html/rfc8089), however, the following external [documentation](https://tools.ietf.org/id/draft-kerwin-file-scheme-07.html) provided by the IETF (Internet Engineering Task Force) is considered more complete and concise.

When using the "file://" URI scheme, all file locations _shall_ be interpreted as being _relative_ to the host executing the [**diagnostic test**](#def-diagnostic). For instance, if a [**diagnostic**](#def-diagnostic) was executed by a control computer off of the [**DUT**](#def-dut), the File URIs would be relative to the control computer. For a [**diagnostic**](#def-diagnostic) that was executed on the [**device under test**](#def-dut), the File URI specified would refer to a local disk on that device.

The "file://" URI scheme is unusual in that it does not specify an Internet protocol or access method for shared files; as such, its utility in network protocols between hosts is limited. Examples of file server protocols that do define such access methods include [SMB/CIFS](https://tools.ietf.org/id/draft-kerwin-file-scheme-07.html#MS-SMB2) [MS-SMB2], [NFS](https://tools.ietf.org/id/draft-kerwin-file-scheme-07.html#RFC3530) [RFC3530] and [NCP](https://tools.ietf.org/id/draft-kerwin-file-scheme-07.html#NOVELL) [NOVELL].

It is not excluded that the file is retrievable from any other internet enabled protocol such as HTTP [RFC2616], FTP [RFC959] or others.

A few examples of common URI's are outlined below. Please refer to the external documentation around URI syntax for more comprehensive coverage of URI formatting and syntax:


###### Common URI examples for diagnostics

<table>
    <tr>
        <td><strong>Description</strong></td>
        <td><strong>Example</strong></td>
    </tr>
    <tr>
        <td>Local file with a full path identified</td>
        <td><em>file:///usr/bin/example</em></td>
    </tr>
    <tr>
        <td>Local file with a Microsoft Windows drive letter</td>
        <td><em>file:///c:/tmp/test.txt</em></td>
    </tr>
    <tr>
        <td>File provided via SMB network protocol</td>
        <td><em>smb://server/share</em><br/><em>smb://workgroup/server/share</em></td>
    </tr>
    <tr>
        <td>File provided via SMB network protocol  with username and passwords</td>
        <td><em>smb://username:password@server/share</em><br/><em>smb://username:password@workgroup/server/share</em></td>
    </tr>
    <tr>
        <td>File accessible via SCP</td>
        <td><em>scp://root@host:3131/root/ids/rules.tar.gz</em></td>
    </tr>
    <tr>
        <td>File accessible via NFS</td>
        <td><em>nfs://server&lt;:port&gt;/path</em></td>
    </tr>
</table>

##### Attributes

<table>
    <tr>
        <td><strong>Attribute</strong></td>
        <td><strong>Type</strong></td>
        <td><strong>Required</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><em>displayName</em></td>
        <td>string</td>
        <td><strong>Yes</strong></td>
        <td>User-friendly name of the file to be uploaded. This is intended to be displayed to the user or as output in a <a href="#def-testexecutive"><strong>test executive</strong></a>.</td>
    </tr>
    <tr>
        <td><em>uri</em></td>
        <td>string</td>
        <td><strong>Yes</strong></td>
        <td>Specifies the specific URI that can be used to access the file remotely or locally.</td>
    </tr>
    <tr>
        <td><em>description</em></td>
        <td>string</td>
        <td>No</td>
        <td>A description of the file and its contents.</td>
    </tr>
    <tr>
        <td><em>contentType</em></td>
        <td>string</td>
        <td>No</td>
        <td>A MIME-type specifying the content contained within the file.</td>
    </tr>
    <tr>
        <td><em>isSnapshot</em></td>
        <td>boolean</td>
        <td><strong>Yes</strong></td>
        <td>Indicates if the file is a temporary snapshot of the final output, or if it represents the complete file output.</td>
    </tr>
    <tr>
        <td><em>metadata</em></td>
        <td>JSON: object<br/>Message: <a href="#metadata"><em>Metadata</em></a></td>
        <td>No</td>
        <td>A JSON object containing values which contain additional information about the file object.</td>
    </tr>
</table>

##### Example

```json
{
  "testStepArtifact": {
    "file": {
      "displayName": "mem_cfg_log",
      "uri": "file:///root/mem_cfg_log",
      "description": "DIMM configuration settings.",
      "contentType": "text/plain",
      "isSnapshot": false
    },
    "testStepId": "1"
  },
  "sequenceNumber": 1,
  "timestamp": "2022-07-25T03:13:18.755979Z"
}
```


#### HardwareInfo

##### Description

Provides information on a [**hardware component**](#def-dut) in the system that is exercised or enumerated by the [**diagnostic test**](#def-diagnostic).

##### Attributes

<table>
    <tr>
        <td><strong>Attribute</strong></td>
        <td><strong>Type</strong></td>
        <td><strong>Required</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><em>hardwareInfoId</em></td>
        <td>string</td>
        <td><strong>Yes</strong></td>
        <td><a href="#def-uniqueidentifier"><strong>Unique identifier</strong></a> used to reference this hardware device in the <a href="#def-diagnostic"><strong>diagnostic</strong></a> output.</td>
    </tr>
    <tr>
        <td><em>computerSystem</em></td>
        <td>string</td>
        <td>No</td>
        <td>
            Name of the system to which this hardware info is visible. This could be a hostname, IP address, or <a href="#def-uniqueidentifier"><strong>unique identifier</strong></a> for a computer system in the <a href="#def-dut"><strong>DUT</strong></a> which is a device for running arbitrary software.<br/>
            This attribute value is intended to align with the computerSystem as defined in the Redfish specification from the DMTF.
        </td>
    </tr>
    <tr>
        <td><em>name</em></td>
        <td>string</td>
        <td><strong>Yes</strong></td>
        <td>Name of the hardware device.</td>
    </tr>
    <tr>
        <td><em>location</em></td>
        <td>string</td>
        <td>No</td>
        <td>Location for a component. The source of the location is left up to the implementation of the <a href="#def-diagnostic"><strong>diagnostic</strong></a>, but this is generally a location that a user would use to locate the device uniquely in a hardware system. This may be a silkscreen indicator (i.e. CPU0) or a unique location code.</td>
    </tr>
    <tr>
        <td><em>odataId</em></td>
        <td>string</td>
        <td>No</td>
        <td>Indicates an odata.Id identifier for a redfish managed physical component.</td>
    </tr>
    <tr>
        <td><em>partNumber</em></td>
        <td>string</td>
        <td>No</td>
        <td>Part number for the hardware device. This may be an internal or external part number.</td>
    </tr>
    <tr>
        <td><em>serialNumber</em></td>
        <td>string</td>
        <td>No</td>
        <td>Unique serial number for the hardware device.</td>
    </tr>
    <tr>
        <td><em>manager</em></td>
        <td>string</td>
        <td>No</td>
        <td>The name of the manager associated with the particular hardware component. This is typically an identifier for the baseband management controller or other OOB management device.</td>
    </tr>
    <tr>
        <td><em>manufacturer</em></td>
        <td>string</td>
        <td>No</td>
        <td>Name of the device manufacturer.</td>
    </tr>
    <tr>
        <td><em>manufacturerPartNumber</em></td>
        <td>string</td>
        <td>No</td>
        <td>The manufacturer part number of the device.</td>
    </tr>
    <tr>
        <td><em>partType</em></td>
        <td>string</td>
        <td>No</td>
        <td>Provides a category to define a particular part type (i.e. GPU, CPU, DIMM, etc).</td>
    </tr>
    <tr>
        <td><em>version</em></td>
        <td>string</td>
        <td>No</td>
        <td>The version of the hardware component.</td>
    </tr>
    <tr>
        <td><em>revision</em></td>
        <td>string</td>
        <td>No</td>
        <td>The revision of the hardware component.</td>
    </tr>
</table>

##### Example

```json
{
  "testRunArtifact": {
    "testRunStart": {
      "name": "mlc_test",
      "version": "1.0",
      "dutInfo": {
        "dutInfoId": "1",
        "name": "ocp_lab_0222",
        "hardwareInfos": [
          {
            "hardwareInfoId": "1",
            "computerSystem": "primary_node",
            "manager": "bmc0",
            "name": "primary node",
            "location": "MB/DIMM_A1",
            "odataId": "/redfish/v1/Systems/System.Embedded.1/Memory/DIMMSLOTA1",
            "partNumber": "P03052-091",
            "serialNumber": "HMA2022029281901",
            "manufacturer": "hynix",
            "manufacturerPartNumber": "HMA84GR7AFR4N-VK",
            "partType": "DIMM",
            "version": "1",
            "revision": "2"
          }
        ]
      }
    }
  },
  "sequenceNumber": 1,
  "timestamp": "2022-07-25T04:28:12.903943Z"
}
```


#### Log

##### Description

A log is an entry of free-form text documenting the output of a [**diagnostic**](#def-diagnostic) execution typically meant for the purpose of debugging or tracing program execution. This is intended for humans and not intended to be machine interpreted.

##### Attributes

<table>
    <tr>
        <td><strong>Attribute</strong></td>
        <td><strong>Type</strong></td>
        <td><strong>Required</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><em>severity</em></td>
        <td>JSON: string<br/>Message: enum <a href="#severity">Severity</a></td>
        <td><strong>Yes</strong></td>
        <td>Indicates the significance of the log message.</td>
    </tr>
    <tr>
        <td><em>message</em></td>
        <td>String</td>
        <td><strong>Yes</strong></td>
        <td>Text string intended to aid in debugging or tracing of <a href="#def-diagnostic"><strong>diagnostic</strong></a> execution.</td>
    </tr>
</table>

##### Example

```json
{
  "testStepArtifact": {
    "log": {
      "message": "file operation completed successfully.",
      "severity": "INFO"
    },
    "testStepId": "1"
  },
  "sequenceNumber": 1,
  "timestamp": "2022-07-25T03:19:01.274584Z"
}
```


#### Measurement

##### Description

A record of data collected during a [**test step**](#def-teststep) execution. [_Measurements_](#measurement) (and [_MeasurementSeries_](#measurementseriesstart)) exist to provide evidence for a particular [_diagnosis_](#diagnosis). This information is also commonly used for time-series analysis and statistical process control (e.g. C<sub>pk</sub> studies).

##### Attributes

<table>
    <tr>
        <td><strong>Attribute</strong></td>
        <td><strong>Type</strong></td>
        <td><strong>Required</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><em>name</em></td>
        <td>string</td>
        <td><strong>Yes</strong></td>
        <td>Label of the <strong>measurement</strong>.</td>
    </tr>
    <tr>
        <td><em>unit</em></td>
        <td>string</td>
        <td>No</td>
        <td>Unit of measure for the <strong>measurement</strong> (e.g. volts, seconds, transfers/sec)</td>
    </tr>
    <tr>
        <td><em>hardwareInfoId</em></td>
        <td>string</td>
        <td>No</td>
        <td><a href="#def-uniqueidentifier"><strong>Unique ID</strong></a> of the <a href="#hardwareinfo"><em>hardware device</em></a> to which the <strong>measurement</strong> information is associated.</td>
    </tr>
    <tr>
        <td><em>subcomponent</em></td>
        <td>JSON: object<br/>Message: <a href="#subcomponent"><em>Subcomponent</em></a></td>
        <td>No</td>
        <td>Additional information about a particular functional or physical element of the referenced hardware to which this <strong>measurement</strong> applies. See the <a href="#subcomponent"><em>subcomponent</em></a> message for additional details.</td>
    </tr>
    <tr>
        <td><em>validators</em></td>
        <td>JSON: array<br/>Message: list of <a href="#validator"><em>Validator</em></a></td>
        <td>No</td>
        <td>Array of <a href="#validator"><em>validator</em></a> objects to be applied to the message. See the <a href="#validator"><em>Validator</em></a> section for additional information.</td>
    </tr>
    <tr>
        <td><em>value</em></td>
        <td>String, number, boolean</td>
        <td><strong>Yes</strong></td>
        <td>Value of the <strong>measurement</strong></td>
    </tr>
    <tr>
        <td><em>metadata</em></td>
        <td>JSON: object<br/>Message: <a href="#metadata"><em>Metadata</em></a></td>
        <td>No</td>
        <td>A collection of <a href="#metadata"><em>metadata</em></a> associated with the <strong>measurement</strong>.</td>
    </tr>
</table>

##### Example

```json
{
  "testStepArtifact": {
    "measurement": {
      "name": "measured-fan-speed-100",
      "unit": "RPM",
      "hardwareInfoId": "5",
      "subcomponent": {
        "name": "FAN1",
        "location": "F0_1",
        "version": "1",
        "revision": "1",
        "type": "UNSPECIFIED"
      },
      "validators": [
        {
          "name": "80mm_fan_upper_limit",
          "type": "LESS_THAN_OR_EQUAL",
          "value": 11000.0
        },
        {
          "name": "80mm_fan_lower_limit",
          "type": "GREATER_THAN_OR_EQUAL",
          "value": 8000.0
        }
      ],
      "value": 100221.0
    },
    "testStepId": "300"
  },
  "sequenceNumber": 400,
  "timestamp": "2022-07-26T02:11:57.857853Z"
}
```


#### MeasurementSeriesElement

##### Description

A single **measurement** contained in a time-based [_MeasurementSeries_](#measurementseriesstart) message. A [_measurement series element_](#measurementserieselement) can optionally include [_validators_](#validator) to specify the values which are considered to be [**conformant**](#def-nonconformance) for the [_measurement element_](#measurementserieselement). See the [_Validator_](#validator) section for additional information on how they apply to **measurements** which are members of a [_measurement series_](#measurementseriesstart).

##### Attributes

<table>
    <tr>
        <td><strong>Attribute</strong></td>
        <td><strong>Type</strong></td>
        <td><strong>Required</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><em>index</em></td>
        <td>number (integer)</td>
        <td><strong>Yes</strong></td>
        <td>Specifies a zero based offset indicating the order in which the <strong>measurement</strong> is located in the series.</td>
    </tr>
    <tr>
        <td><em>measurementSeriesId</em></td>
        <td>string</td>
        <td><strong>Yes</strong></td>
        <td>Identifies the <a href="#measurementseriesstart"><em>MeasurementSeries</em></a> to which the <a href="#measurementserieselement"><em>MeasurementSeriesElement</em></a> belongs via its <a href="#def-uniqueidentifier"><strong>unique</strong></a> measurement series id.</td>
    </tr>
    <tr>
        <td><em>value</em></td>
        <td>string, number, boolean</td>
        <td><strong>Yes</strong></td>
        <td>Value of the <strong>measurement</strong> in the <strong>measurement series</strong>.</td>
    </tr>
    <tr>
        <td><em>timestamp</em></td>
        <td>string</td>
        <td><strong>Yes</strong></td>
        <td>An ISO formatted date time string indicating when the <strong>measurement</strong> was recorded from the perspective of the RTC of the <a href="#def-dut"><strong>device under test</strong></a>.</td>
    </tr>
    <tr>
        <td><em>metadata</em></td>
        <td>JSON: object<br/>Message: <a href="#metadata"><em>Metadata</em></a></td>
        <td>No</td>
        <td>Optional <a href="#metadata"><em>metadata</em></a> concerning the <a href="#measurementserieselement"><em>measurement series element</em></a>.</td>
    </tr>
</table>

##### Example

```json
{
  "testStepArtifact": {
    "measurementSeriesElement": {
      "index": 144,
      "measurementSeriesId": "12",
      "value": 100219.0,
      "timestamp": "2022-07-26T02:45:00.102414Z"
    },
    "testStepId": "45398"
  },
  "sequenceNumber": 700,
  "timestamp": "2022-07-26T02:45:00.097896Z"
}
```


#### MeasurementSeriesEnd

##### Description

Indicates the completion of a **measurement series** in a [**test step**](#def-teststep).

##### Attributes

<table>
    <tr>
        <td><strong>Attribute</strong></td>
        <td><strong>Type</strong></td>
        <td><strong>Required</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><em>measurementSeriesId</em></td>
        <td>string</td>
        <td><strong>Yes</strong></td>
        <td>A <a href="#def-testrun"><strong>test run</strong></a> scoped <a href="#def-uniqueidentifier"><strong>unique id</strong></a> which is used to identify data associated to the <a href="#measurementseriesstart"><em>measurement series</em></a>.</td>
    </tr>
    <tr>
        <td><em>totalCount</em></td>
        <td>number</td>
        <td><strong>Yes</strong></td>
        <td>The number of <a href="#measurementserieselement"><em>measurement elements</em></a> in the completed <a href="#measurementseriesstart"><em>measurement series</em></a>.</td>
    </tr>
</table>

##### Example

```json
{
  "testStepArtifact": {
    "measurementSeriesEnd": {
      "measurementSeriesId": "0",
      "totalCount": 51
    },
    "testStepId": "231"
  },
  "sequenceNumber": 300,
  "timestamp": "2022-07-26T02:16:10.393209Z"
}
```


#### MeasurementSeriesStart

##### Description

Indicates the start of a new **measurement series** in a [**test step**](#def-teststep). A **measurement series** consists of time based data that is emitted from a [**diagnostic**](#def-diagnostic) with no expectation of the transmission sequence being ordered. Each element of the **measurement series** is numbered so that the order can be established when the test execution is complete. No additional [_measurement elements_](#measurementserieselement) for the series should be transmitted after the [_measurementSeriesEnd_](#measurementseriesend) message.

##### Attributes

<table>
    <tr>
        <td><strong>Attribute</strong></td>
        <td><strong>Type</strong></td>
        <td><strong>Required</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><em>measurementSeriesId</em></td>
        <td>string</td>
        <td><strong>Yes</strong></td>
        <td>A <a href="#def-testrun"><strong>test run</strong></a> scoped <a href="#def-uniqueidentifier"><strong>unique id</strong></a> which is used to identify data associated to the <strong>measurement series</strong>.</td>
    </tr>
    <tr>
        <td><em>name</em></td>
        <td>string</td>
        <td><strong>Yes</strong></td>
        <td>Name of the <strong>measurement</strong>.</td>
    </tr>
    <tr>
        <td><em>unit</em></td>
        <td>string</td>
        <td>No</td>
        <td>Unit of measure of the <strong>series element</strong>.</td>
    </tr>
    <tr>
        <td><em>hardwareInfoId</em></td>
        <td>string</td>
        <td>No</td>
        <td><a href="#def-uniqueidentifier"><strong>Unique ID</strong></a> of the <a href="#def-dut"><em>hardware device</em></a> to which the <strong>measurement</strong> information is associated.</td>
    </tr>
    <tr>
        <td><em>subcomponent</em></td>
        <td>JSON: object<br/>Message: <a href="#subcomponent"><em>Subcomponent</em></a></td>
        <td>No</td>
        <td>Additional information about a particular functional or physical element of the referenced hardware to which this <strong>measurement series</strong> applies.</td>
    </tr>
    <tr>
        <td><em>validators</em></td>
        <td>JSON: array<br/>Message: list of <a href="#validator"><em>Validator</em></a></td>
        <td>No</td>
        <td>Array of <a href="#validator"><em>validators</em></a> that are applied against ALL elements of a <strong>measurement series</strong>.</td>
    </tr>
    <tr>
        <td><em>metadata</em></td>
        <td>JSON: object<br/>Message: <a href="#metadata"><em>Metadata</em></a></td>
        <td>No</td>
        <td>Optional <a href="#metadata"><em>metadata</em></a> concerning the <strong>measurement series</strong>.</td>
    </tr>
</table>

##### Example

```json
{
  "testStepArtifact": {
    "measurementSeriesStart": {
      "measurementSeriesId": "0",
      "name": "measured-fan-speed-100",
      "unit": "RPM",
      "hardwareInfoId": "5",
      "subcomponent": {
        "name": "FAN1",
        "location": "F0_1",
        "version": "1",
        "revision": "1",
        "type": "UNSPECIFIED"
      },
      "validators": [
        {
          "name": "80mm_fan_upper_limit",
          "type": "LESS_THAN_OR_EQUAL",
          "value": 11000.0
        },
        {
          "name": "80mm_fan_lower_limit",
          "type": "GREATER_THAN_OR_EQUAL",
          "value": 8000.0
        }
      ]
    },
    "testStepId": "33"
  },
  "sequenceNumber": 300,
  "timestamp": "2022-07-26T02:04:02.083679Z"
}
```


#### Metadata

##### Description

Additional data concerning a [_measurement_](#measurement), [_measurement series_](#measurementserieselement), [_validator_](#validator), [_file_](#file), or [_DUTInfo_](#dutinfo). [_Metadata_](#metadata) essentially provides a well-known location for attaching additional data via a JSON object to any supported message.

##### Attributes

<table>
    <tr>
        <td><strong>Attribute</strong></td>
        <td><strong>Type</strong></td>
        <td><strong>Required</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><strong><em>&lt;various&gt;</em></strong></td>
        <td>JSON: object<br/>Message: various</td>
        <td>No</td>
        <td>Any valid JSON object</td>
    </tr>
</table>

##### Example

```json
{
  "testStepArtifact": {
    "measurement": {
      "name": "measured-fan-speed-100",
      "unit": "RPM",
      "hardwareInfoId": "5",
      "subcomponent": {
        "name": "FAN1",
        "location": "F0_1",
        "version": "1",
        "revision": "1",
        "type": "UNSPECIFIED"
      },
      "validators": [
        {
          "name": "80mm_fan_upper_limit",
          "type": "LESS_THAN_OR_EQUAL",
          "value": 11000.0
        },
        {
          "name": "80mm_fan_lower_limit",
          "type": "GREATER_THAN_OR_EQUAL",
          "value": 8000.0
        }
      ],
      "value": 100221.0,
      "metadata": {
        "@type": "ExampleMetadata",
        "exampleField1": "example string",
        "exampleField2": "42"
      }
    },
    "testStepId": "4123"
  },
  "sequenceNumber": 800,
  "timestamp": "2022-07-26T03:06:56.549372Z"
}
```


#### Parameters

##### Description

[_Parameters_](#parameters) represent the configuration values that were passed to the [**diagnostic**](#def-diagnostic) run at the time of invocation. They are command line or file based settings that control the execution and behavior of a [**diagnostic test**](#def-diagnostic).

The optional parameter definition is left undefined in this [**specification**](#def-spec) with the caveat being that the message object if present shall be a valid JSON object. The usage of a non-hierarchical parameter space is encouraged as it will map well with command line parameters, but is not an explicit requirement.

##### Example

```json
{
  "testRunArtifact": {
    "testRunStart": {
      "name": "mlc_test",
      "version": "1.0",
      "commandLine": "mlc/mlc --use_default_thresholds=true --data_collection_mode=true",
      "parameters": {
        "max_bandwidth": 7200.0,
        "mode": "fast_mode",
        "data_collection_mode": true,
        "min_bandwidth": 700.0,
        "use_default_thresholds": true
      },
      "dutInfo": {
        "dutInfoId": "1",
        "name": "ocp_lab_0222",
        "platformInfos": [
          {
            "info": "memory_optimized"
          }
        ],
        "softwareInfos": [
          {
            "softwareInfoId": "1",
            "computerSystem": "primary_node",
            "softwareType": "FIRMWARE",
            "name": "bmc_firmware",
            "version": "10",
            "revision": "11"
          }
        ],
        "hardwareInfos": [
          {
            "hardwareInfoId": "1",
            "computerSystem": "primary_node",
            "manager": "bmc0",
            "name": "primary node",
            "location": "MB/DIMM_A1",
            "odataId": "/redfish/v1/Systems/System.Embedded.1/Memory/DIMMSLOTA1",
            "partNumber": "P03052-091",
            "serialNumber": "HMA2022029281901",
            "manufacturer": "hynix",
            "manufacturerPartNumber": "HMA84GR7AFR4N-VK",
            "partType": "DIMM",
            "version": "1",
            "revision": "2"
          }
        ]
      }
    }
  },
  "sequenceNumber": 1,
  "timestamp": "2022-07-26T02:04:02.083679Z"
}
```


#### PlatformInfo

##### Description

Describes properties or attributes of the [**device under test**](#def-dut). This can be used as an indicator of product genealogy such as product family, product type, etc.

This object is intended primarily for future expansion for a genealogy structure for the [**device under test**](#def-dut).

##### Attributes

<table>
    <tr>
        <td><strong>Attribute</strong></td>
        <td><strong>Type</strong></td>
        <td><strong>Required</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><em>info</em></td>
        <td>string</td>
        <td><strong>Yes</strong></td>
        <td>A string containing information about the platform of the <a href="#def-dut"><strong>device under test</strong></a>.</td>
    </tr>
</table>

##### Example

```json
{
  "testRunArtifact": {
    "testRunStart": {
      "name": "example",
      "version": "1.0",
      "commandLine": "example",
      "dutInfo": {
        "dutInfoId": "1",
        "name": "ocp_lab_0222",
        "platformInfos": [
          {
            "info": "memory_optimized"
          }
        ],
      }
    }
  },
  "sequenceNumber": 1,
  "timestamp": "2022-07-25T04:28:12.903943Z"
}
```


#### SoftwareInfo

##### Description

Provides information about software that was discovered or exercised during the execution of a [**diagnostic test**](#def-dut).

##### Attributes

<table>
    <tr>
        <td><strong>Attribute</strong></td>
        <td><strong>Type</strong></td>
        <td><strong>Required</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><em>softwareInfoId</em></td>
        <td>string</td>
        <td><strong>Yes</strong></td>
        <td><a href="#def-uniqueidentifier"><strong>Unique identifier</strong></a> used to reference this software component in the <a href="#def-diagnostic"><strong>diagnostic</strong></a> output.</td>
    </tr>
    <tr>
        <td><em>computerSystem</em></td>
        <td>string</td>
        <td>No</td>
        <td>Name of the computer system on which this software is executing.</td>
    </tr>
    <tr>
        <td><em>name</em></td>
        <td>string</td>
        <td><strong>Yes</strong></td>
        <td>Name of the software component.</td>
    </tr>
    <tr>
        <td><em>version</em></td>
        <td>string</td>
        <td>No</td>
        <td>Version of the software component.</td>
    </tr>
    <tr>
        <td><em>revision</em></td>
        <td>string</td>
        <td>No</td>
        <td>Revision of the software component.</td>
    </tr>
    <tr>
        <td><em>softwareType</em></td>
        <td>JSON: string<br/>Message: enum <a href="#softwaretype">SoftwareType</a></td>
        <td>No</td>
        <td>Optional field provided to categorize software into specific types that may be useful for post-analysis. This is expressed as a <a href="#softwaretype">SoftwareType</a> enumeration.</td>
    </tr>
</table>

##### Example

```json
{
  "testRunArtifact": {
    "testRunStart": {
      "name": "example",
      "version": "1.0",
      "commandLine": "example",
      "dutInfo": {
        "dutInfoId": "1",
        "name": "ocp_lab_0222",
        "softwareInfos": [
          {
            "softwareInfoId": "1",
            "computerSystem": "primary_node",
            "softwareType": "FIRMWARE",
            "name": "bmc_firmware",
            "version": "10",
            "revision": "11"
          }
        ],
      }
    }
  },
  "sequenceNumber": 1,
  "timestamp": "2022-07-25T04:28:12.903943Z"
}
```


#### Subcomponent

##### Description

Provides information about a sub-system or functional block of a [_hardware device_](#hardwareinfo). This can be used to complement and provide additional coverage/isolation information for [_Measurement_](#measurement) and [_Diagnosis_](#diagnosis) messages to provide additional optional detail about the device to which the data applies.

Typically a [_subcomponent_](#subcomponent) may refer to a device or subsection of a device under the FRU-able component for a [**system under test**](#def-dut).

The [_subcomponent_](#subcomponent) is very loosely defined and may refer to a physical device, logical function, or collection of related devices that are related to the corresponding [_hardware device_](#hardwareinfo) which is referenced by the [_hardwareInfoId_](#hardwareinfo) on the parent message.

##### Attributes

<table>
    <tr>
        <td><strong>Attribute</strong></td>
        <td><strong>Type</strong></td>
        <td><strong>Required</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><em>type</em></td>
        <td>JSON: string<br/>Message: enum <a href="#subcomponenttype">SubcomponentType</a></td>
        <td>No</td>
        <td>An optional enumeration indicating the subcomponent type.</td>
    </tr>
    <tr>
        <td><em>name</em></td>
        <td>string</td>
        <td><strong>Yes</strong></td>
        <td>Name of the functional block, signal, bus or element of the <a href="#hardwareinfo"><em>hardwareId</em></a> to which the supporting data is applicable.</td>
    </tr>
    <tr>
        <td><em>location</em></td>
        <td>string</td>
        <td>No</td>
        <td>A location identifier for the subcomponent.</td>
    </tr>
    <tr>
        <td><em>version</em></td>
        <td>string</td>
        <td>No</td>
        <td>Version of the subcomponent.</td>
    </tr>
    <tr>
        <td><em>revision</em></td>
        <td>string</td>
        <td>No</td>
        <td>Revision of the subcomponent.</td>
    </tr>
</table>

##### Example

```json
{
  "testStepArtifact": {
    "measurement": {
      "name": "measured-fan-speed-100",
      "unit": "RPM",
      "hardwareInfoId": "5",
      "subcomponent": {
        "name": "FAN1",
        "location": "F0_1",
        "version": "1",
        "revision": "1",
        "type": "UNSPECIFIED"
      },
      "value": 100221.0
    },
    "testStepId": "300"
  },
  "sequenceNumber": 400,
  "timestamp": "2022-07-26T02:16:10.393209Z"
}
```


#### TestRunEnd

##### Description

An output [**artifact**](#def-artifact) indicating the overall completion of a [**test run**](#def-testrun).

###### Critical Message Handling Errata

In the event that an error or condition that occurs that would prevent this message from being emitted in its entirety, it's expected that the client consuming this diagnostic output will have a reasonable client side timeout which would cause the [**diagnostic**](#def-diagnostic) execution to be determined an 'Error' if the [_TestRunStart_](#testrunstart) and [_TestRunEnd_](#testrunend) events are not both received in a parseable form prior to the elapsed maximum execution time for the [**diagnostic**](#def-diagnostic).

##### Attributes

<table>
    <tr>
        <td><strong>Attribute</strong></td>
        <td><strong>Type</strong></td>
        <td><strong>Required</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><em>status</em></td>
        <td>JSON: string<br/>Message: enum <a href="#teststatus">TestStatus</a></td>
        <td><strong>Yes</strong></td>
        <td><a href="#teststatus">TestStatus</a> enumeration which specifies the execution status of the <a href="#def-testrun"><strong>test run</strong></a>.</td>
    </tr>
    <tr>
        <td><em>result</em></td>
        <td>JSON: string<br/>Message: enum <a href="#testresult">TestResult</a></td>
        <td><strong>Yes</strong></td>
        <td><a href="#testresult">TestResult</a> enumeration which indicates the overall result of the <a href="#def-testrun"><strong>test run</strong></a>.</td>
    </tr>
</table>

##### Example

```json
{
  "testRunArtifact": {
    "testRunEnd": {
      "status": "COMPLETE",
      "result": "PASS"
    }
  },
  "sequenceNumber": 100,
  "timestamp": "2022-07-25T04:49:25.262947Z"
}
```


#### TestRunStart

##### Description

The [_TestRunStart_](#testrunstart) [**artifact**](#def-artifact) establishes the information pertaining to the scope and execution of the overall [**test run**](#def-testrun) which may contain zero or more [**test steps**](#def-teststep).

##### Attributes

<table>
    <tr>
        <td><strong>Attribute</strong></td>
        <td><strong>Type</strong></td>
        <td><strong>Required</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><em>name</em></td>
        <td>string</td>
        <td><strong>Yes</strong></td>
        <td>Name of the <a href="#def-diagnostic"><strong>diagnostic</strong></a> being executed.</td>
    </tr>
    <tr>
        <td><em>version</em></td>
        <td>string</td>
        <td><strong>Yes</strong></td>
        <td>Version of the <a href="#def-diagnostic"><strong>diagnostic</strong></a> being executed.</td>
    </tr>
    <tr>
        <td><em>commandLine</em></td>
        <td>string</td>
        <td><strong>Yes</strong></td>
        <td>
            Full command line with arguments that was used to invoke the <a href="#def-diagnostic"><strong>diagnostic</strong></a>.<br/>
            Note that this may be different from the <a href="#parameters"><em>parameters</em></a> since they may be determined from a second level of interpolation based on implementation.<br/>
            For example, a command line argument may include a file name, and the <a href="#parameters"><em>parameters</em></a> message may include information captured from that particular file.
        </td>
    </tr>
    <tr>
        <td><em>parameters</em></td>
        <td>JSON: object<br/>Message: <em>various</em></td>
        <td><strong>Yes</strong></td>
        <td>
            Object containing information about the state or configuration of the diagnostic as it executed.<br/>
            Typically parameters can be input from the command line, configuration files, or may be default values of the <a href="#def-diagnostic">M<strong>diagnostic</strong></a>.<br/>
            These parameters are specific to the <a href="#def-diagnostic"><strong>diagnostic test</strong></a>, and are included to allow capture of the configuration of the <a href="#def-diagnostic"><strong>diagnostic</strong></a> in a data store.
        </td>
    </tr>
    <tr>
        <td><em>metadata</em></td>
        <td>JSON: object<br/>Message: <a href="#metadata"><em>Metadata</em></a></td>
        <td>No</td>
        <td>Optional <a href="#metadata"><em>metadata</em></a> concerning the <a href="#def-testrun"><strong>test run</strong></a>.</td>
    </tr>
    <tr>
        <td><em>dutInfo</em></td>
        <td>JSON: object<br/>Message: <a href="#dutinfo"><em>DutInfo</em></a></td>
        <td><strong>Yes</strong></td>
        <td>Collection of structured information obtained by the <a href="#def-diagnostic"><strong>diagnostic</strong></a> concerning the <a href="#def-dut"><strong>system under test</strong></a>.</td>
    </tr>
</table>

##### Example Artifact

```json
{
  "testRunArtifact": {
    "testRunStart": {
      "name": "mlc_test",
      "version": "1.0",
      "commandLine": "mlc/mlc --use_default_thresholds=true --data_collection_mode=true",
      "parameters": {
        "max_bandwidth": 7200.0,
        "mode": "fast_mode",
        "data_collection_mode": true,
        "min_bandwidth": 700.0,
        "use_default_thresholds": true
      },
      "dutInfo": {
        "dutInfoId": "1",
        "name": "ocp_lab_0222",
        "platformInfos": [
          {
            "info": "memory_optimized"
          }
        ],
        "softwareInfos": [
          {
            "softwareInfoId": "1",
            "computerSystem": "primary_node",
            "softwareType": "FIRMWARE",
            "name": "bmc_firmware",
            "version": "10",
            "revision": "11"
          }
        ],
        "hardwareInfos": [
          {
            "hardwareInfoId": "1",
            "computerSystem": "primary_node",
            "manager": "bmc0",
            "name": "primary node",
            "location": "MB/DIMM_A1",
            "odataId": "/redfish/v1/Systems/System.Embedded.1/Memory/DIMMSLOTA1",
            "partNumber": "P03052-091",
            "serialNumber": "HMA2022029281901",
            "manufacturer": "hynix",
            "manufacturerPartNumber": "HMA84GR7AFR4N-VK",
            "partType": "DIMM",
            "version": "1",
            "revision": "2"
          }
        ]
      }
    }
  },
  "sequenceNumber": 1,
  "timestamp": "2022-07-25T04:49:25.262947Z"
}
```


#### TestStepEnd

##### Description

[**Artifact**](#def-artifact) indicating the completion of a [**test step**](#def-teststep).

##### Attributes

<table>
    <tr>
        <td><strong>Attribute</strong></td>
        <td><strong>Type</strong></td>
        <td><strong>Required</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><em>status</em></td>
        <td>JSON: string<br/>Message: enum <a href="#teststatus">TestStatus</a></td>
        <td><strong>Yes</strong></td>
        <td><a href="#teststatus">TestStatus</em></a> enumeration expressing the overall status of the <a href="#def-teststep"><strong>test step</strong></a>.</td>
    </tr>
</table>

##### Example

```json
{
  "testStepArtifact": {
    "testStepEnd": {
      "status": "COMPLETE"
    },
    "testStepId": "22"
  },
  "sequenceNumber": 200,
  "timestamp": "2022-07-25T04:54:41.292679Z"
}
```


#### TestStepStart

##### Description

The [_TestStepStart_](#teststepstart) [**artifact**](#def-artifact) contains all the information relating to the beginning of a new phase, task, step, or area of focus for a [**diagnostic**](#def-diagnostic).

##### Attributes

<table>
    <tr>
        <td><strong>Attribute</strong></td>
        <td><strong>Type</strong></td>
        <td><strong>Required</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><em>name</em></td>
        <td>string</td>
        <td><strong>Yes</strong></td>
        <td>Name of the <a href="#def-teststep"><strong>test step</strong></a></td>
    </tr>
</table>

##### Example

```json
{
  "testStepArtifact": {
    "testStepStart": {
      "name": "intranode-bandwidth-check"
    },
    "testStepId": "22"
  },
  "sequenceNumber": 200,
  "timestamp": "2022-07-25T04:56:42.249367Z"
}
```


#### Validator

##### Description

A [_validator_](#validator) is the description of a comparison or evaluation of a [_measurement_](#measurement) or [_measurement series element_](#measurementserieselement) against a criteria to determine if the [_measurement_](#measurement) is [**conformant**](#def-nonconformance) or within expected tolerances. [_Validators_](#validator) are polymorphic and can support multiple value types, although the data types on both sides of the evaluation shall have the same type, or the behavior is not supported.

##### Attributes

<table>
    <tr>
        <td><strong>Attribute</strong></td>
        <td><strong>Type</strong></td>
        <td><strong>Required</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><em>name</em></td>
        <td>string</td>
        <td>No</td>
        <td>The name of the comparison being performed (i.e. checksum verification)</td>
    </tr>
    <tr>
        <td><em>type</em></td>
        <td>JSON: string<br/>Message: enum <a href="#validatortype">ValidatorType</a></td>
        <td><strong>Yes</strong></td>
        <td>A supported validator/comparison type.</td>
    </tr>
    <tr>
        <td><em>value</em></td>
        <td>JSON: string, number, boolean</td>
        <td><strong>Yes</strong></td>
        <td>The value to use on the right side of the arithmetic comparison.</td>
    </tr>
    <tr>
        <td><em>metadata</em></td>
        <td>JSON: object<br/>Message: <a href="#metadata"><em>Metadata</em></a></td>
        <td>No</td>
        <td>Optional <a href="#metadata"><em>metadata</em></a> about the <a href="#validator"><em>validator</em></a>.</td>
    </tr>
</table>

---
# TODO: duplicated with section on enums (this one is left uncorrected for links)
---

The following table indicates the value types that are supported for each _validator_.

<table>
    <tr>
        <td><strong>Value</strong></td>
        <td><strong>Supported Measurement Value Types</strong></td>
        <td><strong>Supported Validator Value Types</strong></td>
        <td><strong>Description</strong></td>
    </tr>
    <tr>
        <td><strong>EQUAL</strong></td>
        <td>string, number, boolean</td>
        <td>string, number, boolean</td>
        <td>Validates that a <em>measurement</em> and a comparison value are equal.</td>
    </tr>
    <tr>
        <td><strong>NOT_EQUAL</strong></td>
        <td>string, number, boolean</td>
        <td>string, number, boolean</td>
        <td>Validates that a <em>measurement</em> and a comparison value are not equal.</td>
    </tr>
    <tr>
        <td><strong>LESS_THAN</strong></td>
        <td>number</td>
        <td>number</td>
        <td>Validates that a <em>measurement</em> is less than a comparison value.</td>
    </tr>
    <tr>
        <td><strong>LESS_THAN_OR_EQUAL</strong></td>
        <td>number</td>
        <td>number</td>
        <td>Validates that a <em>measurement</em> is less than or equal to a comparison value.</td>
    </tr>
    <tr>
        <td><strong>GREATER_THAN</strong></td>
        <td>number</td>
        <td>number</td>
        <td>Validates that a <em>measurement</em> is greater than a comparison value.</td>
    </tr>
    <tr>
        <td><strong>GREATER_THAN_OR_EQUAL</strong></td>
        <td>number</td>
        <td>number</td>
        <td>Validates that <em>measurement</em> is greater than or equal to a comparison value.</td>
    </tr>
    <tr>
        <td><strong>REGEX_MATCH</strong></td>
        <td>string</td>
        <td>string, array[string]</td>
        <td>Validates that a <em>measurement</em> is found to match one or more regex strings contained by the <em>validator</em>.</td>
    </tr>
    <tr>
        <td><strong>REGEX_NO_MATCH</strong></td>
        <td>string</td>
        <td>string, array[string]</td>
        <td>Validates that a <em>measurement</em> does not match any regex strings contained by the <em>validator</em>.</td>
    </tr>
    <tr>
        <td><strong>IN_SET</strong></td>
        <td>string,number</td>
        <td>array[string], array[number]</td>
        <td>Validates that a <em>measurement</em> exists within a set of allowable values contained by the <em>validator</em>.</td>
    </tr>
    <tr>
        <td><strong>NOT_IN_SET</strong></td>
        <td>string, number</td>
        <td>array[string], array[number].</td>
        <td>Validates that a <em>measurement</em> does not exist within a set of allowables values contained by the <em>validator</em>.</td>
    </tr>
</table>

##### Example

```json
{
  "testStepArtifact": {
    "measurement": {
      "name": "measured-fan-speed-100",
      "unit": "RPM",
      "hardwareInfoId": "5",
      "subcomponent": {
        "name": "FAN1",
        "location": "F0_1",
        "version": "1",
        "revision": "1",
        "type": "UNSPECIFIED"
      },
      "validators": [
        {
          "name": "80mm_fan_upper_limit",
          "type": "LESS_THAN_OR_EQUAL",
          "value": 11000.0
        },
        {
          "name": "80mm_fan_lower_limit",
          "type": "GREATER_THAN_OR_EQUAL",
          "value": 8000.0
        }
      ],
      "value": 100221.0
    },
    "testStepId": "300"
  },
  "sequenceNumber": 400,
  "timestamp": "2022-07-26T02:16:10.393209Z"
}
```


## <a name="contributors">Specification Contributors</a>

<table>
    <tr>
        <td><strong>Contributor</strong></td>
        <td><strong>Organization</strong></td>
    </tr>
    <tr>
        <td><strong>Adrian Enache</strong></td>
        <td>Meta</td>
    </tr>
    <tr>
        <td><strong>Arun Darlie Koshy</strong></td>
        <td>HPE</td>
    </tr>
    <tr>
        <td><strong>Dan Frame</strong></td>
        <td>Google</td>
    </tr>
    <tr>
        <td><strong>Daniel Alvarez Wise</strong></td>
        <td>Meta</td>
    </tr>
    <tr>
        <td><strong>Dylan Hawkes</strong></td>
        <td>Google</td>
    </tr>
    <tr>
        <td><strong>Giovanni Colapinto</strong></td>
        <td>Meta</td>
    </tr>
    <tr>
        <td><strong>Karen Murphy</strong></td>
        <td>Meta</td>
    </tr>
    <tr>
        <td><strong>Brennen Direnzo</strong></td>
        <td>Keysight</td>
    </tr>
    <tr>
        <td><strong>Marcos Pearson</strong></td>
        <td>Google</td>
    </tr>
    <tr>
        <td><strong>Paul Ng</strong></td>
        <td>Meta</td>
    </tr>
    <tr>
        <td><strong>Thomas Minor</strong></td>
        <td>Google</td>
    </tr>
    <tr>
        <td><strong>Vincent Matossian</strong></td>
        <td>Meta</td>
    </tr>
    <tr>
        <td><strong>Yuanlin Wen</strong></td>
        <td>Google</td>
    </tr>
</table>
