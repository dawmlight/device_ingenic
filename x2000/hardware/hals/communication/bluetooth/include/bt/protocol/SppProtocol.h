/*
 *
 */

#ifndef SppProtocol_H_
#define SppProtocol_H_

#include <stdio.h>
#include <pthread.h>

#include "Cfg.h"
#include "DBHelper.h"
#include <protocol/SyncModule.h>
/*
#include "AboutModule_Android.h"
#include "ContactsModule.h"
#include "ControlModule.h"
#include "HeadsetModule.h"
#include "LanguageModule.h"
#include "LiveModule.h"
#include "PhotoModule.h"
#include "SettingModule_Android.h"
#include "UpdateModule.h"
#include "WifiModule.h"
#include "PowerStorageModule.h"
*/
#include "ThirdPartyServer.h"

namespace android {
class SppProtocol : public RefBase{
public:
    typedef std::map<string, SyncModule*> MODULEMAP;
    static SppProtocol* getInstance();
    virtual ~SppProtocol();
    void disconnect();
    void registerModule(const string & module_name, SyncModule * module);
    void unRegisterModule(const string & module_name);
    int sendPacket(const string & moduleName,char* data,int size);
    bool getSppBindedState();
private:
    SppProtocol();
    static SppProtocol* sInstance;
    bool mIsBinded;
    bool mCfgWaitData;
    int mConId;
    int mLeftLength;
    char* mLeftData;
    sp<Cfg> mCfg;
    sp<DBHelper> mDBHelper;
    MODULEMAP mModuleMap;
    /*
    AboutModule_Android* mAboutModule;
    ContactsModule* mContactsModule;
    ControlModule* mControlModule;
    HeadsetModule* mHeadsetModule;
    LanguageModule* mLanguageModule;
    LiveModule* mLiveModule;
    PhotoModule* mPhotoModule;
    /SettingModule_Android* mSettingModule;
    UpdateModule* mUpdateModule;
    WifiModule* mWifiModule;
    PowerStorageModule* mPowerStorageModule;
    */
    ThirdPartyServer* mThirdPartyServer;

    void processBindRequest(int con_id,char * remote_addr,int size);
    static void receiveWriteRequestCback(void *user, int con_id, char * data, int size);
    static void linkStateCback(void *user,int con_id, bool linked,char * remote_addr, int size);
    void parserExtraHeader(char *buf,int size);
    void parserPackageHeader(char *package,int size);
    int checkBindState(const char *remote_addr);
    int sendToSource(int con_id, char* data, int size);
    void startModule();
    void stopModule();
    void moduleRetrive(const char * moduleName,char * data,int size);
    void resetBindState();
};
}  // namespace android

#endif  // SppProtocol_H_


