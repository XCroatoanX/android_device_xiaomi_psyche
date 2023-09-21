/*
 * Copyright (C) 2022 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "UdfpsHandler.xiaomi_kona"

#include "UdfpsHandler.h"

#include <android-base/logging.h>
#include <android-base/unique_fd.h>
#include <fcntl.h>
#include <fstream>
#include <poll.h>
#include <thread>
#include <unistd.h>

#define COMMAND_NIT 10
#define PARAM_NIT_UDFPS 1
#define PARAM_NIT_NONE 0

#define UDFPS_STATUS_ON 1
#define UDFPS_STATUS_OFF -1

#define TOUCH_DEV_PATH "/dev/xiaomi-touch"
#define TOUCH_UDFPS_ENABLE 10
#define TOUCH_MAGIC 0x5400
#define TOUCH_IOC_SETMODE TOUCH_MAGIC + 0

#define DISPPARAM_PATH "/sys/devices/platform/soc/ae00000.qcom,mdss_mdp/drm/card0/card0-DSI-1/disp_param"

#define DISPPARAM_HBM_UDFPS_ON "0x20001"
#define DISPPARAM_HBM_UDFPS_OFF "0xE0000"

static const char* kFodUiPaths[] = {
        "/sys/devices/platform/soc/soc:qcom,dsi-display-primary/fod_ui",
        "/sys/devices/platform/soc/soc:qcom,dsi-display/fod_ui",
};

namespace {

    template <typename T>
    static void set(const std::string& path, const T& value) {
        std::ofstream file(path);
        if (!file.is_open()) {
            LOG(ERROR) << "Failed to open file: " << path;
            return;
        }
        file << value;
    }

    static bool readBool(int fd) {
        char c;
        int rc = pread(fd, &c, sizeof(char), 0);
        if (rc < 0) {
            LOG(ERROR) << "Failed to read bool from fd, errno: " << errno;
            return false;
        }
        return c != '0';
    }

} // anonymous namespace

class XiaomiKonaUdfpsHandler : public UdfpsHandler {
  public:
    void init(fingerprint_device_t *device) {
        mDevice = device;
        touch_fd_ = android::base::unique_fd(open(TOUCH_DEV_PATH, O_RDWR));
        if (touch_fd_ < 0) {
            LOG(ERROR) << "Failed to open touch device, errno: " << errno;
            return;
        }

        std::thread([this]() {
            int fd = -1;
            for (auto& path : kFodUiPaths) {
                fd = open(path, O_RDONLY);
                if (fd >= 0) {
                    break;
                }
            }

            if (fd < 0) {
                LOG(ERROR) << "Failed to open fod_ui path, errno: " << errno;
                return;
            }

            struct pollfd fodUiPoll = {
                    .fd = fd,
                    .events = POLLERR | POLLPRI,
                    .revents = 0,
            };

            while (true) {
                int rc = poll(&fodUiPoll, 1, -1);
                if (rc < 0) {
                    LOG(ERROR) << "Failed to poll fd, errno: " << errno;
                    continue;
                }

                mDevice->extCmd(mDevice, COMMAND_NIT,
                                readBool(fd) ? PARAM_NIT_UDFPS : PARAM_NIT_NONE);
            }
        }).detach();
    }

    void onFingerDown(uint32_t /*x*/, uint32_t /*y*/, float /*minor*/, float /*major*/) {
        if (touch_fd_ >= 0) {
            set(DISPPARAM_PATH, DISPPARAM_HBM_UDFPS_ON);
        } else {
            LOG(ERROR) << "Invalid touch file descriptor: " << touch_fd_;
        }
    }

    void onFingerUp() {
        if (touch_fd_ >= 0) {
            set(DISPPARAM_PATH, DISPPARAM_HBM_UDFPS_OFF);
        } else {
            LOG(ERROR) << "Invalid touch file descriptor: " << touch_fd_;
        }
    }

    void onAcquired(int32_t result, int32_t vendorCode) {
        if (result == FINGERPRINT_ACQUIRED_GOOD) {
            if (touch_fd_ >= 0) {
                set(DISPPARAM_PATH, DISPPARAM_HBM_UDFPS_OFF);
                int arg[2] = {TOUCH_UDFPS_ENABLE, UDFPS_STATUS_OFF};
                int rc = ioctl(touch_fd_, TOUCH_IOC_SETMODE, &arg);
                if (rc < 0) {
                    LOG(ERROR) << "Failed to disable fingerprint scanner, errno: " << errno;
                }
                setFodUiState(false); // Turn off fod_ui
            } else {
                LOG(ERROR) << "Invalid touch file descriptor: " << touch_fd_;
            }
        } else if (vendorCode == 21 || vendorCode == 23) {
            if (touch_fd_ >= 0) {
                int arg[2] = {TOUCH_UDFPS_ENABLE, UDFPS_STATUS_ON};
                int rc = ioctl(touch_fd_, TOUCH_IOC_SETMODE, &arg);
                if (rc < 0) {
                    LOG(ERROR) << "Failed to enable fingerprint scanner, errno: " << errno;
                }
            } else {
                LOG(ERROR) << "Invalid touch file descriptor: " << touch_fd_;
            }
        }
    }

    void cancel() {
        if (touch_fd_ >= 0) {
            set(DISPPARAM_PATH, DISPPARAM_HBM_UDFPS_OFF);
            int arg[2] = {TOUCH_UDFPS_ENABLE, UDFPS_STATUS_OFF};
            int rc = ioctl(touch_fd_, TOUCH_IOC_SETMODE, &arg);
            if (rc < 0) {
                LOG(ERROR) << "Failed to disable fingerprint scanner, errno: " << errno;
            }
        } else {
            LOG(ERROR) << "Invalid touch file descriptor: " << touch_fd_;
        }
    }

  private:
    fingerprint_device_t *mDevice;
    android::base::unique_fd touch_fd_;

    void setFodUiState(bool state) {
        for (const auto& path : kFodUiPaths) {
            set(path, state ? "1" : "0");
        }
    }
};

static UdfpsHandler* create() {
    return new XiaomiKonaUdfpsHandler();
}

static void destroy(UdfpsHandler* handler) {
    delete handler;
}

extern "C" UdfpsHandlerFactory UDFPS_HANDLER_FACTORY = {
    .create = create,
    .destroy = destroy,
};