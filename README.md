# Open Compute Project Hardware Diagnostics Project

## Introduction

The OCP hardware diagnostics project is a collaboration between data center hyperscalers participating in the OCP to provide a diagnostic framework.  This framework aims to provide a portable solution for execution of hardware diagnostics, and a rich output model which can be cleanly integrated with various test executives, manufacturing execution systems, or lab data collection systems.

The project was motivated by the common desire of being able to execute the same diagnostics across all phases of an NPI hardware projects life cycle including.

* Hardware design and validation phases
  * Hardware bringup
  * System integration testing
  * Reliability testing
  * Third party lab validation
* Mass producton and deployment
  * Manufacturing
  * Data center operations
  * RMA and reverse logistics

### Just getting started and want to know more?

Please checkout our [OCP Keynote presentations](./docs/ocp_presentations.md) for more information about the Test and Validation track's common diagnostic framework initiative.

## What problems does it address?

* Acceleration/re-use of diagnostic development and integration efforts at all stages of the product life-cycle.
* Diagnostic portability across multiple products, environments, and use-cases.
* Reproduction of test and validation issues across multiple hardware and software partners.
* Simple sharing of component vendor tests to accelerate RMA and root-cause analysis.

## What does this project provide?

* Proven thoroughly documented data model for diagnostic output
* API’s to easily produce that output.
* Streaming results for long running tests
* Simple but capable parameter management
* An optional device communication library
* An optional hardware interface layer

## What are the different repos making up this effort?

This repo contains the diagnostic core libraries that are compatible with C++ and Python. These libraries provide the framework to reliably generated diagnostics that are compatible with clients designed to consume the OCP diagnostic output format.  It also contains the detailed specification covering the schema of the diagnostic output and guidance on how it should be leveraged.  This schema has been designed from the ground up to provide the necessary meta data and structure to be used effectively in heuristic and ML based systems for hardware troubleshooting and repair activities.  It has been proven to be a key enabler of diagnosability and repair at scale for large fleets of data center hardware when combined with downstream repair automation systems.

In this repo you will find:

* *apis* - These are implementations of the OCP diagnostic framework in c++ with python bindings, as well as a pure python implementation.
* *ci_scripts* - repo automation for pre-submit testing for this repository.
* *json_spec* - A complete overview of the OCP output specification in markdown for easy reference, as well as a json schema document.
* *validators* - A json output validator for the OCP output specification.

### Related repositories

In addition, you will also find the following repositories which provide hardware diagnostics that are compliant with the OCP diagnostic specification which have been implemented in this library.

These include:
* *ocp-diag-sat* - An OCP compliant version of Google's popular Stressful Application Test for server and storage burnin testing.
* *ocp-memtester* - A comprehensive DIMM and embedded memory pattern test application.
* *ocp-diag-pcicrawler* - A portable PCIe bus health checker with support for the advanced error reporting standard.

Overtime additional *non-differentiating* diagnostics will be added for storage and hardware accelerators.

## Contact information

**Team:** ocp-test-validation@OCP-All.groups.io

**Code reviews:** ocpdiag-core-team+reviews@google.com
