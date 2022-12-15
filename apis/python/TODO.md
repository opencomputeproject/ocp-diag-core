### Items left to do for initial version

- dataclass structs for spec objects
  Manually write classes for now (there may be a codegen step later on?).
  `__post_init__` can be used for validation hook.
  Add protocol type for json serialization, useful in ArtifactEmitter

- dut info builder
  The run start needs a DutInfo object, one may be constructed from components.

- failsafes?
  Eg. throw error on ending a step multiple times?

- add a lot of tests covering all aspects of the schema objects; git actions

- add pypi publish git action