#define LOG_NDEBUG 0
#define LOG_TAG "BleProtocol"

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>

#include "BleProtocol.h"
#include "SyncDataTools.h"
#include <utils/Log.h>

#define LONGDATA_CRC_LENGTH 8
#define LONGDATA_FLAG_LENGTH 10
#define BLE_DATA_MAX_SIZE 150
#define REMOTE_DEVICE_UUID_SIZE 36
//phone request
#define BLE_BOND_REQ_PREFIX "<bind>uuid:"      //bond request prefix
#define BLE_RECONN_REQ_PREFIX "<connect>uuid:" //reconnect request prefix
#define BLE_UNBOND_REQ "unbond"
#define BLE_UNBOND_END_REQ "unbondEnd"
//glass response
#define BLE_BOND_OK "bond:0"      //bond ok
#define BLE_BOND_FAILED "bond:1"  //bond failed
#define BLE_UNBOND_RSP "unbond:0" //glass unbond response

#define BLE_NORMAL_PKG "syncdata"
#define BLE_CONTACT_PKG "<CN>"

#ifdef __cplusplus
extern "C" {
#endif
extern int setBTVisibility(bool discoverable, bool connectable);

/*from app_ble.c*/
extern int app_ble_server_send_indication(int con_id, char *indication, int size);
extern int app_ble_server_register_write_cback(void (*receiveWriteRequestCback)(char *addr, int conn_id, char *data, int size));
extern void app_ble_server_register_connectstate_cback(void (*onConnectionStateChange)(char *add, int state));
extern int app_ble_server_close();
extern int app_ble_wake_configure();
#ifdef BLUETOOTH_HILINK_SUPPORT
extern int hilink_ble_server_register_write_cback(void (*receiveWriteRequestCback)(int server_no, char *addr, int conn_id, char *data, int size));
extern void hilink_app_ble_server_register_connectstate_cback(void (*hilinkOnConnectionStateChange)(char *addr, int link_state, int con_id, int server_num));
#endif
void BleProtocol_instance();
#ifdef __cplusplus
}
#endif

//clivia #define SOUND_UNBIND "/usr/bin/MediaPlayer /etc/sound/unbind.mp3 &"

namespace android {

BleProtocol *BleProtocol::sInstance = NULL;
int BleProtocol::mConId = -1;
#ifdef BLUETOOTH_HILINK_SUPPORT
int BleProtocol::mHilinkConId[4] = {-1,-1,-1,-1};
#endif

class Header : public RefBase {
public:
    /***********type**********/
    static const int NORMAL_PKG = 0x0;
    static const int CONTACT_PKG = 0x1;

    /*******for deserialize******/
    Header(char *data, int len) {
        char *prefix = new char[20];
        memset(prefix, 0, 20);
        memcpy(prefix, data, 20);
        if (strncasecmp(prefix + 4, BLE_NORMAL_PKG, strlen(BLE_NORMAL_PKG)) == 0) {
            mType = NORMAL_PKG;
            uint8_t a = prefix[16];
            uint8_t b = prefix[17];
            uint8_t c = prefix[18];
            uint8_t d = prefix[19];
            int32_t module_len = a << 24 | b << 16 | c << 8 | d;
            mModuleName = new char[module_len + 1];
            memset(mModuleName, '\0', module_len + 1);
            memcpy(mModuleName, data + 20, module_len);

            mPkgLen = len - 16 - module_len;
            mDatas = new char[mPkgLen];
            memset(mDatas, 0, mPkgLen);
            memcpy(mDatas, data + 12, 4);
            memcpy(mDatas + 4, data + 20 + module_len, mPkgLen - 4);
        } else {
            mType = CONTACT_PKG;
            mPkgLen = len;
            mDatas = new char[mPkgLen];
            memset(mDatas, 0, mPkgLen);
            memcpy(mDatas, data, mPkgLen);
        }
        delete[] prefix;
    }

    int getType() {
        return mType;
    }

    int getPkgLen() {
        return mPkgLen;
    }

    void getModule(string &module) {
        module.assign(mModuleName);
    }

    char *getData() {
        return mDatas;
    }

    /*******for serialize******/
    Header(const char *moduleName, char *data, int size) {
        char *prefix = new char[strlen(BLE_CONTACT_PKG)];
        memset(prefix, 0, strlen(BLE_CONTACT_PKG));
        memcpy(prefix, data, strlen(BLE_CONTACT_PKG));
        if (strncasecmp(prefix, BLE_CONTACT_PKG, strlen(BLE_CONTACT_PKG)) == 0) {
            mType = CONTACT_PKG;
            mPkgLen = size;
            mDatas = new char[mPkgLen];
            memset(mDatas, 0, mPkgLen);
            memcpy(mDatas, data, mPkgLen);

        } else {
            mType = NORMAL_PKG;
            int module_len = strlen(moduleName);
            mModuleName = new char[module_len + 1];
            memset(mModuleName, '\0', module_len + 1);
            memcpy(mModuleName, moduleName, module_len);

            mPkgLen = size + 16 + module_len;

            mDatas = new char[mPkgLen];
            memset(mDatas, 0, mPkgLen);
            toBytes(mDatas);
            int len = strlen(BLE_NORMAL_PKG);
            memcpy(mDatas + 4, BLE_NORMAL_PKG, len);
            memcpy(mDatas + 4 + len, data, 4);
            memcpy(mDatas + 20, moduleName, module_len);
            memcpy(mDatas + 20 + module_len, data + 4, size - 4);
        }
    }

    void toBytes(char *buffer) {
        int32_t i = 8;
        char a = (char)(i >> 24);
        char b = (char)(i >> 16);
        char c = (char)(i >> 8);
        char d = (char)i;
        buffer[0] = a;
        buffer[1] = b;
        buffer[2] = c;
        buffer[3] = d;
        int32_t module_len = strlen(mModuleName);
        a = (char)(module_len >> 24);
        b = (char)(module_len >> 16);
        c = (char)(module_len >> 8);
        d = (char)module_len;
        buffer[16] = a;
        buffer[17] = b;
        buffer[18] = c;
        buffer[19] = d;
    }

    ~Header() {
        delete[] mDatas;
    }

private:
    int mType;
    int mPkgLen;
    char *mDatas;
    char *mModuleName;
};

BleProtocol *BleProtocol::getInstance() {
    if (sInstance == NULL) {
        sInstance = new BleProtocol();
    }
    return sInstance;
}

BleProtocol::BleProtocol()
    : mIsBinded(false),
      /*
      mAboutModule(NULL),
      mContactsModule(NULL),
      mControlModule(NULL),
      mHeadsetModule(NULL),
      mLanguageModule(NULL),
      mLiveModule(NULL),
      mPhotoModule(NULL),
      mSettingModule(NULL),
      mUpdateModule(NULL),
      mWifiModule(NULL),
*/
      mThirdPartyServer(NULL),
#ifdef BLUETOOTH_HILINK_SUPPORT
      mHilinkServer(NULL),
#endif
      mBluetoothAddress("") {
    mDBHelper = new DBHelper();

    app_ble_server_register_write_cback(receiveWriteRequestCback);
    app_ble_server_register_connectstate_cback(onConnectionStateChange);
#ifdef BLUETOOTH_HILINK_SUPPORT
    hilink_ble_server_register_write_cback(hilinkReceiveWriteRequestCback);
    hilink_app_ble_server_register_connectstate_cback(hilinkOnConnectionStateChange);
    mHilinkServer = HilinkServer::getInstance(this);
#endif
    mThirdPartyServer = ThirdPartyServer::getInstance(this);
}

BleProtocol::~BleProtocol() {
    mIsBinded = false;
    mConId = -1;
    mDBHelper = NULL;
    app_ble_server_register_write_cback(NULL);
    app_ble_server_register_connectstate_cback(NULL);
#ifdef BLUETOOTH_HILINK_SUPPORT
    mHilinkConId[0] = mHilinkConId[1] = mHilinkConId[2] = mHilinkConId[3] = -1;
    hilink_ble_server_register_write_cback(NULL);
    hilink_app_ble_server_register_connectstate_cback(NULL);
#endif

    if (mThirdPartyServer != NULL) {
        delete mThirdPartyServer;
        mThirdPartyServer = NULL;
    }
#ifdef BLUETOOTH_HILINK_SUPPORT
    if(mHilinkServer == NULL) {
        delete mHilinkServer;
        mHilinkServer = NULL;
    }
#endif
}

void BleProtocol::onConnectionStateChange(char *addr, int link_state) {
    if (sInstance == NULL)
        return;
    ALOGV("addr:%s,state:%d", addr, link_state);
    if (link_state == 0) {
        //the remote device is physically disconnected
        sInstance->unbond();
        setBTVisibility(true, true);
    } else {
        setBTVisibility(false, true);
    }
}
#ifdef BLUETOOTH_HILINK_SUPPORT
void BleProtocol::hilinkOnConnectionStateChange(char *addr, int link_state, int con_id, int server_num) {
    if (sInstance == NULL)
        return;
    ALOGV("addr:%s,state:%d,con_id:%d,server:%d", addr, link_state ,con_id,server_num);
    if (link_state == 0) {
        //the remote device is physically disconnected
        if((server_num>=0) && (server_num <=4))
            sInstance->mHilinkConId[server_num] = -1;
        //sInstance->unbond();
        setBTVisibility(true, true);
        hilinkReceiveWriteRequestCback(-1, nullptr, -1, "ble-close_evt-", strlen("ble-close_evt-"));//"BSA_BLE_SE_CLOSE_EVT"
    } else {
        if((server_num>=0) && (server_num <=4))
            sInstance->mHilinkConId[server_num] = con_id;
        setBTVisibility(false, true);
    }
    ALOGV("con_id:0->%d, 1->%d, 2->%d, 3->%d",sInstance->mHilinkConId[0],sInstance->mHilinkConId[1],sInstance->mHilinkConId[2],sInstance->mHilinkConId[3]);
}
void BleProtocol::hilinkReceiveWriteRequestCback(int server_no, char *address, int con_id, char *data, int size) {
    if (sInstance == NULL)
        return;
    //sInstance->mIsBinded = true;
    //sInstance->mConId = con_id;
    //sInstance->mBluetoothAddress = address;
    ALOGD("mConId:%d,con_id:%d!\n", sInstance->mConId, con_id);
    sInstance->mHilinkServer->onRetrive("h",(unsigned char*)data, size);
}
#endif
void BleProtocol::receiveWriteRequestCback(char *address, int con_id, char *data, int size) {
    if (sInstance == NULL)
        return;
    ALOGD("mConId:%d,con_id:%d!\n", sInstance->mConId, con_id);
    int length = 0;
    if (strncasecmp(data, BLE_BOND_REQ_PREFIX, strlen(BLE_BOND_REQ_PREFIX)) == 0) {
        //request bond
        ALOGD("recv phone bond request!");
        length = strlen(BLE_BOND_REQ_PREFIX);
        sInstance->processBindConnectRequest(address, con_id, length, data, true);
    } else if (strncasecmp(data, BLE_RECONN_REQ_PREFIX, strlen(BLE_RECONN_REQ_PREFIX)) == 0) {
        //request reconnect
        ALOGD("recv phone reconnect request!");
        length = strlen(BLE_RECONN_REQ_PREFIX);
        sInstance->processBindConnectRequest(address, con_id, length, data, false);
    } else if (strcmp(data, BLE_UNBOND_REQ) == 0) {
        ALOGD("recv phone unbond request !\n");
        if (sInstance->mConId == -1 || sInstance->mConId != con_id)
            return;
        sInstance->recvUnbond();
    } else if (strcmp(data, BLE_UNBOND_END_REQ) == 0) {
        if (sInstance->mConId == -1 || sInstance->mConId != con_id)
            return;
        sInstance->unbond();
    } else {
        if (sInstance->mConId == -1 || sInstance->mConId != con_id)
            return;
        //clivia if(size <=4 || strncmp(data, BLE_CONTACT_PKG,4) == 0)return;
        sInstance->processPackage(data, size);
    }
}

void BleProtocol::processPackage(char *data, int size) {
    char sync[8 + 1] = {0};
    if (size > 12) {
        ;
        memcpy(sync, data + 4, 8);
    }
    if (strcmp("syncdata", sync) == 0) {
        /// <150
        sInstance->parserPackage(data, size);
    } else if (size > 10) {
        // >=150 组包
        char crc[LONGDATA_CRC_LENGTH + 1];
        int len = size - LONGDATA_FLAG_LENGTH;
        char valid[len + 1];
        memcpy(crc, data, LONGDATA_CRC_LENGTH);
        crc[LONGDATA_CRC_LENGTH] = '\0';
        memcpy(valid, data + LONGDATA_FLAG_LENGTH, len);
        valid[len] = '\0';
        MapLongData::iterator it_find;
        it_find = mMapLongData.find(crc);
        if (it_find != mMapLongData.end()) {
            LongSyncData *longData = it_find->second;
            char *reData = longData->getReceiveData();
            int length = longData->getSize();
            memcpy(reData + length, valid, len);
            int alreadCount = longData->getReceiveCount() + 1;
            if (alreadCount == longData->getTotalCount()) {
                sInstance->parserPackage(reData, length + len);
                mMapLongData.erase(crc);
            } else {
                longData->setReceiveCount(alreadCount);
                longData->setSize(length + len);
            }
        } else {
            LongSyncData *longSyncData = new LongSyncData();
            char *reData = longSyncData->getReceiveData();
            memcpy(reData, valid, len);
            longSyncData->setTotalCount(data[LONGDATA_CRC_LENGTH] & 0xFF);
            longSyncData->setReceiveCount(1);
            longSyncData->setCrc32(crc);
            longSyncData->setSize(len);
            mMapLongData.insert(MapLongData::value_type(crc, longSyncData));
        }
    }
}

void BleProtocol::processBindConnectRequest(char *address, int con_id, int length, char *dataBuf, bool bindRequest) {
    //check bond state
    char remote_uuid[REMOTE_DEVICE_UUID_SIZE + 1];
    memset(remote_uuid, 0, REMOTE_DEVICE_UUID_SIZE + 1);
    memcpy(remote_uuid, dataBuf + length, REMOTE_DEVICE_UUID_SIZE);

    if (checkBindState(remote_uuid) == 1) {
        //bind success
        sendToSource(con_id, BLE_BOND_OK, strlen(BLE_BOND_OK));
        mIsBinded = true;
        mConId = con_id;
        mBluetoothAddress = address;
        //startModule();
        app_ble_wake_configure();
        mThirdPartyServer->BindedStateChange(true, true);
        setBTVisibility(false, true);
        ALOGI(" bind success remote_uuid=%s\n", remote_uuid);
    } else {
        //bind failed
        sendToSource(con_id, BLE_BOND_FAILED, strlen(BLE_BOND_FAILED));
        setBTVisibility(true, true);
    }
}

int BleProtocol::checkBindState(const char *remote_addr) {
    t_DB_CONFIG db_config;
    db_config.bind_state = 0;
    memset(db_config.bind_addr, 0, sizeof(db_config.bind_addr));
    mDBHelper->readStateFromDB(db_config);
    if (db_config.bind_state && strcmp(db_config.bind_addr, remote_addr)) {
        ALOGW("has already bind other device\n");
        return 0;
    }
    mDBHelper->writeStateToDB(true, remote_addr);
    return 1;
}

void BleProtocol::recvUnbond() {
    t_DB_CONFIG db_config;
    memset(db_config.bind_addr, 0, sizeof(db_config.bind_addr));
    sp<DBHelper> dBHelper = new DBHelper();
    dBHelper->readStateFromDB(db_config);
    if (strlen(db_config.bind_addr) != REMOTE_DEVICE_UUID_SIZE) {
        return;
    }

    sendToSource(mConId, BLE_UNBOND_RSP, strlen(BLE_UNBOND_RSP));
}

int BleProtocol::unbond() {
    ALOGV("ble unbond");
    //clivia system(SOUND_UNBIND);
    mIsBinded = false;
    mThirdPartyServer->BindedStateChange(false, true);
    app_ble_server_close();
    mDBHelper->writeStateToDB(false, "");
    mConId = -1;
    //stopModule();
    MapLongData::iterator it;
    for (it = mMapLongData.begin(); it != mMapLongData.end();) {
        mMapLongData.erase(it++);
    }
    return 0;
}

void BleProtocol::parserPackage(char *package, int size) {
    //deserialize
    sp<Header> header = new Header(package, size);
    switch (header->getType()) {
    case (Header::NORMAL_PKG): {
        string midName;
        header->getModule(midName);
        char *data = header->getData();
        int size = header->getPkgLen();
        moduleRetrive(midName.c_str(), data, size);
        break;
    }
    case (Header::CONTACT_PKG): {
        break;
    }
    default:
        ALOGE("Error:unkonw pkg type!");
    }
    header = NULL;
}

int BleProtocol::sendPacket(const string &moduleName, char *data, int size) {
    //serialize
    sp<Header> header = new Header(moduleName.c_str(), data, size);
    ;
    switch (header->getType()) {
    case (Header::NORMAL_PKG): {
        char *data = header->getData();
        int size = header->getPkgLen();
        sendToSource(mConId, data, size);
        break;
    }
    case (Header::CONTACT_PKG): {
        break;
    }
    default:
        ALOGE("Error:unkonw pkg type!");
    }
    header = NULL;
    return 0;
}
#ifdef BLUETOOTH_HILINK_SUPPORT
#define IDX_SEND_2_PHONE_SERVER 0
int BleProtocol::sendPacket(const string &moduleName, char *data, int size, bool ishilink) {
    int ret;
    //serialize
    if(mHilinkConId[IDX_SEND_2_PHONE_SERVER] == -1) {
        ALOGE("data size:%d,mHilinkConId[%d]:%d no hilink ble connected",size, IDX_SEND_2_PHONE_SERVER, mHilinkConId[IDX_SEND_2_PHONE_SERVER]);
        ret = -1;
    }else {
        ALOGD("data size:%d,mHilinkConId[%d]:%d", size, IDX_SEND_2_PHONE_SERVER, mHilinkConId[IDX_SEND_2_PHONE_SERVER]);
        ret = sendToSource(mHilinkConId[IDX_SEND_2_PHONE_SERVER], data, size);
    }
    return ret;
}
#endif
int BleProtocol::sendToSource(int con_id, char *data, int size) {
    if (con_id < 0)
        return -1;
    app_ble_server_send_indication(con_id, data, size);
}

bool BleProtocol::getBleBindedState() {
    return mIsBinded;
}

void BleProtocol::startModule() {
    //    mAboutModule = AboutModule::getInstance(this);
    //    mContactsModule = ContactsModule::getInstance(this);
    //    mControlModule = ControlModule::getInstance(this);
    //    mHeadsetModule = HeadsetModule::getInstance(this);
    //    mLanguageModule = LanguageModule::getInstance(this);
    //    mLiveModule = LiveModule::getInstance(this);
    //    mPhotoModule = PhotoModule::getInstance(this);
    //    mSettingModule = SettingModule::getInstance(this);
    //    mUpdateModule = UpdateModule::getInstance(this);
    //    mWifiModule = WifiModule::getInstance(this);
}

void BleProtocol::stopModule() {
    /*
    if(mAboutModule != NULL){
    	delete mAboutModule;
    	mAboutModule = NULL;
    }
    if(mContactsModule != NULL){
    	delete mContactsModule;
    	mContactsModule = NULL;
    }
    if(mControlModule != NULL){
    	delete mControlModule;
    	mControlModule = NULL;
    }
    if(mHeadsetModule != NULL){
    	delete mHeadsetModule;
    	mHeadsetModule = NULL;
    }
    if(mLanguageModule != NULL){
    	delete mLanguageModule;
    	mLanguageModule = NULL;
    }
    if(mLiveModule != NULL){
    	delete mLiveModule;
    	mLiveModule = NULL;
    }
    if(mPhotoModule != NULL){
    	delete mPhotoModule;
    	mPhotoModule = NULL;
    }
    if(mSettingModule != NULL){
    	delete mSettingModule;
    	mSettingModule = NULL;
    }
    if(mUpdateModule != NULL){
    	delete mUpdateModule;
    	mUpdateModule = NULL;
    }
    if(mWifiModule != NULL){
    	delete mWifiModule;
    	mWifiModule = NULL;
    }
*/
}

void BleProtocol::registerModule(const string &module_name, SyncModule *module) {
    mModuleMap.insert(MODULEMAP::value_type(module_name, module));
}

void BleProtocol::unRegisterModule(const string &module_name) {
    MODULEMAP::iterator it = mModuleMap.find(module_name);
    if (it == mModuleMap.end()) {
        ALOGD("no find this module\n");
        return;
    }
    it->second = NULL;
    mModuleMap.erase(it);
}

void BleProtocol::moduleRetrive(const char *moduleName, char *data, int size) {
    MODULEMAP::iterator it = mModuleMap.find(moduleName);
    if (it == sInstance->mModuleMap.end()) {
        //ALOGD("module no find this key\n");
        mThirdPartyServer->onRetrive(moduleName, data, size);
    } else {
        sp<SyncData> syncData = new SyncData();
        int ret = SyncDataTools::bytes2Data(syncData, data, size);

        if (ret < 0) {
            ALOGE("function=%s,Error:syncdata parser failed!\n", __FUNCTION__);
        } else {
            (it->second)->onRetrive(syncData);
        }
    }
}

} // namespace android
void BleProtocol_instance(){
	android::BleProtocol::getInstance();
}
