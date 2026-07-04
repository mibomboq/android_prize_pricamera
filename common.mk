#
# Copyright (C) 2024 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

PRIZE_PATH := device/prize/camera

# Soong Namespace
PRODUCT_SOONG_NAMESPACES += \
    $(PRIZE_PATH)

# Inherit from the proprietary version
$(call inherit-product, vendor/prize/camera/camera-vendor.mk)

# Dex
PRODUCT_DEXPREOPT_SPEED_APPS += \
    PriCamera

# Priv-app permission
PRODUCT_COPY_FILES += \
    $(PRIZE_PATH)/configs/privapp-permissions-pricamera.xml:$(TARGET_COPY_OUT_SYSTEM_EXT)/etc/permissions/privapp-permissions-pricamera.xml

# Sysconfig
PRODUCT_COPY_FILES += \
    $(PRIZE_PATH)/configs/pricamera-hiddenapi-package-allowlist.xml:$(TARGET_COPY_OUT_SYSTEM_EXT)/etc/sysconfig/pricamera-hiddenapi-package-allowlist.xml
