#
# Copyright (C) 2024 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

# Inherit from the proprietary version

PRIZE_PATH := device/prize/camera

# Build Hacks
BUILD_BROKEN_DUP_SYSPROP := true

# Properties
TARGET_SYSTEM_EXT_PROP += $(PRIZE_PATH)/system_ext.prop

# Sepolicy
SYSTEM_EXT_PRIVATE_SEPOLICY_DIRS += $(PRIZE_PATH)/sepolicy/private
SYSTEM_EXT_PUBLIC_SEPOLICY_DIRS += $(PRIZE_PATH)/sepolicy/public
BOARD_VENDOR_SEPOLICY_DIRS += $(PRIZE_PATH)/sepolicy/vendor
