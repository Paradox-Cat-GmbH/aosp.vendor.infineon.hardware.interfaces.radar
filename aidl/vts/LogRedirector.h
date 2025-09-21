#pragma once

#include <thread>

namespace aidl::vendor::infineon::radar {

/**
 * Redirect stdout and stderr to logcat using NDK logging methods.
 * This is necessary to see logs from the infineon libraries we use.
 */
class LogRedirector final {
public:    
    explicit LogRedirector();
private:
    std::thread mThread;
    int mPipe[2];
};

} // namespace aidl::vendor::infineon::radar