#
# Copyright (C) 2024 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

# Inherit from the proprietary version

LOCAL_PATH := vendor/prize/camera

# Properties
TARGET_SYSTEM_EXT_PROP += $(LOCAL_PATH)/system_ext.prop

SYSTEM_EXT_PRIVATE_SEPOLICY_DIRS += $(LOCAL_PATH)/sepolicy/private
SYSTEM_EXT_PUBLIC_SEPOLICY_DIRS += $(LOCAL_PATH)/sepolicy/public
BOARD_VENDOR_SEPOLICY_DIRS += $(LOCAL_PATH)/sepolicy/vendor
