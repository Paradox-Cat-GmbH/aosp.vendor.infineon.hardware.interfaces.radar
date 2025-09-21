/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "infineon_radar_aidl_hal_test"

#include "LogRedirector.h" // FIXME it's a duplicate - extract somewhere?

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/vendor/infineon/radar/IRadarSdk.h>
#include <aidl/vendor/infineon/radar/BnRawDataListener.h>
#include <android/binder_ibinder.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android-base/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

namespace aidl::vendor::infineon::radar {

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk()) << ret.getDescription()
#define EXPECT_OK(ret) EXPECT_TRUE(ret.isOk()) << ret.getDescription()

class MockListener : public BnRawDataListener
{
public:
    MOCK_METHOD(ndk::ScopedAStatus, onFrameReceived, (const FrameData& in_data), (override));
};

class RadarSdkAidl : public testing::Test {
  public:
    void SetUp() override {
        const std::string name = std::string() + IRadarSdk::descriptor + "/default";
        ASSERT_TRUE(AServiceManager_isDeclared(name.c_str())) << name;
        ndk::SpAIBinder binder(AServiceManager_waitForService(name.c_str()));
        ASSERT_NE(binder, nullptr);
        radarSdk_ = IRadarSdk::fromBinder(binder);
        ASSERT_NE(radarSdk_, nullptr);
    }

    std::shared_ptr<IRadarSdk> radarSdk_;
};

TEST_F(RadarSdkAidl, RawDataCallbackIsCalled)
{
    SensorConfig config = {};
    config.sample_rate_Hz = 2000000;
    config.rx_mask = 7; // antennas RX1, RX2, RX3
    config.tx_mask = 1; // antenna TX1
    config.tx_power_level = 31;
    config.if_gain_dB = 30;
    config.start_frequency_Hz = 58500000000;
    config.end_frequency_Hz = 62500000000;
    config.num_samples_per_chirp = 64;
    config.num_chirps_per_frame = 32;
    config.chirp_repetition_time_s = 0.0002997874980792403;
    config.frame_repetition_time_s = 0.03004460036754608;
    config.hp_cutoff_Hz = 80000;
    config.aaf_cutoff_Hz = 500000;
    config.mimo_mode = 0; // IFX_MIMO_OFF
    int64_t subscription_id = -1;

    auto callback = ndk::SharedRefBase::make<MockListener>();
    // data comes from another thread, we need to wait for it and keep the test running
    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;
    EXPECT_CALL(*callback, onFrameReceived)
        .WillOnce(testing::Invoke(
            [&mutex, &cv, &done](const FrameData& frame) {
                std::unique_lock<std::mutex> lock(mutex);
                done = true;
                cv.notify_one();
                return ndk::ScopedAStatus::ok();
            }));

    // act & assert
    ASSERT_OK(radarSdk_->subscribe(callback, config, &subscription_id));
    EXPECT_TRUE(subscription_id > 0);

    // wait until the callback happens (with timeout)
    {
        std::unique_lock<std::mutex> lock(mutex);
        bool timeout = ! cv.wait_for(lock, std::chrono::seconds(1), [&done] { return done; });
        EXPECT_FALSE(timeout);
    }

    ASSERT_OK(radarSdk_->unsubscribe(subscription_id));
}

} // namespace aidl::vendor::infineon::radar

int main(int argc, char** argv)
{
    aidl::vendor::infineon::radar::LogRedirector();
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    auto res = RUN_ALL_TESTS();
    // workaround for LogRedirector to catch up
    // TODO proper solution: https://trello.com/c/Yg57ge4Y/53-logredirector-flush-before-dying
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return res;
}
