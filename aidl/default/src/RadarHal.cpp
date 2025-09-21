#include "RadarHal.h"
#include "ifxBase/Base.h"
#include <android-base/logging.h>
#include <cassert>
#include <chrono>
#include <iomanip>
#include <thread>

namespace aidl::vendor::infineon::radar {

// could be extracted to utils, but currently is only needed in this translation unit
namespace
{
    ifx_Avian_Config_t fromSensorConfig(const SensorConfig& config)
    {
        return {
            .sample_rate_Hz = static_cast<uint32_t>(config.sample_rate_Hz),
            .rx_mask = static_cast<uint32_t>(config.rx_mask),
            .tx_mask = static_cast<uint32_t>(config.tx_mask),
            .tx_power_level = static_cast<uint32_t>(config.tx_power_level),
            .if_gain_dB = static_cast<uint32_t>(config.if_gain_dB),
            .start_frequency_Hz = static_cast<uint64_t>(config.start_frequency_Hz),
            .end_frequency_Hz = static_cast<uint64_t>(config.end_frequency_Hz),
            .num_samples_per_chirp = static_cast<uint32_t>(config.num_samples_per_chirp),
            .num_chirps_per_frame = static_cast<uint32_t>(config.num_chirps_per_frame),
            .chirp_repetition_time_s = config.chirp_repetition_time_s,
            .frame_repetition_time_s = config.frame_repetition_time_s,
            .hp_cutoff_Hz = static_cast<uint32_t>(config.hp_cutoff_Hz),
            .aaf_cutoff_Hz = static_cast<uint32_t>(config.aaf_cutoff_Hz),
            .mimo_mode = config.mimo_mode == 0 ? IFX_MIMO_OFF : IFX_MIMO_TDM,
        };
    }
}

using namespace std::chrono_literals;

ndk::ScopedAStatus RadarHal::subscribe(const std::shared_ptr<IRawDataListener>& in_listener,
    const SensorConfig& in_config, int64_t* out_subscription_id)
{
    if (in_listener == nullptr)
    {
        LOG(ERROR) << "Provided listener is nullptr!";
        *out_subscription_id = -1;
        return ndk::ScopedAStatus::ok();
    }
      
    // refuse to subscribe if other listeners exist and use different config
    if (! mRawDataListeners.empty() && in_config != mCurrentConfig)
    {
        LOG(ERROR) << "Provided configuration is different to the active one used by other active listeners, aborting subscription";
        // TODO (would be nice to have) print the diff between configs
        *out_subscription_id = -1;
        return ndk::ScopedAStatus::ok();
    }

    // if this is a first listener, connect sensor, set config, start data acquisition
    printActiveListeners();
    if (mRawDataListeners.empty())
    {
        mCurrentConfig = in_config;
        if (! connectSensor())
        {
            *out_subscription_id = -1;
            return ndk::ScopedAStatus::ok();
        }
        startDataAcquisition();
    }
    else
    {
        if (! mDeviceHandle)
            LOG(INFO) << "There are listeners, but sensor is not connected. This new subscriber won't get data until sensor is reconnected.";
        if (mStopRawDataAcquisition)
            LOG(INFO) << "There are listeners, but data acquisition is not active. Apparently waiting to reconnect";
    }

    // FIXME better id generation! https://trello.com/c/a8CT7GWL/57-better-id-generation-for-subscriptions
    *out_subscription_id = static_cast<int64_t>(rand()) << 32 | rand();
    LOG(DEBUG) << "Adding listener 0x" << std::hex << *out_subscription_id << std::dec << " ...";
    mRawDataListeners[*out_subscription_id] = in_listener;

    LOG(DEBUG) << "Subscription successful. Generated subscription id = 0x" << std::hex << *out_subscription_id << std::dec;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadarHal::unsubscribe(int64_t subscription_id)
{
    auto it = mRawDataListeners.find(subscription_id);
    if (it == mRawDataListeners.end())
    {
        LOG(ERROR) << "Could not find subscription with id 0x" << std::hex << subscription_id << std::dec << ". Cannot unsubscribe.";
        printActiveListeners();
        return ndk::ScopedAStatus::ok();
    }
    LOG(DEBUG) << "Removing subscription 0x" << std::hex << subscription_id << std::dec << " ...";
    mRawDataListeners.erase(it);
    printActiveListeners();
    if (mRawDataListeners.empty())
    {
        stopDataAcquisition();
        disconnectSensor();
    }
    LOG(DEBUG) << "unsubscribe() was successful";
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadarHal::unsubscribeAll()
{
    LOG(DEBUG) << "Removing all listeners...";
    mRawDataListeners.clear();
    printActiveListeners();
    stopDataAcquisition();
    disconnectSensor();
    LOG(DEBUG) << "unsubscribeAll() was successful";
    return ndk::ScopedAStatus::ok();
}

binder_status_t RadarHal::dump(int fd, const char** /* args */, uint32_t /* numArgs */)
{
    dprintf(fd, "Radar SDK version: %s\n", ifx_sdk_get_version_string_full());
    if (! mDeviceHandle)
    {
        dprintf(fd, "No sensor connected\n");
        return STATUS_OK;
    }
    dprintf(fd, "Avian sensor connected\n");
    dprintf(fd, "UUID of board: %s\n", ifx_avian_get_board_uuid(mDeviceHandle));
    // get and print device config
    ifx_Avian_Config_t deviceConfig = {};
    ifx_avian_get_config(mDeviceHandle, &deviceConfig);
    dprintf(fd, "Device configuration:\n");
    dprintf(fd, "\tsample_rate_Hz:          %u\n", deviceConfig.sample_rate_Hz);
    dprintf(fd, "\trx_mask:                 %u\n", deviceConfig.rx_mask);
    dprintf(fd, "\ttx_mask:                 %u\n", deviceConfig.tx_mask);
    dprintf(fd, "\ttx_power_level:          %u\n", deviceConfig.tx_power_level);
    dprintf(fd, "\tif_gain_dB:              %u\n", deviceConfig.if_gain_dB);
    dprintf(fd, "\tstart_frequency_Hz:      %lu\n", deviceConfig.start_frequency_Hz);
    dprintf(fd, "\tend_frequency_Hz:        %lu\n", deviceConfig.end_frequency_Hz);
    dprintf(fd, "\tnum_samples_per_chirp:   %u\n", deviceConfig.num_samples_per_chirp);
    dprintf(fd, "\tnum_chirps_per_frame:    %u\n", deviceConfig.num_chirps_per_frame);
    dprintf(fd, "\tchirp_repetition_time_s: %g\n", deviceConfig.chirp_repetition_time_s);
    dprintf(fd, "\tframe_repetition_time_s: %g\n", deviceConfig.frame_repetition_time_s);
    dprintf(fd, "\thp_cutoff_Hz:            %u\n", deviceConfig.hp_cutoff_Hz);
    dprintf(fd, "\taaf_cutoff_Hz:           %u\n", deviceConfig.aaf_cutoff_Hz);
    dprintf(fd, "\tmimo_mode:               %s\n", deviceConfig.mimo_mode == IFX_MIMO_TDM ? "time-domain multiplexed" : "off");
    dprintf(fd, "\n");
    dprintf(fd, "Registered listeners: %s\n", mRawDataListeners.empty() ? "none" : "");
    for (const auto& [id, listener] : mRawDataListeners)
        dprintf(fd, "\tclientId = %lx\n", id);
    return STATUS_OK;
}

bool RadarHal::connectSensor()
{
    // TODO lock_guard with mutex to avoid races when multiple subscribers are present
    // 
    LOG(DEBUG) << "Connecting to first sensor...";
    if (! mDeviceHandle)
    {
#ifdef FALSE
        ifx_List_t* device_list = ifx_avian_get_list();
        int nDevices = ifx_list_size(device_list);
        std::string uuid = "";
        LOG(DEBUG) << "ifx_avian_get_list() returned " << nDevices << " devices";
        if (nDevices < 1)
            return false;
        for (size_t i = 0; i < nDevices; i++)
        {
            ifx_Radar_Sensor_List_Entry_t* entry = reinterpret_cast<ifx_Radar_Sensor_List_Entry_t*>(ifx_list_get(device_list, i));
            if (! entry)
            {
                LOG(DEBUG) << "ifx_list_get(" << i << ") returned nullptr";
            }
            else
            {
                LOG(DEBUG) << "ifx_list_get(" << i << ") returned:";
                LOG(DEBUG) << "\tsensor_type: " << entry->sensor_type;
                LOG(DEBUG) << "\tboard_type: " << entry->board_type;
                LOG(DEBUG) << "\tuuid: " << entry->uuid;
                uuid = std::string(entry->uuid);
            }
        }
        ifx_list_destroy(device_list);
        mDeviceHandle = ifx_avian_create_by_uuid(uuid.c_str());
#else
        mDeviceHandle = ifx_avian_create();
#endif
        ifx_Error_t error = ifx_error_get_and_clear();
        if (error != IFX_OK)
        {
            LOG(ERROR) << "Failed to open device. Error " << error << ": " << ifx_error_to_string(error);
            disconnectSensor();
            return false;
        }
        LOG(DEBUG) << "Device opened!";
#ifdef FALSE
        LOG(DEBUG) << "Getting UUID...";
        const char* uuid = ifx_avian_get_board_uuid(mDeviceHandle);
        LOG(DEBUG) << "UUID of board: " << uuid;
#endif
        LOG(DEBUG) << "Setting provided configuration...";
        ifx_Avian_Config_t deviceConfig = fromSensorConfig(mCurrentConfig);
        ifx_avian_set_config(mDeviceHandle, &deviceConfig);
        error = ifx_error_get_and_clear();
        if (error != IFX_OK)
        {
            LOG(ERROR) << "Failed to set device config. Error " << error << ": " << ifx_error_to_string(error);
            disconnectSensor();
            return false;
        }
        LOG(DEBUG) << "Sensor config updated successfully";
    }
    else
    {
        LOG(DEBUG) << "Sensor already connected";
    }
    return true;
}

void RadarHal::connectSensorUntilSuccess()
{
    // TODO let revievers know we got an error and we are trying to recover
    // https://trello.com/c/2qTBKYZk/102-communicate-state-of-sensor-connection-to-subscribers
    const std::vector<std::chrono::duration<long long>> reconnectIn = {1s, 2s, 5s, 10s, 30s, 60s};
    int i = 0;
    while (! connectSensor())
    {
        LOG(WARNING) << "Can not retrieve raw data from sensor, reconnecting in "
                << std::chrono::seconds(reconnectIn[i]).count() << " seconds...";
        std::this_thread::sleep_for(reconnectIn[i]);
        if (i < reconnectIn.size() - 1)
            ++i;
    }
}

void RadarHal::disconnectSensor()
{
    LOG(DEBUG) << "Disconnecting sensor...";
    if (mDeviceHandle)
        ifx_avian_destroy(mDeviceHandle);
    mDeviceHandle = nullptr;
}

void RadarHal::printActiveListeners() const
{
    if (! mRawDataListeners.empty())
    {
        LOG(DEBUG) << mRawDataListeners.size() << " listener(s) active:";
        for (const auto& [id, listener] : mRawDataListeners)
            LOG(DEBUG) << "\t0x" << std::hex << id << std::dec;
    }
    else
    {
        LOG(DEBUG) << "No active listeners";
    }
}

void RadarHal::startDataAcquisition()
{
    LOG(DEBUG) << "Starting data acquisition...";
    if (! mStopRawDataAcquisition)
    {
        LOG(DEBUG) << "Data acquisition is already running";
        // NOTE: even if someone unsubscribes now, we already added a new listener, thus it will not stop data acquisition
    }
    else
    {
        mStopRawDataAcquisition = false;
        mRawDataAqcuisitionThread = std::thread([this]()
        {
            ifx_Cube_R_t* raw_frame = nullptr;
            LOG(DEBUG) << "Raw data acquisition started";
            const std::chrono::seconds SILENCE_TIME = 5s;
            auto lastTimePrintedFps = std::chrono::system_clock::now();
            unsigned long long numFramesSinceLastFpsPrint = 0;
            while (! mStopRawDataAcquisition)
            {
                raw_frame = ifx_avian_get_next_frame(mDeviceHandle, raw_frame);
                ifx_Error_t error = ifx_error_get_and_clear();
                if (error != IFX_OK)
                {
                    LOG(ERROR) << "Failed to get next frame. Error " << error << ": " << ifx_error_to_string(error);
                    // Sensor handle is not good anymore, need to reconnect and reconfigure before we can call
                    // ifx_avian_get_next_frame() again.
                    disconnectSensor();
                    connectSensorUntilSuccess();
                    LOG(INFO) << "Sensor is connected again, resuming data acquisition";
                    lastTimePrintedFps = std::chrono::system_clock::now();
                    numFramesSinceLastFpsPrint = 0;
                }
                else
                {
                    const size_t nAntennas = IFX_MDA_SHAPE(raw_frame)[0];
                    const size_t nChirps = IFX_MDA_SHAPE(raw_frame)[1];
                    const size_t nSamples = IFX_MDA_SHAPE(raw_frame)[2];
                    assert(nChirps == mCurrentConfig.num_chirps_per_frame);
                    assert(nSamples == mCurrentConfig.num_samples_per_chirp);
                    FrameData frame = {};
                    frame.data.reserve(nAntennas * nChirps * nSamples);
                    for (uint32_t iAntenna = 0; iAntenna < nAntennas; ++iAntenna)
                        for (uint32_t iChirp = 0; iChirp < nChirps; ++iChirp)
                            for (uint32_t iSample = 0; iSample < nSamples; ++iSample)
                                frame.data.emplace_back(IFX_MDA_AT(raw_frame, iAntenna, iChirp, iSample));
                    // print framerate once every 5 seconds
                    {
                        const auto now = std::chrono::system_clock::now();
                        ++numFramesSinceLastFpsPrint;
                        if (now - lastTimePrintedFps > SILENCE_TIME)
                        {
                            LOG(VERBOSE) << "~" << std::fixed << std::setprecision(2) << 
                                static_cast<float>(numFramesSinceLastFpsPrint) / SILENCE_TIME.count() << " FPS:\tgot " << 
                                numFramesSinceLastFpsPrint << " frames in " << SILENCE_TIME.count() << " seconds";
                            lastTimePrintedFps = now;
                            numFramesSinceLastFpsPrint = 0;
                        }
                    }
                    if (mRawDataListeners.empty())
                        LOG(WARNING) << "Got data, but there are no listeners to notify!";
                    for (const auto& [id, listener] : mRawDataListeners)
                    {
                        listener->onFrameReceived(frame);
                        // LOG(VERBOSE) << "Notified listener 0x" << std::hex << id << std::dec << " ...";
                    }
                }
            }
            ifx_cube_destroy_r(raw_frame);
            LOG(DEBUG) << "Raw data acquisition stopped";
        });
    }
}

void RadarHal::stopDataAcquisition()
{
    LOG(DEBUG) << "Stopping data acquistion...";
    mStopRawDataAcquisition = true;
    if (mRawDataAqcuisitionThread.joinable())
        mRawDataAqcuisitionThread.join();
}

} // namespace aidl::vendor::infineon::radar