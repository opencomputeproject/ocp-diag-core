### Items left to do for initial version

- `__post_init__` can be used for validation of dataclasses at runtime?

- dut info builder
  The run start needs a DutInfo object, one may be constructed from components.

- failsafes?
  Eg. throw error on ending a step multiple times?

- add a lot of tests covering all aspects of the schema objects; git actions
  Also a linter on push/PR

- add pypi publish git action