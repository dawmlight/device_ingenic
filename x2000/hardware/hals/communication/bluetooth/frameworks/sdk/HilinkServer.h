#ifndef HILINKSERVER_H_
#define HILINKSERVER_H_

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
class HilinkServer : public RefBase{
 public:
    typedef std::map<string, int> MODULEMAP;
    static HilinkServer* getInstance(RefBase* protocol);
    virtual ~HilinkServer();

    void onRetrive(const char * moduleName,unsigned char * data,unsigned int size);
    void BindedStateChange(bool isBinded,bool isBle);
 private:
    pthread_mutex_t msendLock;
    struct THREAD_ARG{
	HilinkServer* server;
	int client_fd;
    };

    static HilinkServer* sInstance;
    HilinkServer(RefBase* protocol);

    static void *SetBTVisibiltiy(void *me);
    static void *AcceptThreadWrapper(void *me);
    static void *ClientReadThreadWrapper(void *map);

    void AcceptThreadFunc();
    void ClientReadThreadFunc(int client_fd);

    void startSocketServer();
    void stopSocketServer();

    bool registerModule(string module,int client_fd);
    bool unRegisterModule(int client_fd);

    int sendTimes(string module,unsigned char* bytes,unsigned int size, int *used_size);
    void senddone(int client_fd,int ret);
    bool mDone;
    bool mIsBinded;
    int mListenfd;
    pthread_t mAcceptThread;
    pthread_t mReadThread;

    MODULEMAP mModuleMap;
    RefBase* mProtocol;
    BIND_TYPE mType; 
};
}
#endif  // HILINKSERVER_H_
