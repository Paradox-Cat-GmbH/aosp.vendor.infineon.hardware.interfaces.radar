#pragma once

#include "ifxAvian/DeviceControl.h"

#include <aidl/vendor/infineon/radar/BnRadarSdk.h>

#include <atomic>
#include <memory>
#include <thread>
#include <unordered_map>

namespace aidl::vendor::infineon::radar {

class RadarHal : public BnRadarSdk {
public:
    ndk::ScopedAStatus subscribe(const std::shared_ptr<IRawDataListener>& in_listener, const SensorConfig& in_config, int64_t* out_subscription_id) override;
    ndk::ScopedAStatus unsubscribe(int64_t subscription_id) override;
    ndk::ScopedAStatus unsubscribeAll() override;
    binder_status_t dump(int fd, const char** args, uint32_t numArgs) override;

private:
    ifx_Avian_Device_t* mDeviceHandle = nullptr; // must be reset to nullptr when device is not connected
    SensorConfig mCurrentConfig = {}; // there could be only one active config on the sensor
    std::unordered_map<int64_t, std::shared_ptr<IRawDataListener>> mRawDataListeners; // all listeners must use same config
    std::atomic_bool mStopRawDataAcquisition = true;
    std::thread mRawDataAqcuisitionThread;

    bool connectSensor();
    void connectSensorUntilSuccess(); // will block forever until successfully connected
    void disconnectSensor();
    void printActiveListeners() const;
    void startDataAcquisition();
    void stopDataAcquisition();
};

} // namespace aidl::vendor::infineon::radar