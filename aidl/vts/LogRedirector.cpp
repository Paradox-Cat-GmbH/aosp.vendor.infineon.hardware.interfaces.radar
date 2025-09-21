#include "LogRedirector.h"

#include <android-base/logging.h>

#include <cstdio>
#include <iostream>
#include <thread>
#include <unistd.h>


namespace aidl::vendor::infineon::radar {

/**
 * Adopted by Viktor Mukha to C++ from here (license is public domain)
 * https://codelab.wordpress.com/2014/11/03/how-to-use-standard-output-streams-for-logging-in-android-apps/
 */

LogRedirector::LogRedirector()
{
    LOG(INFO) << "LogRedirector is initializing...";
    
    // log everything
    // NOTE: if performance is affected, make it dependent on build type (userdebug/eng/rel)
    android::base::SetMinimumLogSeverity(::android::base::VERBOSE);
        
    /* make stdout line-buffered and stderr unbuffered */
    setvbuf(stdout, 0, _IOLBF, 0);
    setvbuf(stderr, 0, _IONBF, 0);

    /* create the pipe and redirect stdout and stderr */
    pipe(mPipe);
    dup2(mPipe[1], 1);
    dup2(mPipe[1], 2);

    mThread = std::thread([this]()
    {
        LOG(DEBUG) << "LogRedirector thread is now running";
        
        ssize_t rdsz;
        char buf[128];
        for(;;)
        {
            if(((rdsz = read(mPipe[0], buf, sizeof buf - 1)) > 0))
            {
                if(buf[rdsz - 1] == '\n')
                    --rdsz;
                buf[rdsz] = 0;  /* add null-terminator */
                LOG(DEBUG) << buf;
            }
        }
    });
    mThread.detach(); // TODO https://trello.com/c/Yg57ge4Y/53-logredirector-flush-before-dying
}

} // namespace aidl::vendor::infineon::radar