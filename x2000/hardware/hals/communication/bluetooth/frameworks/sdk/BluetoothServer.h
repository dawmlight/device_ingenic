#ifndef BLUETOOTHSERVER_H_
#define BLUETOOTHSERVER_H_

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <utils/List.h>
#include <utils/RefBase.h>
#include "SyncData.h"  
#include "app_disc_cback.h"

namespace android {

class BluetoothServer : public RefBase{
 public:
    static BluetoothServer* getInstance();
    virtual ~BluetoothServer();

 private:
    BluetoothServer();
    struct THREAD_ARG{
	BluetoothServer* server;
	int client_fd;
    };

    static BluetoothServer* sInstance;
    BluetoothServer(RefBase* protocol);

    static void *AcceptThreadWrapper(void *me);
    static void *ClientReadThreadWrapper(void *map);

    void AcceptThreadFunc();
    void ClientReadThreadFunc(int client_fd);

    void startSocketServer();
    void stopSocketServer();

    void openBluetooth();
    void closeBluetooth();
    void setBleBeacon(int state);
	void printPacketInfo(const char* title, const char* buf, int size,bool realy);
    void setBluetoothVisibility(bool enable);
    void disableBluetooth();
    void hsAnswerCall();
    void hsHangUp();
    void creatBond(const char* bd_name);
    void removeBond(const char* bd_name);
    void startDiscovery(int client_fd, int duration, int type);
    void stopDiscovery(int client_fd);
    void sendDiscDataToClient(int event, tAPP_DISC_MSG* p_data);
    static void app_disc_cback(int event, tAPP_DISC_MSG* p_data);

    void getLinkedDevice(int client_fd, int duration);
    static void app_getlinked_cback(int event, tAPP_DISC_MSG* p_data);
    void sendLinkedDeviceDataToClient(int event, tAPP_DISC_MSG* p_data);

    void getLinkingDevice(int client_fd, int duration);
    static void app_getlinking_cback(int event, tAPP_DISC_MSG* p_data);
    void sendLinkingDeviceDataToClient(int event, tAPP_DISC_MSG* p_data);

    void avSetNotify(int client_fd, int notify);
    static void app_av_notify_cback(int state);

    bool mDone;
    bool mDiscovering;
    int mListenfd;
    pthread_t mAcceptThread;
    pthread_t mReadThread;

    pthread_cond_t mCondition;
    pthread_mutex_t mMutex;
    pthread_mutex_t mDiscLock;
    pthread_mutex_t mBondLock;

    List<int> mDiscoveryManager;

    bool mGettingLinkedDevice;
    pthread_mutex_t mGetLinkedDeviceLock;
    List<int> mGetLinkedDeviceManager;

    bool mGettingLinkingDevice;
    pthread_mutex_t mGetLinkingDeviceLock;
    List<int> mGetLinkingDeviceManager;

    int mNotifyClientfd;

};
}
#endif  // BLUETOOTHSERVER_H_
