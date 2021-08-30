/*
 * deserial data from bsa. and serial data to bsa
 *create by gysun in 2016.12.27
 */
#define LOG_NDEBUG 0
#define LOG_TAG "SyncData"

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <utils/Log.h>

#include "SyncData.h"
#define ALOGD printf
#define ALOGE printf
#define ALOGV printf
namespace android {

SyncData::SyncData(){

}

SyncData::~SyncData(){
    ALOGD("~SyncData\n");
    DATAMAP::iterator iter;
    for(iter = mValues.begin(); iter != mValues.end(); ++iter){
	iter->second = NULL;
	mValues.erase(iter);
    }
}

void SyncData::add(const string & key, char* values,int size) {
    sp<Object> obj = new Object(values,size);
    mValues.insert(DATAMAP::value_type(key, obj));
    ALOGD("SyncData : key=%s value size=%d\n",key.c_str(),size);
}

int8_t SyncData::getBoolean(const string & key){
    DATAMAP::iterator it= mValues.find(key);

    if(it == mValues.end()) {
	ALOGE("no find this key\n");
	return -1;
    }

    return readBoolean((it->second)->getData());
}

int32_t SyncData::getInt(const string & key){
    DATAMAP::iterator it= mValues.find(key);

    if(it == mValues.end()) {
	ALOGE("no find this key\n");
	return -1;
    }

    return readInt((it->second)->getData());
}

int32_t SyncData::getString(const string & key,string & values){
    DATAMAP::iterator it= mValues.find(key);
    if(it == mValues.end()) {
	ALOGE("no find this key\n");
	return -1;
    }
    char* data = (it->second)->getData();
    int32_t length = readInt(data);
    char* str = new char[length];
    memcpy(str,data+4,(it->second)->getDataSize()-4);
      //values = string(str);
    values.assign(str,length);
    delete []str;
    return 0;
}

int32_t SyncData::getByteArray(const string & key,char* value){
    DATAMAP::iterator it= mValues.find(key);
    if(it == mValues.end()) {
	ALOGE("no find this key\n");
	return -1;
    }
    char* data = (it->second)->getData();
    int32_t length = readInt(data);
    memcpy(value,data+4,length);
    return length;
}

int32_t SyncData::readInt(char* b){
    int32_t i = (b[0] << 24) & 0xff000000;
    i |= (b[1] << 16) & 0x00ff0000;
    i |= (b[2] << 8) & 0x0000ff00;
    i |= b[3] & 0xff;
    return i;
}

int8_t SyncData::readBoolean(char* b){
    return *b;
}

void SyncData::put(const string & key, int type,const char* values,int size) {
    sp<Object> obj = new Object(type,values,size);
    mValues.insert(DATAMAP::value_type(key, obj));
}

void SyncData::putBoolean(const string & key,bool value){
    put(key,BOOL,(char*)&value,1);
}
void SyncData::putInt(const string & key,int value){
    put(key,INT,(char*)(&value),4);
}

void SyncData::putLong(const string & key, int64_t value){
    put(key,LONG,(char*)(&value),8);
}

void SyncData::putString(const string & key,const string & value){
    put(key,STRING,value.c_str(),value.size());
}

void SyncData::putByteArray(const string & key,char* value,int size){
    put(key,BYTE_ARY,value,size);
}
}
