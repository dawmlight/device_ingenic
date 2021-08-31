#ifndef ABOUT_MODULE_H_
#define ABOUT_MODULE_H_

#include <stdio.h>
#include <protocol/SyncModule.h>

namespace android {
class AboutModule : public SyncModule {
 public:
    static AboutModule* getInstance(RefBase* protocol);
    virtual ~AboutModule();
 private:
    AboutModule(RefBase* protocol);
    static AboutModule* sInstance;
    void onRetrive(const sp<SyncData> & data);
    int respondUpTime();
    int respondCpu();
    int respondVersion();
    int respondSerial();
    int respondRamTotalSize();
    int respondLanguage();
    int sendPowerLevel();
    int sendStorageSize();
    int respondBlueSerial();
    int numLanguage();
    void getCpuInfo(string & info);
    void getMemoryInfo(string & total, string & free, string & used);
    void formatSizeString(long long size, string & total);
};
}
#endif  // ABOUT_MODULE_H_
