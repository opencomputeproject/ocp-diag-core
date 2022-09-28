# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

"""This module implements python wrappers around the FakeParams API."""

from ocpdiag.core.params.python import _fake_params

ParamsCleanup = _fake_params.ParamsCleanup
FakeParams = _fake_params.FakeParams
