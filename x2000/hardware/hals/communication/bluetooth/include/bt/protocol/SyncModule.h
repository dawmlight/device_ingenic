#ifndef SYNCMODULE_H_
#define SYNCMODULE_H_

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <utils/RefBase.h>
#include "SyncData.h"  

namespace android {
class SyncModule : public RefBase{
 public:
    SyncModule(const string & moduleName, RefBase* protocol);
    void send(const sp<SyncData> & data);

    virtual ~SyncModule();
    virtual void onRetrive(const sp<SyncData> & data);
    static int isBleBinded();
    string getModuleName(){return mModuleName;};
 private:
    string mModuleName;
    SyncModule* mModule;
    RefBase* mProtocol;
    int mType; //1 SppProtocol,2 BleProtocol
};
}
#endif  // SYNCMODULE_H_
