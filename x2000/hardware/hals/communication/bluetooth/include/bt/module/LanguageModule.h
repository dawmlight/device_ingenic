#ifndef LANGUAGE_MODULE_H_
#define LANGUAGE_MODULE_H_

#include <stdio.h>
#include <protocol/SyncModule.h>

namespace android {
class LanguageModule : public SyncModule {
 public:
     static LanguageModule* getInstance(RefBase* protocol);
     virtual ~LanguageModule();
 private:
    LanguageModule(RefBase* protocol);
    static LanguageModule* sInstance;
    void onRetrive(const sp<SyncData> & data);
    void sendSyncResult(int type,int result,int language);
    int setLanguage(int type);
    int setLanguageForIos(int type);
};
}
#endif  // LANGUAGE_MODULE_H_
