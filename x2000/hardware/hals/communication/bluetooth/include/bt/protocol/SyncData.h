#ifndef SYNCDATA_H_
#define SYNCDATA_H_

#include <stdio.h>
#include <map>
#include <string>
#include <utils/RefBase.h>
#include "Type.h"

using  std::string;
namespace android {
enum BIND_TYPE{
	NONE,
	BLE,
	BLUETOOTH,
};
class Object : public RefBase{
public:

    /*deserial*/
    Object(char* value,int size):
	mSize(size){
	    if(mSize == 0){
		mData = "";
	    }else{
		mData = new char[size];
		memcpy(mData,value,size);
	    }
    }

     /*serial*/
     Object(int type,const char* value,int size):
	mType(type),
	mSize(size){
	    if(mSize == 0){
		mData = "";
	    }else{
		mData = new char[size];
		memcpy(mData,value,size);
	    }
    }

    ~Object(){
	if(mSize != 0)
		delete []mData;
    }
    char* getData(){
	return mData;
    }
    int getDataSize(){
	return mSize;
    
    }

    int getType(){
	return mType;
    }
private:
    char* mData;
    int mSize;
    int mType;
};

class SyncData : public RefBase {
 public:
    typedef std::map<string,sp<Object> > DATAMAP;

    SyncData();

    void add(const string& key, char* values,int size);
    int8_t getBoolean(const string & key);
    int32_t getInt(const string & key);
    int32_t getString(const string & key,string & values);
    int32_t getByteArray(const string & key,char* value);
      /***********serial**********/
    int keyCount(){
	return mValues.size();
    }
    void putBoolean(const string & key,bool value);
    void putInt(const string & key,int value);
    void putLong(const string & key, int64_t value);
    void putString(const string & key,const string & value);
    void putByteArray(const string & key,char* value,int size);

    DATAMAP getDataMap(){
	return mValues;
    }

    ~SyncData();

 private:
    class Decoder;
    class Encoder;
    DATAMAP mValues; 
    int32_t readInt(char* b);
    int8_t  readBoolean(char* b);

    void put(const string & key, int type,const char* values,int size);

};
};
#endif  // SYNCDATA_H_
