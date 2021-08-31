#ifndef LONG_SYNC_DATA_H_
#define LONG_SYNC_DATA_H_

#include <stdio.h>
#include <string>

using  std::string;
namespace android {
class LongSyncData {
public:
    LongSyncData(){
	    mTotalCount = 0;
	    mReceiveCount = 0;
	    mCrc32 = "";
	    memset(mReceiveData, 0, 1000);
    }
    virtual ~LongSyncData(){
    }
    int getTotalCount(){
	    return mTotalCount;
    }
    void setTotalCount(int totalCount){
	    mTotalCount = totalCount;
    }
    int getReceiveCount(){
	    return mReceiveCount;
    }
    void setReceiveCount(int receiveCount){
	    mReceiveCount = receiveCount;
    }
    int getSize(){
	    return mSize;
    }
    void setSize(int size){
	    mSize = size;
    }
    string getCrc32(){
	    return mCrc32;
    }
    void setCrc32(string crc){
	    mCrc32 = crc;
    }
    char* getReceiveData(){
	    return mReceiveData;
    }
private:
    int mTotalCount;    //分成几个包
    int mReceiveCount;   //已接收包数
    int mSize;          //已接收的有效数据长度
    char mReceiveData[1000]= {0};    //已接收的有效数据
    string mCrc32;      //crc标志，8位，区分不同的数据命令.
};
}  // namespace android

#endif  // LONG_SYNC_DATA_H_
