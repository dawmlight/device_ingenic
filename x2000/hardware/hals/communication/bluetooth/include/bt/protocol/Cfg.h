#ifndef CFG_H_
#define CFG_H_

#include <stdio.h>
#include <string>
#include <utils/RefBase.h>

namespace android {
    using  std::string;
    class Cfg  : public RefBase{
    public:
	static const int MAX_LEN = 36;
	static const int MODULE_LEN = 15;
	static const int UUID_LEN = 16;
	static const int FLAG_SERVICE = 1 << 5;
	static const int FLAG_REPLY = 1 << 4;
	static const int FLAG_MID = 1 << 3;
	static const int FLAG_PROJO = 1 << 2;

	static const int POS_MOST_SIG_BITS = 20;
	static const int POS_LEAST_SIG_BITS = 28;

	Cfg(char* datas,int size) {
	    mDatas = new char[size];
	    memcpy(mDatas,datas,size);
	    memset(mMidName,0,MODULE_LEN);
	    memcpy(mMidName,mDatas+1,MODULE_LEN);
	}

	static int Cfg2Bytes(int priority,bool service,bool reply,bool mid,bool projo,string midName,int datalength,string uuid,char* bytes){
	    bytes[0] = (priority && 0xff)<< 6;
	    bytes[0] |= (service && 0xff) << 5;
	    bytes[0] |= (reply && 0xff) << 4;
	    bytes[0] |= (mid && 0xff) << 3;
	    bytes[0] |= (projo && 0xff) << 2;

	    memcpy(&bytes[1], midName.c_str(), strlen(midName.c_str()));

	    bytes[16] = (char)(datalength >> 24);
	    bytes[17] = (char)(datalength >> 16);
	    bytes[18] = (char)(datalength >> 8);
	    bytes[19] = (char)(datalength);

	      /*skip UUID*/
	      //memcpy(&bytes[20],midName.c_str(),UUID_LEN);
	}

	~Cfg(){
	    delete []mDatas;
	}

	int getPriority() {
	    return (mDatas[0] & 0xff) >> 6;
	}

	bool isService() {
	    return (mDatas[0] & FLAG_SERVICE) > 0;
	}

	bool isReply() {
	    return (mDatas[0] & FLAG_REPLY) > 0;
	}

	bool isProjo() {
	    return (mDatas[0] & FLAG_PROJO) > 0;
	}

	bool isMid() {
	    return (mDatas[0] & FLAG_MID) > 0;
	}

	void getModule(string & module) {
	    module.assign(mMidName);
	      //return string(mMidName);
	}

    private:
	char * mDatas;
	char mMidName[MODULE_LEN];
    };
}
#endif  // CFG_H_
