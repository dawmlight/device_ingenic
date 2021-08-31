
/*
 * deserial data from bsa. and serial data to bsa
 *create by gysun in 2016.12.27
 */
#define LOG_NDEBUG 0
#define LOG_TAG "SyncModule"

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
//#include <dlog.h>
#include <utils/Log.h>
#include <typeinfo>
#include "SyncModule.h"
#include "SyncDataTools.h"
#include "DBHelper.h"
#include "SppProtocol.h"
#include "BleProtocol.h"

namespace android {
static const int CAP = 4 * 1024;
#define REMOTE_DEVICE_UUID_SIZE 36

SyncModule::SyncModule(const string & name, RefBase* protocol)
{
    mModuleName = name;

    mModule = this;
    mModule->mProtocol = protocol;
    if(typeid(*protocol) == typeid(SppProtocol)){
	mType = 1;
	((SppProtocol *)protocol)->registerModule(mModuleName, mModule);
    }else if(typeid(*protocol) == typeid(BleProtocol)){
	mType = 2;
	((BleProtocol *)protocol)->registerModule(mModuleName, mModule);
    }
    printf("SyncModule:mModuleName=%s\n", mModuleName.c_str());
}

SyncModule::~SyncModule()
{
    if(mProtocol != NULL){
	if(mType == 1){
	    ((SppProtocol *)mProtocol)->unRegisterModule(mModuleName);
	}else if(mType == 2){
	    ((BleProtocol *)mProtocol)->unRegisterModule(mModuleName);
	}
    }
}

void SyncModule::onRetrive(const sp<SyncData> & data)
{
}

void SyncModule::send(const sp<SyncData> & data)
{
    char* bytes = new char[CAP];
    int size = SyncDataTools::data2Bytes(data,bytes);
    if(size <= 0){
    	ALOGE("Error:serial failed!");
    	return;
    }

    if(mProtocol != NULL){
	if(mType == 1){
	    ((SppProtocol *)mProtocol)->sendPacket(mModuleName.c_str(),bytes,size);
	}else if(mType == 2){
	    ((BleProtocol *)mProtocol)->sendPacket(mModuleName.c_str(),bytes,size);
	}
    }

    delete []bytes;
}

int SyncModule::isBleBinded()
{
    t_DB_CONFIG db_config;
    memset(db_config.bind_addr, 0, sizeof(db_config.bind_addr));
    sp<DBHelper> dBHelper = new DBHelper();
    dBHelper->readStateFromDB(db_config);
    if(strlen(db_config.bind_addr) == REMOTE_DEVICE_UUID_SIZE) {
	    printf("ble binded\n");
	    return 1;
    }
    dBHelper = NULL;
    return 0;
}

} //namespace android
