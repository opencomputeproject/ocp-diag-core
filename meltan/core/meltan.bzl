# Copyright 2021 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Bazel rules for building and packaging Meltan tests"""

load("@rules_pkg//:pkg.bzl", "pkg_tar")
load("@rules_pkg//:providers.bzl", "PackageFilegroupInfo", "PackageFilesInfo", "PackageSymlinkInfo")
load("@bazel_skylib//lib:paths.bzl", "paths")
load("@rules_proto//proto:defs.bzl", "ProtoInfo")

_MELTAN_WORKSPACE = "meltan"

def _parse_label(label):
    """Parse a label into (package, name).

    Args:
      label: string in relative or absolute form.

    Returns:
      Pair of strings: package, relative_name

    Raises:
      ValueError for malformed label (does not do an exhaustive validation)
    """
    if label.startswith("//"):
        label = label[2:]  # drop the leading //
        colon_split = label.split(":")
        if len(colon_split) == 1:  # no ":" in label
            pkg = label
            _, _, target = label.rpartition("/")
        else:
            pkg, target = colon_split  # fails if len(colon_split) != 2
    else:
        colon_split = label.split(":")
        if len(colon_split) == 1:  # no ":" in label
            pkg, target = native.package_name(), label
        else:
            pkg2, target = colon_split  # fails if len(colon_split) != 2
            pkg = native.package_name() + ("/" + pkg2 if pkg2 else "")
    return pkg, target

def _descriptor_set_impl(ctx):
    """Create a FileDescriptorSet with the transitive inclusions of the passed protos.

    Emulates the output of `protoc --include_imports --descriptor_set_out=$@`
    In Bazel that would require recompiling protoc, so use the existing ProtoInfo.
    """
    output = ctx.actions.declare_file(ctx.attr.name + "-transitive-descriptor-set.proto.bin")
    descriptors = depset(transitive = [
        dep[ProtoInfo].transitive_descriptor_sets
        for dep in ctx.attr.deps
    ])
    ctx.actions.run_shell(
        outputs = [output],
        inputs = descriptors,
        arguments = [file.path for file in descriptors.to_list()],
        command = "cat $@ >%s" % output.path,
    )
    return DefaultInfo(
        files = depset([output]),
        runfiles = ctx.runfiles(files = [output]),
    )

descriptor_set = rule(
    attrs = {"deps": attr.label_list(providers = [ProtoInfo])},
    implementation = _descriptor_set_impl,
)

_DEFAULT_PROTO = "@com_google_protobuf//:empty_proto"

def meltan_test_pkg(
        name,
        binary,
        params_proto = _DEFAULT_PROTO,
        json_defaults = None,
        **kwargs):
    """Packages a meltan binary into a self-extracting launcher.

    Args:
      name: Name of the launcher.
      binary: Executable program for the meltan test; must include all data deps.
      params_proto: proto_library rule for the input parameters.
      json_defaults: Optional JSON file with default values for the params.
      **kwargs: Additional args to pass to the shell binary rule.
    """
    descriptors = "_meltan_test_pkg_%s_descriptors" % name
    descriptor_set(
        name = descriptors,
        deps = [params_proto],
        visibility = ["//visibility:private"],
    )

    launcher = "_meltan_test_pkg_%s_launcher" % name
    meltan_launcher(
        name = launcher,
        binary = binary,
        descriptors = ":%s" % descriptors,
        defaults = json_defaults,
        visibility = ["//visibility:private"],
    )

    meltan_sar_name = name

    create_meltan_sar(meltan_sar_name, ":" + launcher)

def join_paths(path, *others):
    return paths.normalize(paths.join(path, *others))

def runpath(ctx, file):
    return join_paths(
        ctx.workspace_name,
        file.owner.workspace_root,
        file.short_path,
    )

def valid(items):
    return [item for item in items if item]

def _launcher_gen_impl(ctx):
    template = """#!/bin/sh
export RUNFILES=${{RUNFILES:-$0.runfiles}}
exec {launcher} {binary} {descriptor} {defaults} "$@"
"""
    prefix = '"${RUNFILES}/%s"'
    script = template.format(
        launcher = prefix % runpath(ctx, ctx.executable._launcher),
        binary = prefix % runpath(ctx, ctx.executable.binary),
        descriptor = prefix % runpath(ctx, ctx.file.descriptor),
        defaults = prefix % runpath(ctx, ctx.file.defaults) if ctx.attr.defaults else "",
    )
    shfile = ctx.actions.declare_file(ctx.attr.name + ".sh")
    ctx.actions.write(shfile, script, is_executable = True)

    deps = valid([ctx.attr._launcher, ctx.attr.binary, ctx.attr.descriptor, ctx.attr.defaults])
    runfiles = ctx.runfiles(valid([ctx.file.descriptor, ctx.file.defaults]))
    for dep in deps:
        runfiles = runfiles.merge(dep[DefaultInfo].default_runfiles)
    return [DefaultInfo(
        files = depset([shfile]),
        runfiles = runfiles,
    )]

_launcher_gen = rule(
    implementation = _launcher_gen_impl,
    attrs = {
        "binary": attr.label(mandatory = True, executable = True, cfg = "target"),
        "descriptor": attr.label(mandatory = True, allow_single_file = True),
        "defaults": attr.label(allow_single_file = True),
        "_launcher": attr.label(
            default = "//meltan/lib/params:meltan_launcher",
            executable = True,
            cfg = "target",
        ),
    },
)

def meltan_launcher(name, binary, descriptors, defaults, **kwargs):
    script = "_%s_meltan_launcher_script" % name
    _launcher_gen(
        name = script,
        binary = binary,
        descriptor = descriptors,
        defaults = defaults,
        visibility = ["//visibility:private"],
    )
    native.sh_binary(
        name = name,
        srcs = [":%s" % script],
        **kwargs
    )

def _create_sar(ctx):
    sar_header = "_meltan_test_pkg_%s_sar_header.sh" % ctx.label.name
    sar_header_file = ctx.actions.declare_file(sar_header)
    sar_contents = """#!/bin/sh -e
RUNFILES=$(
  CMD=$(basename $0)
  DIR=${TMPDIR:-/tmp}
  EXEC=$(mktemp $DIR/$CMD.XXX); chmod a+x $EXEC
  if ! $EXEC >/dev/null 2>&1; then
    DIR=${XDG_RUNTIME_DIR:?"No Executable Run Dir"}
  fi
  rm $EXEC
  mktemp -d $DIR/$CMD.XXX
)
FIFO=$RUNFILES/.running; mkfifo $FIFO
(cat $FIFO; rm -rf $RUNFILES)&
exec 8>$FIFO 9<&0 <$0
until [ "$l" = "+" ]; do
  read l
done
tar xmz -C $RUNFILES
exec <&9 9<&- $RUNFILES/%s "${@}"
exit
+\n""" % (ctx.executable.exec.basename)
    ctx.actions.write(output = sar_header_file, content = sar_contents)

    sar_output = ctx.actions.declare_file(ctx.label.name)

    if len(ctx.attr.srcs) != 1:
        fail("meltan_self_extracting_archive srcs only accepts one argument in 'srcs', got %s" % len(ctx.attr.srcs))
    tar_file = ctx.attr.srcs[0][DefaultInfo].files.to_list()[0]
    ctx.actions.run_shell(
        inputs = [sar_header_file, tar_file],
        outputs = [sar_output],
        command = "cat %s %s > %s" % (sar_header_file.path, tar_file.path, sar_output.path),
    )

    return DefaultInfo(
        files = depset([sar_output]),
        executable = sar_output,
    )

meltan_self_extracting_archive = rule(
    implementation = _create_sar,
    attrs = {
        "srcs": attr.label_list(
            mandatory = True,
            doc = "Source tarball to package as SAR payload.",
        ),
        "exec": attr.label(
            mandatory = True,
            doc = "Binary rule to execute from the passed tarball",
            executable = True,
            cfg = "target",
        ),
    },
    executable = True,
)

def make_relative_path(src, tgt):
    return ("../" * src.count("/")) + tgt

def _pkg_runfiles_impl(ctx):
    runfiles = ctx.runfiles()
    files = []  # List of depsets, for deduplication.
    binaries = []  # List of executables
    for src in [s[DefaultInfo] for s in ctx.attr.srcs]:
        runfiles = runfiles.merge(src.default_runfiles)
        files.append(src.files)
        if src.files_to_run:
            binaries.append(src.files_to_run.executable)
    runfiles_files = runfiles.files.to_list()
    regular_files = depset(direct = binaries, transitive = files).to_list()
    empty_file = ctx.actions.declare_file(ctx.attr.name + ".empty")
    ctx.actions.write(empty_file, "")

    workspace_prefix = join_paths(ctx.attr.runfiles_prefix, ctx.workspace_name)
    manifest = PackageFilegroupInfo(
        pkg_files = [(files_info, ctx.label) for files_info in [
            # Add runfiles to the local workspace.
            PackageFilesInfo(dest_src_map = {
                join_paths(workspace_prefix, file.owner.workspace_root, file.short_path): file
                for file in runfiles_files
            }),
            # Add regular files to root of the package.
            PackageFilesInfo(dest_src_map = {
                file.basename: file
                for file in regular_files
            }),
            # Add empty runfiles.
            PackageFilesInfo(dest_src_map = {
                join_paths(workspace_prefix, file): empty_file
                for file in runfiles.empty_filenames.to_list()
            }),
        ]],
        # Declare runfiles symlinks
        pkg_symlinks = [(sym_info, ctx.label) for sym_info in [
            PackageSymlinkInfo(
                destination = join_paths(workspace_prefix, sym.path),
                source = make_relative_path(sym.path, sym.target_file.short_path),
            )
            for sym in runfiles.symlinks.to_list()
        ] + [
            # Add top-level symlinks to external repos' runfiles.
            PackageSymlinkInfo(
                destination = join_paths(ctx.attr.runfiles_prefix, name),
                source = join_paths(ctx.workspace_name, path),
            )
            for name, path in {
                file.owner.workspace_name: file.owner.workspace_root
                for file in runfiles_files
                if file.owner.workspace_root
            }.items()
        ]],
        pkg_dirs = [],  # pkg_* rules do not gracefully handle empty fields.
    )

    return [
        manifest,
        DefaultInfo(files = depset([empty_file], transitive = files + [runfiles.files])),
    ]

pkg_tar_runfiles = rule(
    implementation = _pkg_runfiles_impl,
    provides = [PackageFilegroupInfo],
    attrs = {
        "srcs": attr.label_list(
            mandatory = True,
            doc = "Target to include in the tarball.",
        ),
        "runfiles_prefix": attr.string(
            doc = "Path prefix for the runfiles directory",
            default = ".",
        ),
    },
)

def create_meltan_sar(name, launcher, **kwargs):
    """ Bundles a meltan diag into a self-extracting archive.

    Args:
      name:      Name of the meltan_test_pkg, will also be the sar output name
      launcher:  Executable to be bundled into the SAR
      **kwargs:  Any arguments to be forwarded to gentar or the self-extracting archive generator
    """

    # Extract just the executable name, discarding the path.
    launcher_path = _parse_label(launcher)[1]

    runfiles = "_%s_runfiles" % name
    pkg_tar_runfiles(
        name = runfiles,
        srcs = [launcher],
        runfiles_prefix = launcher_path + ".runfiles",
        visibility = ["//visibility:private"],
    )

    tar_name = "_%s_test_tar" % name
    pkg_tar(
        name = tar_name,
        srcs = [":" + runfiles],
        extension = "tgz",
        visibility = ["//visibility:private"],
    )

    meltan_self_extracting_archive(
        name = name,
        srcs = [":" + tar_name],
        exec = launcher,
        **kwargs
    )
