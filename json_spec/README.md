# OCP Hardware Validation Spec Draft 0.1

## Specs
`results_spec.json`: this is the main output schema file, with the objects that should implement by any user of the specs. There are restricted to what is strictly need to identify a validation session, trying to keep them as simple as possible and with more optional parameters.

`extensions`: this directory contains a lot of other objects to include in the validation output to make it more detailed and prone to statistics

`extensions/artifact_extensions.json`: <Google provide description please>

`extensions/diagnosis.json`: <Google provide description please>

`extensions/files.json`: this schema provides the objects to map all the files produced from the validation session and uploaded to the orchestrator. The upload could be to a remote filesystem or directly using the json document

`extensions/measurements.json`: <Google provide description please>

`extensions/step_details.json`: sometimes more details are needed for each step and this schema provides some additional fields to use
