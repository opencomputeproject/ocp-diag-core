# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

from google.protobuf import message


class ParamsCleanup:

  def __enter__(self):
    ...

  def __exit__(self, *args):
    ...


def FakeParams(params: message.Message) -> ParamsCleanup:
  ...
