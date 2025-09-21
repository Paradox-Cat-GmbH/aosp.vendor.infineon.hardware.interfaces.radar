#include <string>
#include <memory>

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include "LogRedirector.h"
#include "RadarHal.h"

using aidl::vendor::infineon::radar::RadarHal;

int main() {
    aidl::vendor::infineon::radar::LogRedirector();
    LOG(DEBUG) << "Starting Infineon RadarHal";
    std::shared_ptr<RadarHal> radarHal = ndk::SharedRefBase::make<RadarHal>();
    const std::string instance = std::string() + RadarHal::descriptor + "/default";
    LOG(VERBOSE) << "adding service " << instance;
    LOG(VERBOSE) << "binder: radarHal->asBinder().get() is nullptr: " << (radarHal->asBinder().get() == nullptr);
    LOG(VERBOSE) << "string: " << instance.c_str();
    binder_status_t status = AServiceManager_addService(radarHal->asBinder().get(), instance.c_str());
    LOG(VERBOSE) << "AServiceManager_addService returned: " << status;
    CHECK_EQ(status, STATUS_OK);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    LOG(DEBUG) << "Infineon RadarHal is now running";
    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
