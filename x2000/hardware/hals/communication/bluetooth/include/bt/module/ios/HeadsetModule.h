#ifndef HEADSET_MODULE_H_
#define HEADSET_MODULE_H_

#include <stdio.h>
#include <protocol/SyncModule.h>

namespace android {
class HeadsetModule : public SyncModule {
 public:
     static HeadsetModule* getInstance(RefBase* protocol);
     virtual ~HeadsetModule();
 private:
    HeadsetModule(RefBase* protocol);
    static HeadsetModule* sInstance;
    void onRetrive(const sp<SyncData> & data);
    int sendHeadsetState(int state);
};
}
#endif  // HEADSET_MODULE_H_
