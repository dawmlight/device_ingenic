#ifndef CONTROL_MODULE_H_
#define CONTROL_MODULE_H_

#include <stdio.h>
#include <protocol/SyncModule.h>

namespace android {
class ControlModule : public SyncModule {
 public:
     static ControlModule* getInstance(RefBase* protocol);
     virtual ~ControlModule();
 private:
    ControlModule(RefBase* protocol);
    static ControlModule* sInstance;
    void onRetrive(const sp<SyncData> & data);
};
}
#endif  // CONTROL_MODULE_H_
