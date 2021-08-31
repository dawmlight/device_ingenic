#ifndef POWERSTORAGE_MODULE_H_
#define POWERSTORAGE_MODULE_H_

#include <stdio.h>
#include <protocol/SyncModule.h>

namespace android {
class PowerStorageModule : public SyncModule {
 public:
     static PowerStorageModule* getInstance(RefBase* protocol);
     virtual ~PowerStorageModule();
 private:
    PowerStorageModule(RefBase* protocol);
    static PowerStorageModule* sInstance;
    void onRetrive(const sp<SyncData> & data);
    void responsPowerLevel();
    void responsStorageInfo();
};
}
#endif  // POWERSTORAGE_MODULE_H_
