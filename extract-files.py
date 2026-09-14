#!/usr/bin/env -S PYTHONPATH=../../../tools/extract-utils python3
#
# SPDX-FileCopyrightText: 2024 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

from extract_utils.fixups_blob import (
    blob_fixup,
    blob_fixups_user_type,
)

from extract_utils.fixups_lib import (
    lib_fixups,
    lib_fixups_user_type,
)

from extract_utils.main import (
    ExtractUtils,
    ExtractUtilsModule,
)

namespace_imports = [
    'device/prize/camera',
    'vendor/prize/camera',
]


def lib_fixup_system_ext_suffix(lib: str, partition: str, *args, **kwargs):
    return f'{lib}_{partition}' if partition == 'system_ext' else None


lib_fixups: lib_fixups_user_type = {
    **lib_fixups,
    (
        'libBSTSingleAIBokeh.so',
        'libOpenCL.so',
        'libtensorflowlite.so',
    ): lib_fixup_system_ext_suffix,
}

# NOTE: no blob_fixups / replace_needed needed here.
# lib_fixups above renames the *module* to <lib>_system_ext, but each
# module also sets stem: "<original lib name>" (handled automatically
# by extract_utils when a lib_fixup is applied), so the physical .so
# file installed on device keeps its original name. Since the DT_NEEDED
# entries inside libjni_bstportrait.so / libBSTSingleAIBokeh.so still
# point at the original names (libtensorflowlite.so, libBSTSingleAIBokeh.so),
# and the installed file is still named that way, nothing inside the
# blobs needs to be patched. shared_libs: in Android.bp uses the new
# module name (with _system_ext) for Soong's dependency graph, but the
# on-device file name is unchanged.
blob_fixups: blob_fixups_user_type = {}  # fmt: skip

module = ExtractUtilsModule(
    'camera',
    'prize',
    blob_fixups=blob_fixups,
    lib_fixups=lib_fixups,
    namespace_imports=namespace_imports,
)

if __name__ == '__main__':
    utils = ExtractUtils.device(module)
    utils.run()
