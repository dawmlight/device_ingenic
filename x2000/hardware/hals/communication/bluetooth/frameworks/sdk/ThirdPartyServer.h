#ifndef THIRDPARTYSERVER_H_
#define THIRDPARTYSERVER_H_

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <utils/RefBase.h>
#include "SyncData.h"  

namespace android {
/*enum BIND_TYPE{
	NONE,
	BLE,
	BLUETOOTH,
};*/
class ThirdPartyServer : public RefBase{
 public:
    typedef std::map<string, int> MODULEMAP;
    static ThirdPartyServer* getInstance(RefBase* protocol);
    virtual ~ThirdPartyServer();

    void onRetrive(const char * moduleName,char * data,int size);
    void BindedStateChange(bool isBinded,bool isBle);
    //string getModuleName(){return mModuleName;};
 private:
    pthread_mutex_t msendLock;
    struct THREAD_ARG{
	ThirdPartyServer* server;
	int client_fd;
    };

    static ThirdPartyServer* sInstance;
    ThirdPartyServer(RefBase* protocol);

    static void *SetBTVisibiltiy(void *me);
    static void *AcceptThreadWrapper(void *me);
    static void *ClientReadThreadWrapper(void *map);

    void AcceptThreadFunc();
    void ClientReadThreadFunc(int client_fd);

    void startSocketServer();
    void stopSocketServer();

    bool registerModule(string module,int client_fd);
    bool unRegisterModule(int client_fd);

    //bool send(string module,char* bytes,int size);
    bool sendTimes(string module,char* bytes,int size, int *used_size);
    bool senddone(int client_fd,int ret);
    //bool mNeedPrintData;
    bool mDone;
    bool mIsBinded;
    int mListenfd;
    pthread_t mAcceptThread;
    pthread_t mReadThread;

    MODULEMAP mModuleMap;
    //string mModuleName;
    //ThirdPartyServer* mModule;
    RefBase* mProtocol;
    BIND_TYPE mType; 
};
}
#endif  // THIRDPARTYSERVER_H_
