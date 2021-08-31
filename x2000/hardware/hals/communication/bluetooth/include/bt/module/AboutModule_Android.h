#ifndef ABOUT_ANDROID_MODULE_H_
#define ABOUT_ANDROID_MODULE_H_

#include <stdio.h>
#include <protocol/SyncModule.h>

namespace android {
class AboutModule_Android : public SyncModule {
 public:
    static AboutModule_Android* getInstance(RefBase* protocol);
    virtual ~AboutModule_Android();
 private:
    AboutModule_Android(RefBase* protocol);
    static AboutModule_Android* sInstance;
    void onRetrive(const sp<SyncData> & data);
    void getCpuInfo(string & info);
    int respondCpu();
    int respondVersion();
    int respondUpTime();
    int respondRamTotalSize();
    void getMemoryInfo(string & total, string & free, string & used);
    void formatSizeString(long long size, string & src);
};
}
#endif  // ABOUT_ANDROID_MODULE_H_
