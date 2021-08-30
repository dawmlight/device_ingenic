/*
 * deserial data from bsa. and serial data to bsa
 *create by gysun in 2016.12.27
 */
#define LOG_NDEBUG 0
#define LOG_TAG "SyncDataTools"

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <utils/Log.h>
#include <utils/RefBase.h>

#include "SyncDataTools.h"

namespace android {
class SyncDataTools::Encoder {
public:
    static const int CAP = 4 * 1024;

    Encoder(const sp<SyncData>& data)
	:mPos(0),
	mSyncData(data){
    }

    int encode(char* byteData){
	mBytes = byteData;
	int keycount = mSyncData->keyCount();
	writeInt(keycount);

	SyncData::DATAMAP map = mSyncData->getDataMap();
	SyncData::DATAMAP::iterator iter;
	for(iter = map.begin(); iter != map.end(); ++iter){
	    writeString(iter->first);
	    writeObject(iter->second);
	}
	mBytes = NULL;
	return mPos;
    }

    ~Encoder(){
	delete []mBytes;
    }
private:
    sp<SyncData> mSyncData;
    char* mBytes;
    int mPos;

    void writeInt(int i){
	mBytes[mPos++] = (char) (i >> 24);
	mBytes[mPos++] = (char) (i >> 16);
	mBytes[mPos++] = (char) (i >> 8);
	mBytes[mPos++] = (char) i;
    }

    void writeString(const string & s){
	writeInt(s.size());
	memcpy(&mBytes[mPos],s.c_str(),s.size());
	mPos += s.size();
    }

    void writeString(const char * s,int size){
	string str;
	str.assign(s,size);
	writeInt(size);
	memcpy(&mBytes[mPos],s,size);
	mPos += size;
    }
    void writeObject(const sp<Object> & object){
	char type = (char)(object->getType());
	writeByte(type);
	if(type == BYTE){
	    writeByte((char)(*(object->getData())));
	}else if(type == INT){
		writeInt(*((int*)(object->getData())));
	}else if(type == STRING){
	    writeString(object->getData(),object->getDataSize());
	}else if(type == BOOL){
	    writeBoolean((bool)(*(object->getData())));
	}else if(type == LONG){
	    writeLong((*((int64_t *)object->getData())));
	}else if(type == BYTE_ARY){
	    writeByteArray((char*)object->getData(),object->getDataSize());
	}
    }
    void writeByte(char b) {
	mBytes[mPos++] = b;
    }
    void writeBoolean(bool b) {
	if (b)
	    mBytes[mPos++] = (char) 1;
	else
	    mBytes[mPos++] = (char) 0;
    }

    void writeLong(int64_t l){
       ALOGD("writeLong l =%lld\n", l);
       mBytes[mPos++] = (char) (l >> 56);
       mBytes[mPos++] = (char) (l >> 48);
       mBytes[mPos++] = (char) (l >> 40);
       mBytes[mPos++] = (char) (l >> 32);
       mBytes[mPos++] = (char) (l >> 24);
       mBytes[mPos++] = (char) (l >> 16);
       mBytes[mPos++] = (char) (l >> 8);
       mBytes[mPos++] = (char) l;
    }
    void writeByteArray(const char * s,int size){
	writeInt(size);
	strncpy(mBytes+mPos, s, size);
	mPos += size;
    }
};

class SyncDataTools::Decoder {
 public:
    static const int TRUE = 1;

    Decoder(char* data,int size)
	: mPos(0){
	mOneBytes = new char[size];
	memcpy(mOneBytes,data,size);
	datasize = size;
    }

	int decode(sp<SyncData> & data) {
		int keyCount = readInt();
		for (int i = 0; i < keyCount; i++) {
			string key;
			readString(key);
			int pos = mPos+1; //dispose a byte for type
			readObj();
			data->add(key, mOneBytes+pos,mPos-pos);
		}
		return mPos;
	}

    ~Decoder(){
	delete []mOneBytes;
    }
 private:
    char* mOneBytes;
	int datasize;
    int mPos = 0;

    void readObj(){
	char type = readByte();
        if (type == BYTE) {
	    mPos += 1;
	} else if (type == INT) {
	    mPos += INT_LENGTH;
	} else if (type == STRING) {
	    mPos += readInt();
	} else if (type == BOOL) {
	    mPos += 1;
	} else if (type == LONG) {
	    mPos += LONG_LENGTH;
	} else if (type == SHORT) {
	    mPos += SHORT_LENGTH;
	} else if (type == FLOAT) {
	    mPos += FLOAT_LENGTH;
	} else if (type == DOUBLE) {
	    mPos += DOUBLE_LENGTH;
	} else if (type == BYTE_ARY) {
	    mPos += readInt();
	}else {
	    ALOGE("unknow type:\n" + type);
	}
    }

    bool readBoolean() {
	if (mOneBytes[mPos++] == TRUE) {
	    return true;
	}
	return false;
    }

    int32_t readInt() {
	char* b = new char[INT_LENGTH];
	memcpy(b,mOneBytes+mPos,INT_LENGTH);

	int i = (b[0] << 24) & 0xff000000;
	i |= (b[1] << 16) & 0x00ff0000;
	i |= (b[2] << 8) & 0x0000ff00;
	i |= b[3] & 0xff;
	mPos += INT_LENGTH;
	delete []b;
	return i;
    }

    uint8_t readByte() {
	return mOneBytes[mPos++];
    }

     short readShort() {
	return (short) ((mOneBytes[mPos++] & 0xff << 8) | (mOneBytes[mPos++] & 0xff));
    }

    long readLong() {
	long temp = 0;
	long res = 0;
	for (int i = 0; i < LONG_LENGTH; i++) {
	    res <<= 8;
	    temp = mOneBytes[mPos++] & 0xff;
	    res |= temp;
	}
	return res;
    }

    void readString(string & str){
	int length = readInt();
	char* b = new char[length];
	memcpy(b,mOneBytes+mPos,length);
	mPos += length;
	  //str = string(b);
	str.assign(b,length);
	delete []b;
    }
};

int SyncDataTools::bytes2Data(sp<SyncData> & data,char* dataBytes,int size) {
	int rsize;//cursize = size,

	Decoder* decoder = new SyncDataTools::Decoder(dataBytes,size);
	rsize = decoder->decode(data);
    delete decoder;
    return rsize;
}

int SyncDataTools::data2Bytes(const sp<SyncData> & data,char* bytes){
    Encoder* encoder = new SyncDataTools::Encoder(data);
    int size = encoder->encode(bytes);
    delete encoder;
    return size;
}
}
