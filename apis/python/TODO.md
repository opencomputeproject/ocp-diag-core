### Items left to do for initial stable version

- alternate api? disallows making multiple runs
```
run = ocptv.TestRun(name="test", version="1.0", dut=ocptv.Dut(id="dut0"))
with run:
    with run.add_step("step0") as step:
        step.add_log(...)
```

- runtime checks?
  Eg. throw error on ending a step multiple times?
  - `__post_init__` can be used for validation of dataclasses at runtime?
  - have subcomponent without hardware info specified
  - ref hardware info unrelated to dut used in run

- docstrings for everything after spec markdown PR is merged

- add a lot of tests covering all aspects of the schema objects; git actions
  Also a linter on push/PR; reach 100% coverage

- add pypi publish git action (after move to ocp-diag-python repo)

- formalize config block api with options for lib:
  - timezone formatting override (UTC/local or some other logic)
  - output channel (currently uses `configOutput` method call)

- considerations for multithreading/multiprocess