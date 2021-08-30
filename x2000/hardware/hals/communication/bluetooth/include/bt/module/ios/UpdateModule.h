#ifndef UPDATE_MODULE_H_
#define UPDATE_MODULE_H_

#include <stdio.h>
#include <protocol/SyncModule.h>

namespace android {
class UpdateModule : public SyncModule {
 public:
     static UpdateModule* getInstance(RefBase* protocol);
     virtual ~UpdateModule();
 private:
    UpdateModule(RefBase* protocol);
    static UpdateModule* sInstance;
    void onRetrive(const sp<SyncData> & data);
};
}
#endif  // UPDATE_MODULE_H_
