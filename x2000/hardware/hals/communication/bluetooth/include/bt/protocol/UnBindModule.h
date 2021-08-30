#ifndef UNBIND_MODULE_H_
#define UNBIND_MODULE_H_

#include <stdio.h>
#include "SyncModule.h"

namespace android {
class UnBindModule : public SyncModule {
 public:
    static UnBindModule* getInstance(RefBase* protocol);
    ~UnBindModule();
 private:
    UnBindModule(RefBase* protocol);
    static UnBindModule* sInstance;
    void onRetrive(const sp<SyncData> & data);
};
}
#endif  // UNBIND_MODULE_H_
