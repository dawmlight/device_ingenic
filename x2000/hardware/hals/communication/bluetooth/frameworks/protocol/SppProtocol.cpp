/*
 * deserial data from bsa. and serial data to bsa
 *create by gysun in 2016.12.27
 */
#define LOG_NDEBUG 0
#define LOG_TAG "SppProtocol"

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <iostream>
#include <utils/Log.h>
#include <SettingsDB.h>

//#define SOUND_UNBIND "/usr/bin/MediaPlayer /etc/sound/unbind.mp3 &"

#ifdef __cplusplus
extern "C"
{
#endif
extern int setBTVisibility(bool discoverable, bool connectable);

/*from app_dg.c*/
extern int app_dg_send(int con_id, char* data, int size);
extern int app_dg_register_data_cback(void *user, void (*receiveSppDataCback)(void *user, int conn_id, char * data, int size));
extern int app_dg_register_link_state_cback(void *user, void (*linkStateCback)(void *user,int con_id, bool linked,char * remote_addr, int size));
void SppProtocol_instance();
#ifdef __cplusplus
}
#endif

#include "SppProtocol.h"
#include "SyncDataTools.h"
#include "UnBindModule.h"

//#include "Neg.h"
#define CONFIG_SIZE 36
#define NEG_SIZE 2
#define HEADER_SIZE 2
#define EXTRA_HEADER_SIZE 16
#define MAC_ADDRESS_SIZE 6
using namespace std;
namespace android {

SppProtocol* SppProtocol::sInstance = NULL;

/*for unbind module*/
static const string MODULE_UNBIND = "system_module";
static const string UNBIND_TYPE = "unbind";
static const string MODULE_SYSTEM = "SYSTEM";

class Neg : public RefBase{
public:
      /*reason*/
    static const int SUCCESS = 0;
    static const int FAIL_ADDRESS_MISMATCH = 1;
    static const int FAIL_VERSION_MISMATCH = 2;

      /*******for serialize******/
    Neg(bool pass, int reason) {
	mACK2 = true;
	mPass = pass;
	mReason = reason;
    }

    void toBytes(char* buffer) {
	buffer[0] = (char) mACK2 << 6; //0x40 means ACK2
	if (mPass) {
	    buffer[1] = (char) (1 << 7);
	} else {
	    buffer[1] = (char) mReason;
	}
    }

      /*******for deserialize******/
    Neg(char* data) {
	mACK2 = (data[0] & 0x40);
	mPass = mACK2 && (((data[1] & 0xff) >> 7) == 1);
	mReason = mACK2 ? (data[1] & 0x7f) : -1;
    }

    bool isACK2() {
	return mACK2;
    }
    bool isPass() {
	return mPass;
    }

    char* getReason() {
	if(mReason == FAIL_ADDRESS_MISMATCH)
	    return "address mismatch";
	else if(mReason == FAIL_VERSION_MISMATCH)
	    return "version mismatch";
	else
	    return "success";
    }

    ~Neg(){
    }

private:
    bool mACK2;
    bool mPass;
    int mReason;
};

class Header : public RefBase{
public:
      /***********type**********/
    static const int PKG  = 0x0;
    static const int CFG = 0x1;
    static const int NEG = 0x2;

    /*******for deserialize******/
    Header (char *data) {
	char a = data[0];
	char b = data[1];
	mPkgLen = ((a & 0x3f) << 8) | (b & 0xff);
	mType = (a & (0xc0)) >> 6;
    }

    int getType(){
	return mType;
    }

    int getPkgLen(){
	return mPkgLen;
    }

    /*******for serialize******/
    Header (int type, int len) {
	mType = type;
	mPkgLen = len;
    }

    void toBytes(char* buffer) {
	int i = (mType << 14) | mPkgLen;
	char a = (char) (i >> 8);
	char b = (char) i;
	buffer[0] = a;
	buffer[1] = b;
    }

    ~Header(){
    }

private:
    int mType;
    int mPkgLen;
};

class ExtraHeader : public RefBase{
public:
    static const int TYPE_USER_DATA  = 0;
    static const int TYPE_ACK = 1;

    static const int MEMBER_NUM = 3;
    static const int BYTES_PER_MEMBER = 4;

    /*******for deserialize******/
    ExtraHeader (char* bytes) {
	char* headerData = bytes+BYTES_PER_MEMBER; //skip header len
	int* mem = new int[MEMBER_NUM];
	for (int i = 0; i < MEMBER_NUM; i++) {
	    mem[i] = readInt(headerData, BYTES_PER_MEMBER*i);
	}

	mmDataType = mem[0];
	mmSerialNum = mem[1];
	mmDataLen = mem[2];
	delete []mem;
    }

    int getDataLen(){
	return mmDataLen;
    }

    int getDataType(){
	return mmDataType;
    }

    /*******for serialize******/
    ExtraHeader (int type, int num, int dataLen) {
	mmDataType = type;
	mmSerialNum = num;
	mmDataLen = dataLen;
    }

    void toBytes(char* buffer) {
	writeInt(buffer, BYTES_PER_MEMBER*0, MEMBER_NUM*BYTES_PER_MEMBER); // write header len
	writeInt(buffer, BYTES_PER_MEMBER*1, mmDataType);
	writeInt(buffer, BYTES_PER_MEMBER*2, mmSerialNum);
	writeInt(buffer, BYTES_PER_MEMBER*3, mmDataLen);
    }

    ~ExtraHeader(){
    }

private:
    int mmDataType;
    int mmSerialNum;
    int mmDataLen;

    static int readInt(char* buf, int offset) {
	int val = ((buf[offset]&0xff)
		   | ((buf[offset+1]&0xff) << 8)
		   | ((buf[offset+2]&0xff) << 16)
		   | ((buf[offset+3]&0xff) << 24));
	return val;
    }

    static void writeInt(char* buf, int offset, int value) {
	buf[offset] = (char) (value);
	buf[offset+1] = (char) (value >> 8);
	buf[offset+2] = (char) (value >> 16);
	buf[offset+3] = (char) (value >> 24);
    }

};

SppProtocol::SppProtocol()
    : mCfgWaitData(false),
      mConId(-1),
      mLeftLength(0),
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
      //mWifiModule(NULL),
      mPowerStorageModule(NULL),
*/
      mThirdPartyServer(NULL)
      {
    mIsBinded = false;
    mLeftData = new char[2048]();
    mDBHelper = new DBHelper();

    UnBindModule::getInstance(this);

    app_dg_register_data_cback(this,receiveWriteRequestCback);
    app_dg_register_link_state_cback(this,linkStateCback);

    mThirdPartyServer = ThirdPartyServer::getInstance(this);
}

SppProtocol::~SppProtocol() {
    mDBHelper = NULL;
    app_dg_register_data_cback(this,NULL);
    app_dg_register_link_state_cback(this,NULL);
    //stopModule();
    delete []mLeftData;
    if(mThirdPartyServer != NULL){
	delete mThirdPartyServer;
    	mThirdPartyServer = NULL;
    }
}

SppProtocol* SppProtocol::getInstance()
{
    if(sInstance == NULL){
	    sInstance = new SppProtocol();
    }
    return sInstance;
}

void SppProtocol::processBindRequest(int con_id, char * remote_addr, int size){
    int length = EXTRA_HEADER_SIZE+HEADER_SIZE+NEG_SIZE;
    char *buf = new char[length];
    sp<Neg> neg ;
    if(!checkBindState(remote_addr)){
	    /*has aleady bonded a addr*/
	    sp<ExtraHeader> extraHeader = new ExtraHeader(ExtraHeader::TYPE_USER_DATA,1,HEADER_SIZE+NEG_SIZE);
	    sp<Header> header = new Header(Header::NEG,NEG_SIZE);
	    neg = new Neg(false,Neg::FAIL_ADDRESS_MISMATCH);
	    extraHeader->toBytes(buf);
	    header->toBytes(buf+EXTRA_HEADER_SIZE);
	    neg->toBytes(buf+EXTRA_HEADER_SIZE+HEADER_SIZE);
	    sendToSource(mConId, buf, length);
	    ALOGW("has already bind other device");
    }else{
	ALOGI(" bind success remote_addr=%s",remote_addr);
	    sp<ExtraHeader> extraHeader = new ExtraHeader(ExtraHeader::TYPE_USER_DATA,1,HEADER_SIZE+NEG_SIZE);
	    sp<Header> header = new Header(Header::NEG,NEG_SIZE);
	    neg = new Neg(true,0);
	    extraHeader->toBytes(buf);
	    header->toBytes(buf+EXTRA_HEADER_SIZE);
	    neg->toBytes(buf+EXTRA_HEADER_SIZE+HEADER_SIZE);

	    mIsBinded = true;
	    mThirdPartyServer->BindedStateChange(true,false);
	    mConId = con_id;
	    mLeftLength = 0;
	    sendToSource(mConId, buf, length);
	    setBTVisibility(false, true);
	    //startModule();
    }
    delete []buf;
}

int SppProtocol::checkBindState(const char *remote_addr)
{
    t_DB_CONFIG db_config;
    db_config.bind_state = 0;
    memset(db_config.bind_addr, 0, sizeof(db_config.bind_addr));
    mDBHelper->readStateFromDB(db_config);
    if (db_config.bind_state && strncmp(db_config.bind_addr, remote_addr, MAC_ADDRESS_SIZE)) {
	    ALOGD("has already bind other device\n");
	    return 0;
    }
    mDBHelper->writeStateToDB(true, remote_addr);
    return 1;
}

void SppProtocol::linkStateCback(void *user,int con_id, bool linked,char * remote_addr, int size){
    SppProtocol *spp = (SppProtocol *) user;
    ALOGV("linkStateCback linked=%d",linked);
    if(linked){
	  //receive a bind request from a mobilephone
	spp->processBindRequest(con_id, remote_addr, size);
    }else{
	  // the remote device is physically disconnected
	if(spp->mIsBinded)
	    spp->resetBindState();
    }
}

void SppProtocol::receiveWriteRequestCback(void *user, int con_id, char * data, int size){
    SppProtocol *spp = (SppProtocol *) user;
    ALOGV("SppProtocol mConId=%d,con_id:%d,size:%d\n",spp->mConId,con_id,size);

    if(size < EXTRA_HEADER_SIZE){
	ALOGE("receive data is invalid,drop it!");
    }else{
	spp->parserExtraHeader(data,size);
    }
}

void SppProtocol::parserExtraHeader(char *buf,int size) {
	int length = 0;
	int index = 0;
	char *databuf = new char[mLeftLength + size]();
	if(mLeftLength > 0){
		memcpy(databuf, mLeftData, mLeftLength);
		memcpy(databuf + mLeftLength, buf, size);
		length = mLeftLength + size;
		mLeftLength = 0;
	}else {
		memcpy(databuf, buf, size);
		length = size;
	}
	for(;length > EXTRA_HEADER_SIZE;){
	    char extraHeaderBuf[EXTRA_HEADER_SIZE]={0};
		memcpy(extraHeaderBuf, databuf + index, EXTRA_HEADER_SIZE);
		sp<ExtraHeader> extraHeader = new ExtraHeader(extraHeaderBuf);
		int dataLen = extraHeader->getDataLen();
		if(length - EXTRA_HEADER_SIZE >= dataLen){
			index += EXTRA_HEADER_SIZE;
			parserPackageHeader(databuf + index,dataLen);
			index += dataLen;
			length = length - EXTRA_HEADER_SIZE - dataLen;
		}else {
			memset(mLeftData, 0, strlen(mLeftData));
			memcpy(mLeftData, databuf + index, length);
			break;
		}
		extraHeader = NULL;
	}
	mLeftLength = length;
	delete []databuf;
}

void SppProtocol::parserPackageHeader(char *package,int size) {
    sp<Header> header = new Header(package);
    char* data = package + HEADER_SIZE;
    int dataSize = size - HEADER_SIZE;

    switch(header->getType()){
    case (Header::NEG):{
	  /*just for spp connect ACK action*/
    	sp<Neg> neg = new Neg(data);
    	if(neg->isACK2() == false){
    	    ALOGE("Error:no support no-ACK2 Neg now!\n");
    	    return;
    	}
    	if(neg->isPass()){
    	    ALOGD("connect success!\n");
    	}else{
    	    ALOGE("connect failed. reason=%s\n",neg->getReason());
    	}
	  //delete neg;
	break;
    }
    case (Header::CFG):{
	mCfg = new Cfg(data,dataSize);
	mCfgWaitData = true;
	break;
    }
    case (Header::PKG):{
	if(mCfgWaitData == false){
	    ALOGE("Error:no cfg data.lost package.\n");
	    return;
	}
	mCfgWaitData = false;

	string midName ;
	mCfg->getModule(midName);
	if(midName.compare(MODULE_SYSTEM)==0){
		ALOGE("SYSTEM ignore !\n");
		return;
	}
	ALOGD("parserPackageHeader midName =%s",midName.c_str());
	moduleRetrive(midName.c_str(),data,dataSize);
	mCfg = NULL;
	break;
    }
    default:
	ALOGE("Error:unkonw pkg type!\n");
    }
}

int SppProtocol::sendPacket(const string & moduleName,char* data,int size){
    ALOGV("SppProtocol::sendPacket in size=%d\n",size);
    if(!mIsBinded || size <= 0)return -1;
     /*send header+config*/
    int headerSize = EXTRA_HEADER_SIZE+HEADER_SIZE;
    int length = EXTRA_HEADER_SIZE+HEADER_SIZE+CONFIG_SIZE;
    char* buf = new char[length]();
    sp<ExtraHeader> extraHeader = new ExtraHeader(ExtraHeader::TYPE_USER_DATA,1,HEADER_SIZE+CONFIG_SIZE);
    sp<Header> header = new Header(Header::CFG,CONFIG_SIZE);
    extraHeader->toBytes(buf);
    header->toBytes(buf+EXTRA_HEADER_SIZE);

    Cfg::Cfg2Bytes(CHANNEL_SPEICAL,false,false,false,false,moduleName,size,"",buf+headerSize);

    sendToSource(mConId, buf, length);
    delete []buf;
    header = NULL;
    extraHeader=NULL;

    /*send header+data*/
    length = EXTRA_HEADER_SIZE+HEADER_SIZE+size;
    buf = new char[length]();
    extraHeader = new ExtraHeader(ExtraHeader::TYPE_USER_DATA,1,HEADER_SIZE+size);
    header = new Header(Header::PKG,size);
    extraHeader->toBytes(buf);
    header->toBytes(buf+EXTRA_HEADER_SIZE);
    memcpy(&buf[headerSize],data,size);
    sendToSource(mConId, buf, length);
    delete []buf;
    if(strcmp(MODULE_UNBIND.c_str(), moduleName.c_str()) == 0){
	mIsBinded = false;
	mThirdPartyServer->BindedStateChange(false,false);
	//stopModule();
    }
    return 0;
}

int SppProtocol::sendToSource(int con_id, char* data, int size)
{
    if(mIsBinded == false){
	ALOGW("send data failed.device is unbind now");
	return -1;
    }

    if(con_id < 0) return -1;
    return app_dg_send(con_id, data, size);
}

bool SppProtocol::getSppBindedState(){
    return mIsBinded;
}

void SppProtocol::resetBindState()
{
    sp<SyncData> data = new SyncData();
    data->putBoolean(UNBIND_TYPE, true);
    UnBindModule *module = UnBindModule::getInstance(this);
    module->send(data);
    mDBHelper->writeStateToDB(false, "");
    setBTVisibility(true, true);
    //stopModule();
    mIsBinded = false;
}

void SppProtocol::disconnect()
{
    ALOGV("disconnect");
    //system(SOUND_UNBIND);
    resetBindState();
}

void SppProtocol::startModule(){
///    mAboutModule = AboutModule_Android::getInstance(this);
//    mContactsModule = ContactsModule::getInstance(this);
//    mControlModule = ControlModule::getInstance(this);
//    mHeadsetModule = HeadsetModule::getInstance(this);
//    mLanguageModule = LanguageModule::getInstance(this);
//    mLiveModule = LiveModule::getInstance(this);
//    mPhotoModule = PhotoModule::getInstance(this);
//    mSettingModule = SettingModule_Android::getInstance(this);
//    mUpdateModule = UpdateModule::getInstance(this);
//    mWifiModule = WifiModule::getInstance(this);
//    mPowerStorageModule = PowerStorageModule::getInstance(this);
}

void SppProtocol::stopModule(){
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
    if(mPowerStorageModule != NULL){
	delete mPowerStorageModule;
    	mPowerStorageModule = NULL;
    }
*/
}

void SppProtocol::registerModule(const string & module_name, SyncModule* module){
    mModuleMap.insert(MODULEMAP::value_type(module_name, module));
}

void SppProtocol::unRegisterModule(const string & module_name){
    MODULEMAP::iterator it= mModuleMap.find(module_name);
    if(it == mModuleMap.end()) {
	ALOGE("no find this module\n");
	return;
    }
    it->second = NULL;
    mModuleMap.erase(it);
}

void SppProtocol::moduleRetrive(const char * moduleName,char * data,int size){
    MODULEMAP::iterator it= mModuleMap.find(moduleName);
    if(it == sInstance->mModuleMap.end()) {
	mThirdPartyServer->onRetrive(moduleName,data,size);

    }else {
	sp<SyncData> syncData = new SyncData();
	int ret = SyncDataTools::bytes2Data(syncData,data,size);

	if(ret < 0){
	    ALOGE("function=%s,Error:syncdata parser failed!\n",__FUNCTION__);
	}else{
	    (it->second)->onRetrive(syncData);
	}
    }
}

}  // namespace android
void SppProtocol_instance(){
	android::SppProtocol::getInstance();
}
