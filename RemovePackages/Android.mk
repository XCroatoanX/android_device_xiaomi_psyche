LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := RemovePackages
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_TAGS := optional
LOCAL_OVERRIDES_PACKAGES := \
    Camera2 \
    Drive \
    GoogleCamera \
    Music \
    PrebuiltGmail \
    Snap \
    Snap2 \
    SnapdragonCamera \
    YouTube \
    YouTubeMusicPrebuilt \
    Photos \
    Chrome \
    Chrome-Stub \
    Gallery2 \
    Music \
    Browser2 \
    MusicFX \
    Recorder \
    Jelly \
    Eleven \
    WellbeingPrebuilt \
    PlayAutoInstallConfig \
    Aperture
LOCAL_UNINSTALLABLE_MODULE := true
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_SRC_FILES := /dev/null
include $(BUILD_PREBUILT)
