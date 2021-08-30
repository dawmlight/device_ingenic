/*
 *
 */

#ifndef BleProtocol_H_
#define BleProtocol_H_

#include <stdio.h>
#include <pthread.h>
#include <utils/RefBase.h>
#include <string>
#include "DBHelper.h"
#include <protocol/SyncModule.h>
/*
#include "AboutModule.h"
#include "ContactsModule.h"
#include "ControlModule.h"
#include "HeadsetModule.h"
#include "LanguageModule.h"
#include "LiveModule.h"
#include "PhotoModule.h"
#include "SettingModule.h"
#include "UpdateModule.h"
#include "WifiModule.h"
*/
#include "LongSyncData.h"
#include "ThirdPartyServer.h"
#ifdef BLUETOOTH_HILINK_SUPPORT
#include "HilinkServer.h"
#endif

using  std::string;
namespace android {
class BleProtocol: public RefBase {
public:
    typedef std::map<string, SyncModule*> MODULEMAP;
    virtual ~BleProtocol();
    static BleProtocol* getInstance();
    int checkBindState(const char *remote_addr);
    void recvUnbond();
    void registerModule(const string & module_name, SyncModule * module);
    void unRegisterModule(const string & module_name);
    int sendPacket(const string & moduleName, char* data, int size);
    bool getBleBindedState();
private:
    BleProtocol();
    static BleProtocol* sInstance;
    bool mIsBinded;
    static int mConId;
    sp<DBHelper> mDBHelper;
    MODULEMAP mModuleMap;
    /*
    AboutModule* mAboutModule;
    ContactsModule* mContactsModule;
    ControlModule* mControlModule;
    HeadsetModule* mHeadsetModule;
    LanguageModule* mLanguageModule;
    LiveModule* mLiveModule;
    PhotoModule* mPhotoModule;
    SettingModule* mSettingModule;
    UpdateModule* mUpdateModule;
    WifiModule* mWifiModule;
    */
    ThirdPartyServer* mThirdPartyServer;
    string mBluetoothAddress;
    typedef std::map<string,LongSyncData*> MapLongData;
    MapLongData mMapLongData;
    void processPackage(char *data, int size);
    int unbond();
    static void receiveWriteRequestCback(char* address, int con_id, char * data, int size);
    static void onConnectionStateChange(char* add, int state);
    void processBindConnectRequest(char* address,int con_id,int length,char *dataBuf,bool bindRequest);
    void parserPackage(char *package, int size);
    int sendToSource(int con_id, char* data, int size);//send data to source
    void startModule();
    void stopModule();
    void moduleRetrive(const char * moduleName,char * data,int size);
#ifdef BLUETOOTH_HILINK_SUPPORT
    HilinkServer *mHilinkServer;
    static int mHilinkConId[4];
    static void hilinkOnConnectionStateChange(char *addr, int link_state, int con_id, int server_num);
    static void hilinkReceiveWriteRequestCback(int server_no, char* address, int con_id, char * data, int size);
public:
    int sendPacket(const string &moduleName, char *data, int size, bool ishilink);
#endif
};
}  // namespace android

#endif  // BleProtocol_H_
