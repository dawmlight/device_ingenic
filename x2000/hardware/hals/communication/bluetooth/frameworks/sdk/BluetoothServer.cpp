//#define LOG_NDEBUG 0
#define LOG_TAG "BluetoothServer"
//#include <dlog.h>
#include <signal.h>
#include <utils/Log.h>
//#include <monitor/ProcessMonitor.h>
#include "SppProtocol.h"
#include "BleProtocol.h"
#include "BluetoothServer.h"
#include "BluetoothUtils.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>


#define UNIX_DOMAIN "/var/run/bluetooth_socket"
#define BUFFER_SIZE 4096

#ifdef __cplusplus
extern "C"
{
#endif
#include "app_av.h"
#ifdef BLUETOOTH_AG_SUPPORT
#include "app_ag.h"
#endif
#ifdef BLUETOOTH_HILINK_SUPPORT
//extern int setBTBleBeacon(int state);
#endif
extern int setBTVisibility(bool discoverable, bool connectable);
extern int setBTEnable(bool enable);
extern int app_hs_answer_call();
extern int app_hs_hangup();
extern int app_mgr_start_discovery(void (*appDiscCback)(int event, tAPP_DISC_MSG* p_data), int duration, int type);
extern int app_disc_abort();
extern int app_mgr_sec_bond(const char* bd_name);
extern int app_mgr_sec_unpair(const char* bd_name);
extern int app_mgr_get_linked_devices(void (*appGetLinkedCback)(int event, tAPP_DISC_MSG* p_data), int duration);
extern int app_mgr_get_linking_devices(void (*appGetLinkingCback)(int event, tAPP_DISC_MSG* p_data), int duration);
extern int app_mgr_del_linked_devices(const BD_ADDR addr);
void BluetoothServer_instance();

#ifdef __cplusplus
}
#endif

namespace android {
BluetoothServer* BluetoothServer::sInstance = NULL;
BluetoothServer* BluetoothServer::getInstance()
{
    ALOGD("%s", __FUNCTION__);
    if(sInstance == NULL){
	    sInstance = new BluetoothServer();
    }
    return sInstance;
}

BluetoothServer::BluetoothServer(){
    mDone = false;
    startSocketServer();
    pthread_mutex_init(&mDiscLock, NULL);
    pthread_mutex_init(&mMutex, NULL);
    pthread_mutex_init(&mBondLock, NULL);
    pthread_cond_init(&mCondition,NULL);
    mGettingLinkedDevice = false;
    mDiscovering = false;
    pthread_mutex_init(&mGetLinkedDeviceLock,NULL);
    pthread_mutex_init(&mGetLinkingDeviceLock,NULL);
    mGettingLinkingDevice = false;
    mNotifyClientfd = -1;
}

BluetoothServer::~BluetoothServer()
{
    stopSocketServer();
    pthread_cond_destroy(&mCondition);
    pthread_mutex_destroy(&mMutex);
    pthread_mutex_destroy(&mDiscLock);
    pthread_mutex_destroy(&mBondLock);

    pthread_mutex_destroy(&mGetLinkedDeviceLock);
    pthread_mutex_destroy(&mGetLinkingDeviceLock);
}

void BluetoothServer::openBluetooth(){
    ALOGV("%s", __FUNCTION__);
    pthread_mutex_lock(&mMutex);
    setBTEnable(true);
    pthread_mutex_unlock(&mMutex);
}

void BluetoothServer::closeBluetooth(){
    ALOGV("%s", __FUNCTION__);
    pthread_mutex_lock(&mMutex);
    setBTEnable(false);
    pthread_mutex_unlock(&mMutex);
}

void BluetoothServer::setBluetoothVisibility(bool enable){
    ALOGV("%s", __FUNCTION__);
    pthread_mutex_lock(&mMutex);
    setBTVisibility(enable,true);
    pthread_mutex_unlock(&mMutex);
}

void BluetoothServer::setBleBeacon(int state){
    ALOGD("%s", __FUNCTION__);
    pthread_mutex_lock(&mMutex);
#ifdef BLUETOOTH_HILINK_SUPPORT
    //setBTBleBeacon(state);
#endif
    pthread_mutex_unlock(&mMutex);
}

void BluetoothServer::disableBluetooth(){
    ALOGV("%s", __FUNCTION__);
    pthread_mutex_lock(&mMutex);
    BleProtocol* ble = BleProtocol::getInstance();
    SppProtocol* spp = SppProtocol::getInstance();

    if(spp->getSppBindedState()){
	spp->disconnect();
    }else if(ble->getBleBindedState()){
	ble->recvUnbond();
    }else{
	ALOGI("bluetooth is disable already");
    }
    pthread_mutex_unlock(&mMutex);
}

void BluetoothServer::hsAnswerCall(){
    ALOGV("%s", __FUNCTION__);
    app_hs_answer_call();
}

void BluetoothServer::hsHangUp(){
    ALOGV("%s", __FUNCTION__);
    app_hs_hangup();
}

void BluetoothServer::creatBond(const char* bd_name){
    ALOGV("%s name = %s", __FUNCTION__, bd_name);
    pthread_mutex_lock(&mBondLock);
    app_mgr_sec_bond(bd_name);
    pthread_mutex_unlock(&mBondLock);
}

void BluetoothServer::removeBond(const char* bd_name){
    ALOGV("%s name = %s", __FUNCTION__, bd_name);
    pthread_mutex_lock(&mBondLock);
    app_mgr_sec_unpair(bd_name);
    pthread_mutex_unlock(&mBondLock);
}

void BluetoothServer::sendLinkedDeviceDataToClient(int event, tAPP_DISC_MSG* p_data){
    if (event == APP_DISC_NEW_EVT) {
    int msg_len;
	int head_len = strlen("linked-");
	char msg[head_len+BD_ADDR_SIZE+BD_NAME_SIZE];
	memcpy(&msg[0],"linked-",head_len);
	msg[head_len++] = p_data->disc_new.bd_type;
	memcpy(&msg[head_len],p_data->disc_new.bd_addr,BD_ADDR_SIZE);
	msg_len = head_len+BD_ADDR_SIZE;
	memcpy(&msg[head_len+BD_ADDR_SIZE],p_data->disc_new.name,strlen(p_data->disc_new.name)+1);
	msg_len += strlen(p_data->disc_new.name)+1;

	List<int>::iterator it;
	for (it = mGetLinkedDeviceManager.begin(); it != mGetLinkedDeviceManager.end(); ++it){
	    ::send(*it, msg, msg_len, 0); // p_data->disc_new.bd_addr maybe has element '00', strlen(msg) maybe error.
	    ::sync();
	}
    } else if(event == APP_DISC_CMPL_EVT) {
	pthread_mutex_lock(&mGetLinkedDeviceLock);
	mGettingLinkedDevice = false;
	pthread_mutex_unlock(&mGetLinkedDeviceLock);
	string msg = "linkedcmpl-";
	List<int>::iterator it;
	for (it = mGetLinkedDeviceManager.begin(); it != mGetLinkedDeviceManager.end(); ++it){
	    ::send(*it,msg.c_str(),msg.length(),0);
	    ::sync();
	}
	mGetLinkedDeviceManager.clear();
    }
}

void BluetoothServer::app_getlinked_cback(int event, tAPP_DISC_MSG* p_data){
    switch (event)
	{
	    /* a New Device has been discovered */
	case APP_DISC_NEW_EVT:{
	    BluetoothServer *server = static_cast<BluetoothServer *>(BluetoothServer::getInstance());
	      //server->sendDiscDataToClient(event, p_data->disc_new.name, p_data->disc_new.bd_addr);
	    server->sendLinkedDeviceDataToClient(event, p_data);
	    break;
	}
	case APP_DISC_CMPL_EVT: {/* Discovery complete. */
	    BluetoothServer *server = static_cast<BluetoothServer *>(BluetoothServer::getInstance());
	      //server->sendDiscDataToClient(event, NULL, NULL);
	    server->sendLinkedDeviceDataToClient(event, NULL);
	    break;
	}
	default:
	    ALOGE("app_getlinked_cback unknown event:%d", event);
	    break;
	}
}

void BluetoothServer::getLinkedDevice(int client_fd, int duration)
{
    pthread_mutex_lock(&mGetLinkedDeviceLock);
    //ALOGE("%s mGettingLinkedDevice = %d", __FUNCTION__, mGettingLinkedDevice);
    if (mGettingLinkedDevice) {
	mGetLinkedDeviceManager.push_back(client_fd);
	pthread_mutex_unlock(&mGetLinkedDeviceLock);
	return;
    }

//	ALOGE("%s LINE = %d", __FUNCTION__, __LINE__);
	mGetLinkedDeviceManager.push_back(client_fd);
    mGettingLinkedDevice = true;
    pthread_mutex_unlock(&mGetLinkedDeviceLock);
    app_mgr_get_linked_devices(app_getlinked_cback, duration);
    //mGettingLinkedDevice = false;

}

void BluetoothServer::sendLinkingDeviceDataToClient(int event, tAPP_DISC_MSG* p_data){
    if (event == APP_DISC_NEW_EVT) {
    int msg_len;
	int head_len = strlen("linking-");
	char msg[head_len+BD_ADDR_SIZE+BD_NAME_SIZE];
	memcpy(&msg[0],"linking-",head_len);
	msg[head_len++] = p_data->disc_new.bd_type;
	memcpy(&msg[head_len],p_data->disc_new.bd_addr,BD_ADDR_SIZE);
	msg_len = head_len+BD_ADDR_SIZE;
	memcpy(&msg[head_len+BD_ADDR_SIZE],p_data->disc_new.name,strlen(p_data->disc_new.name)+1);
	msg_len += strlen(p_data->disc_new.name)+1;

	List<int>::iterator it; int foriii=0;
	for (it = mGetLinkingDeviceManager.begin(); it != mGetLinkingDeviceManager.end(); ++it){
	    ::send(*it, msg, msg_len, 0); // p_data->disc_new.bd_addr maybe has element '00', strlen(msg) maybe error.
	    ::sync();
	}
    } else if(event == APP_DISC_CMPL_EVT) {
	pthread_mutex_lock(&mGetLinkingDeviceLock);
	mGettingLinkingDevice = false;
	pthread_mutex_unlock(&mGetLinkingDeviceLock);
	string msg = "linkingcmpl-";
	List<int>::iterator it;
	for (it = mGetLinkingDeviceManager.begin(); it != mGetLinkingDeviceManager.end(); ++it){
	    ::send(*it,msg.c_str(),msg.length(),0);
	    ::sync();
	}
	mGetLinkingDeviceManager.clear();
    }
}

void BluetoothServer::app_getlinking_cback(int event, tAPP_DISC_MSG* p_data){
    switch (event)
	{
	    /* a New Device has been discovered */
	case APP_DISC_NEW_EVT:{
	    BluetoothServer *server = static_cast<BluetoothServer *>(BluetoothServer::getInstance());
	      //server->sendDiscDataToClient(event, p_data->disc_new.name, p_data->disc_new.bd_addr);
	    server->sendLinkingDeviceDataToClient(event, p_data);
	    break;
	}
	case APP_DISC_CMPL_EVT: {/* Discovery complete. */
	    BluetoothServer *server = static_cast<BluetoothServer *>(BluetoothServer::getInstance());
	      //server->sendDiscDataToClient(event, NULL, NULL);
	    server->sendLinkingDeviceDataToClient(event, NULL);
	    break;
	}
	default:
	    ALOGE("app_getlinking_cback unknown event:%d", event);
	    break;
	}
}

void BluetoothServer::getLinkingDevice(int client_fd, int duration)
{
    pthread_mutex_lock(&mGetLinkingDeviceLock);
    //ALOGE("%s mGettingLinkingDevice = %d", __FUNCTION__, mGettingLinkingDevice);
    if (mGettingLinkingDevice) {
	mGetLinkingDeviceManager.push_back(client_fd);
	pthread_mutex_unlock(&mGetLinkingDeviceLock);
	return;
    }

//	ALOGE("%s LINE = %d", __FUNCTION__, __LINE__);
	mGetLinkingDeviceManager.push_back(client_fd);
    mGettingLinkingDevice = true;
    pthread_mutex_unlock(&mGetLinkingDeviceLock);
    app_mgr_get_linking_devices(app_getlinking_cback, duration);
    //mGettingLinkingDevice = false;

}
/**
 * state 1: start_evt, 0:stop_evt
 **/
void BluetoothServer::app_av_notify_cback(int state) {
    BluetoothServer *server = static_cast<BluetoothServer *>(BluetoothServer::getInstance());
    if(server->mNotifyClientfd > 0) {
    	string msg = "a2dp-av-notify-";
        msg.append(std::to_string(state));
        ALOGD("server send %s,state:%d",msg.c_str(),state);
	    ::send(server->mNotifyClientfd, msg.c_str(), msg.length(), 0);
	    ::sync();
    }
}
void BluetoothServer::avSetNotify(int client_fd, int notify) {
    pthread_mutex_lock(&mDiscLock);
    if(notify){
        mNotifyClientfd = client_fd;
        app_av_set_notify_cb(app_av_notify_cback);
    } else {
        app_av_set_notify_cb(NULL);
        mNotifyClientfd = -1;
    }
	pthread_mutex_unlock(&mDiscLock);
}
void BluetoothServer::startDiscovery(int client_fd, int duration, int type){
    ALOGV("%s default duration = %d", __FUNCTION__, duration);
    pthread_mutex_lock(&mDiscLock);
    if (mDiscovering) {
	mDiscoveryManager.push_back(client_fd);
	pthread_mutex_unlock(&mDiscLock);
	return;
    }

    app_mgr_start_discovery(app_disc_cback, duration, type);
    mDiscoveryManager.push_back(client_fd);
    mDiscovering = true;

    pthread_mutex_unlock(&mDiscLock);
}

void BluetoothServer::stopDiscovery(int client_fd){
    ALOGV("%s", __FUNCTION__);
    pthread_mutex_lock(&mDiscLock);
    app_disc_abort();
    mDiscoveryManager.clear();
    mDiscovering = false;
    pthread_mutex_unlock(&mDiscLock);
}


void BluetoothServer::app_disc_cback(int event, tAPP_DISC_MSG* p_data){
    switch (event)
	{
	    /* a New Device has been discovered */
	case APP_DISC_NEW_EVT:{
	    BluetoothServer *server = static_cast<BluetoothServer *>(BluetoothServer::getInstance());
	      //server->sendDiscDataToClient(event, p_data->disc_new.name, p_data->disc_new.bd_addr);
	    server->sendDiscDataToClient(event, p_data);
	    break;
	}
	case APP_DISC_CMPL_EVT: {/* Discovery complete. */
	    BluetoothServer *server = static_cast<BluetoothServer *>(BluetoothServer::getInstance());
	      //server->sendDiscDataToClient(event, NULL, NULL);
	    server->sendDiscDataToClient(event, NULL);
	    break;
	}
	default:
	    ALOGE("app_disc_cback unknown event:%d", event);
	    break;
	}
}

/*
static inline void dump_app_disc_msg(const char * data, int len)
{
	char buf[512];
	ALOGD("%s() %p: %d\n", __func__, data, len);
	memset(&buf[0], 0, 512);
	char *b = (char *)&buf[0];
	while (len--) {
		sprintf(b, "%02x", (unsigned char)*data++);
		b += 2;
	}
	ALOGD("\t%s", &buf[0]);
}
*/

void BluetoothServer::sendDiscDataToClient(int event, tAPP_DISC_MSG* p_data){
    if (event == APP_DISC_NEW_EVT) {

	int msg_len;
	int head_len = strlen("discovery-");
	char msg[head_len+BD_ADDR_SIZE+BD_NAME_SIZE];

	memcpy(&msg[0],"discovery-",head_len);
	msg[head_len++] = p_data->disc_new.bd_type;
	memcpy(&msg[head_len],p_data->disc_new.bd_addr,BD_ADDR_SIZE);
	msg_len = head_len+BD_ADDR_SIZE;
	memcpy(&msg[head_len+BD_ADDR_SIZE],p_data->disc_new.name,strlen(p_data->disc_new.name)+1);
	msg_len += strlen(p_data->disc_new.name)+1;

	/*ALOGD("head_len+BD_ADDR_SIZE+BD_NAME_SIZE: %d, msg_len=%d", head_len+BD_ADDR_SIZE+BD_NAME_SIZE, msg_len);
	dump_app_disc_msg(p_data->disc_new.bd_addr, 10);
	dump_app_disc_msg(p_data->disc_new.name, 100);
	ALOGD("p_data->disc_new.name: %s", p_data->disc_new.name);*/
	/* bd_addr: 00:23:01:24:b7:5c */

	List<int>::iterator it; int foriii=0;
	for (it = mDiscoveryManager.begin(); it != mDiscoveryManager.end(); ++it){
	    ::send(*it, msg, msg_len, 0); // p_data->disc_new.bd_addr maybe has element '00', strlen(msg) maybe error.
	    ::sync();
	}
    } else if(event == APP_DISC_CMPL_EVT) {
	pthread_mutex_lock(&mDiscLock);
	mDiscovering = false;
	pthread_mutex_unlock(&mDiscLock);
	string msg = "disccmpl-";
	List<int>::iterator it;
	for (it = mDiscoveryManager.begin(); it != mDiscoveryManager.end(); ++it){
	    ::send(*it,msg.c_str(),msg.length(),0);
	    ::sync();
	}
	mDiscoveryManager.clear();
    }
}

void BluetoothServer::stopSocketServer(){
    void *dummy;
    mDone = true;
    pthread_join(mAcceptThread, &dummy);
    close(mListenfd);
}

void BluetoothServer::startSocketServer(){
    struct sockaddr_un ser_addr;
    ser_addr.sun_family = AF_UNIX;
    strncpy(ser_addr.sun_path,UNIX_DOMAIN,sizeof(ser_addr.sun_path)-1);
    unlink(UNIX_DOMAIN);//delete already exist point
    mListenfd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);//clivia add  | SOCK_CLOEXEC 180429
    if(mListenfd < 0){
        ALOGE("error: socket creation failed; skt_fd:%d\n", mListenfd);
        return;
    }
    if(bind(mListenfd, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) < 0){
    	ALOGE(" bind error: %s(errno: %d)\n",strerror(errno),errno);
	::close(mListenfd);
	return;
    }
    if(listen(mListenfd, 1) < 0){
        ALOGE("error: socket listen failed\n");
        ::close(mListenfd);
        return;
    }

    pthread_create(&mAcceptThread, NULL, BluetoothServer::AcceptThreadWrapper, this);
}

void *BluetoothServer::AcceptThreadWrapper(void *me) {
    BluetoothServer *source = static_cast<BluetoothServer *>(me);
    source->AcceptThreadFunc();
    return NULL;
}

void *BluetoothServer::ClientReadThreadWrapper(void *arg) {
    THREAD_ARG *args = (THREAD_ARG *)arg;
    BluetoothServer *server = static_cast<BluetoothServer *>(args->server);
    server->ClientReadThreadFunc(args->client_fd);
    return NULL;
}

void BluetoothServer::AcceptThreadFunc(){
    char buf[BUFFER_SIZE];
    int readLen = 0;
    int count = 0;
    while (mDone == false){
	int client_fd = accept(mListenfd, NULL, NULL);
	if(client_fd < 0){
		ALOGE("cannot accept client connect request");
		continue;
	}
	THREAD_ARG arg;
	arg.server = this;
	arg.client_fd = client_fd;
	pthread_create(&mReadThread, NULL, BluetoothServer::ClientReadThreadWrapper, &arg);
    }
}
void BluetoothServer::printPacketInfo(const char* title, const char* buf, int size,bool realy)
{
	if(/*mNeedPrintData || */realy)
	{
	    int j,i,k;
	    char prtinfo[512],pr2[256];

	    if(realy)
            ALOGE(" -%s-size=%d--- -start-------",title,size);
        else
            ALOGD(" -%s-size=%d--- -start-------",title,size);
	    for(k=i=0; i<size;k++)
	    {
	        prtinfo[0]=0;
	        pr2[0] = 0;
	        for(j=0; (i<size && j<16); j++,i++)
	        {
	           	sprintf(pr2+j, "%c", isgraph(buf[i]) ? buf[i] : ' ');
	           	sprintf(prtinfo+strlen(prtinfo), "%02X, ", ((unsigned int)buf[i])&0xff);
	        }
            if(realy)
                ALOGE("%3x| %s -- %s",k,prtinfo,pr2);
            else
                ALOGD("%3x| %s -- %s",k,prtinfo,pr2);
	    }
        if(realy)
            ALOGE(" -%s-size=%d--end----------",title,size);
        else
            ALOGD(" -%s-size=%d--end----------",title,size);
	}
}


/**/
void BluetoothServer::ClientReadThreadFunc(int client_fd_arg){
    char recv_buf[BUFFER_SIZE];
    int count = 1;
	int client_fd = client_fd_arg;
    //ALOGD("\n\n---start ClientReadThreadFunc client_fd=%d\n",client_fd);
    while (1){
		if(client_fd <= 0)
		{
			usleep(10*1000);
			client_fd = accept(mListenfd, NULL, NULL);
			if(client_fd < 0){
				ALOGE("wait new mListenfd");
				continue;
			}else
				ALOGD("get new mListenfd=%d", mListenfd);
		}

    while(1){
	memset(recv_buf, 0 ,BUFFER_SIZE);
	count = recv(client_fd, recv_buf, BUFFER_SIZE, MSG_NOSIGNAL);
	if(count <= 0){
		ALOGE("socket is close ! error=%s(%d)",strerror(errno),errno);
		close(client_fd);
		client_fd = -1;
		break;
	}else {
#if 1
		char *recv_ptr;
		bool printflag;
		int bkcount = count;

		printflag = false;
		recv_ptr = recv_buf;
        printPacketInfo("server read msg", recv_buf, bkcount, true);
		do
		{
			if(memcmp(recv_ptr, "open-", strlen("open-")) == 0)
			{
				int msglen = strlen("open-");
				openBluetooth();
				string msg = "open-ok";
				::send(client_fd,msg.c_str(),msg.length(),0);
				count -= msglen;
				recv_ptr += msglen;
			}else if(memcmp(recv_ptr, "close-", strlen("close-")) == 0)
			{
				int msglen = strlen("close-");
				closeBluetooth();
				string msg = "close-ok";
				::send(client_fd,msg.c_str(),msg.length(),0);
				count -= msglen;
				recv_ptr += msglen;
			}else if(memcmp(recv_ptr, "hwbeacon-", strlen("hwbeacon-")) ==0) {
                int msglen = strlen("hwbeacon-");
	            char *ptr = recv_ptr + msglen;
	            if(strncmp(ptr,"1",1)==0){
                    msglen +=1;
	                setBleBeacon(1);//on
	            }else if(strncmp(ptr,"0",1)==0){
                    msglen += 1;
	                setBleBeacon(0);//off
	            }else if(strncmp(ptr,"2",1)==0){
                    msglen += 1;
	                setBleBeacon(2);//unreg
	            }else if(strncmp(ptr,"3",1)==0){
                    msglen += 1;
	                setBleBeacon(3);//reg
	            }else if(strncmp(ptr,"4",1)==0){
                    msglen += 1;
	                setBleBeacon(4);//重设广播间隔时间
	            }
				count -= msglen;
				recv_ptr += msglen;

			}else if(memcmp(recv_ptr, "visibility-" ,strlen("visibility-")) == 0)
			{
				int msglen;
				if(memcmp(recv_ptr, "visibility-true", strlen("visibility-true")) == 0)
				{
					setBluetoothVisibility(true);
					msglen = strlen("visibility-true");
				}
				else
				{
					setBluetoothVisibility(false);
					msglen = strlen("visibility-false");
				}
				count -= msglen;
				recv_ptr += msglen;
			}else if(memcmp(recv_ptr, "disable-", strlen("disable-")) == 0)
			{
				int msglen = strlen("disable-");
				disableBluetooth();
				string msg = "disable-ok";
				::send(client_fd,msg.c_str(),msg.length(),0);
				count -= msglen;
				recv_ptr += msglen;
			}else if(memcmp(recv_ptr, "answercall-", strlen("answercall-")) == 0)
			{
				int msglen = strlen("answercall-");
				hsAnswerCall();
				count -= msglen;
				recv_ptr += msglen;
			}else if(memcmp(recv_ptr, "hangup-", strlen("hangup-")) == 0)
			{
				int msglen = strlen("hangup-");
				hsHangUp();
				count -= msglen;
				recv_ptr += msglen;
			}else if(memcmp(recv_ptr, "getlinking-", strlen("getlinking-")) == 0)
			{
				int msglen = strlen("getlinking-");
                #if 0
				string duration;
				//int head_len = strlen("getlinking-");
				duration.assign(recv_ptr+msglen,count-msglen);
				getLinkingDevice(client_fd, atoi(duration.c_str()));
                #else
                    char ibuf[2];
                    ibuf[0] = recv_ptr[msglen];
                    ibuf[1] = 0;
                    int duration = atoi(ibuf);
                    msglen ++;
				getLinkingDevice(client_fd, duration);
                #endif
                ALOGD("getlinking- duration:%d,msglen:%d\n",duration,msglen);
				count -= msglen;
				recv_ptr += msglen;
			}else if(memcmp(recv_ptr, "getlinked-", strlen("getlinked-")) ==0)
			{
				int msglen = strlen("getlinked-");
                #if 0
				string duration;
				//int head_len = strlen("getlinked-");
				duration.assign(recv_ptr+msglen,count-msglen);
				getLinkedDevice(client_fd, atoi(duration.c_str()));
                #else
                    char ibuf[2];
                    ibuf[0] = recv_ptr[msglen];
                    ibuf[1] = 0;
                    int duration = atoi(ibuf);
                    msglen ++;
				getLinkedDevice(client_fd, duration);
                #endif
                ALOGD("getlinked- duration:%d,msglen:%d\n",duration,msglen);
				count -= msglen;
				recv_ptr += msglen;
			}else if(memcmp(recv_ptr, "dellinked-", strlen("dellinked-")) == 0)
			{
				int msglen = strlen("dellinked-");
				BD_ADDR addr;
				memcpy(&addr,&recv_ptr[msglen],BD_ADDR_SIZE);
				msglen += BD_ADDR_SIZE;
				int ret = app_mgr_del_linked_devices(addr);
				if(ret == 0)
				::send(client_fd,"dellinked-ok",strlen("dellinked-ok"),0);
				else
				::send(client_fd,"dellinked-failed",strlen("dellinked-failed"),0);
				count -= msglen;
				recv_ptr += msglen;
			}else if(memcmp(recv_ptr, "startdisc-", strlen("startdisc-")) == 0)
			{   //startdisc-4-0-
				int msglen = strlen("startdisc-");
                int duration=8,type=0;
				//string duration;
                char *ptr=recv_ptr+strlen("startdisc-");
                duration = atoi(ptr);
                ptr = strchr(ptr,'-');
                if(ptr){
                    ptr ++;
                    type = atoi(ptr);
                }
                msglen = ptr - recv_ptr + 2;
                ALOGD("startdisc-%d-%d-,msglen=%d",duration,type,msglen);
                startDiscovery(client_fd, duration, type);

				count -= msglen;
				recv_ptr += msglen;
			}else if(memcmp(recv_ptr, "stopdisc-", strlen("stopdisc-")) == 0)
			{
				int msglen = strlen("stopdisc-");
				stopDiscovery(client_fd);
				count -= msglen;
				recv_ptr += msglen;
			}else if(memcmp(recv_ptr, "creatbond-", strlen("creatbond-")) == 0)
			{
				int msglen = strlen("creatbond-");
				string name;
				//int head_len = strlen("creatbond-");
				name.assign(recv_ptr+msglen,count-msglen);
				creatBond(name.c_str());
				count -= msglen;
				recv_ptr += msglen;
			}else if(memcmp(recv_ptr, "removebond-", strlen("removebond-")) == 0)
			{
				int msglen = strlen("removebond-");
				string name;
				//int head_len = strlen("removebond-");
				name.assign(recv_ptr+msglen,count-msglen);
				removeBond(name.c_str());
				count -= msglen;
				recv_ptr += msglen;
			}else if(memcmp(recv_ptr, "a2dp-source-open-", strlen("a2dp-source-open-")) == 0)
			{
				int msglen = strlen("a2dp-source-open-");
				BD_ADDR addr;
				memcpy(&addr,&recv_ptr[msglen],BD_ADDR_SIZE);
				msglen += BD_ADDR_SIZE;

				int ret = app_av_open(&addr ); //,1 clivia
				if(ret == 0)
				::send(client_fd,"a2dp-source-open-ok",strlen("a2dp-source-open-ok"),0);
				else
				{
					//app_av_restart();
					::send(client_fd,"a2dp-source-open-failed",strlen("a2dp-source-open-failed"),0);
				}
				count -= msglen;
				recv_ptr += msglen;
			}
        #ifdef BLUETOOTH_AG_SUPPORT
            else if(memcmp(recv_ptr, "ag-source-open-",strlen("ag-source-open-")) == 0) {
                int msglen = strlen("ag-source-open-");
                BD_ADDR addr;
                memcpy(&addr,&recv_ptr[msglen],BD_ADDR_SIZE);
                msglen += BD_ADDR_SIZE;

                int ret = app_ag_open(&addr);
                if(ret == 0)
                ::send(client_fd,"ag-source-open-ok",strlen("ag-source-open-ok"),0);
                else
                ::send(client_fd,"ag-source-open-failed",strlen("ag-source-open-failed"),0);
                count -= msglen;
                recv_ptr += msglen;

            }else if(memcmp(recv_ptr, "ag-source-close-", strlen("ag-source-close-")) == 0) {
                int msglen = strlen("ag-source-close-");
                int ret = app_ag_close();

                if(ret == 0)
                ::send(client_fd,"ag-source-close-ok",strlen("ag-source-close-ok"),0);
                else
                ::send(client_fd,"ag-source-close-failed",strlen("ag-source-close-failed"),0);
                count -= msglen;
                recv_ptr += msglen;
            }else if(memcmp(recv_ptr, "ag-open-audio-", strlen( "ag-open-audio-")) == 0) {
                int msglen = strlen("ag-open-audio-");
                app_ag_open_audio();
                //clivia ::send(client_fd,"ag-open-audio-ok",strlen("ag-open-audio-ok"),0);
                count -= msglen;
                recv_ptr += msglen;
            }else if(memcmp(recv_ptr, "ag-close-audio-" , strlen("ag-close-audio-")) == 0) {
                int msglen = strlen("ag-close-audio-");
                app_ag_close_audio();
                //clivia ::send(client_fd,"ag-close-audio-ok",strlen("ag-close-audio-ok"),0);
                count -= msglen;
                recv_ptr += msglen;
            }
        #endif
            else if(memcmp(recv_ptr, "a2dp-source-close-",strlen("a2dp-source-close-")) == 0)
			{
				int msglen = strlen("a2dp-source-close-");
				int ret = app_av_close();

				if(ret == 0)
				::send(client_fd,"a2dp-source-close-ok",strlen("a2dp-source-close-ok"),0);
				else
				{
					//app_av_restart();
					::send(client_fd,"a2dp-source-close-failed",strlen("a2dp-source-close-failed"),0);
				}
				count -= msglen;
				recv_ptr += msglen;
            }else if(memcmp(recv_ptr, "a2dp-av-socket-ready-", strlen("a2dp-av-socket-ready-"))== 0) {
                int msglen = strlen("a2dp-av-socket-ready-");
                ALOGD("a2dp-av-socket-ready-: app_av_socket_connect");
                app_av_socket_connect();
				count -= msglen;
				recv_ptr += msglen;
			}else if(memcmp(recv_ptr, "a2dp-av-send-volume-", strlen("a2dp-av-send-volume-")) == 0) {
				int msglen = strlen("a2dp-av-send-volume-");
				int ret, volume;

				volume = atoi(&recv_ptr[msglen]);
				ALOGD("a2dp-av-send-volume-: %d", volume);
				ret = app_av_rc_send_absolute_volume_vd_command(0, volume);
/* no need send msg back
				if(ret == 0)
				::send(client_fd,"a2dp-av-send-volume-ok",strlen("a2dp-av-send-volume-ok"),0);
				else
				::send(client_fd,"a2dp-av-send-volume-failed",strlen("a2dp-av-send-volume-failed"),0);
*/

				count -= msglen+1;
				recv_ptr += msglen+1;
            }else if(memcmp(recv_ptr, "a2dp-av-notify-", strlen("a2dp-av-notify-")) == 0) {
				int msglen = strlen("a2dp-av-notify-");
				int state;

				if('0' == recv_ptr[msglen])
					state = 0;
				else
					state = 1;
                //todo: set notify here
                avSetNotify(client_fd, state);
				count -= msglen+1;
				recv_ptr += msglen+1;
            }else if(memcmp(recv_ptr, "a2dp-av-check_connect-", strlen("a2dp-av-check_connect-")) == 0) {
				int msglen = strlen("a2dp-av-check_connect-");
				int state = app_av_is_connect();
                if(state)
                    ::send(client_fd, "a2dp-av-check_connect-y-", strlen("a2dp-av-check_connect-y-"), 0);
                else
                    ::send(client_fd, "a2dp-av-check_connect-n-", strlen("a2dp-av-check_connect-n-"), 0);
				count -= msglen;
				recv_ptr += msglen;
			}else{
				ALOGE("no valid data be received!");
                if(printflag == false)
                {
					printPacketInfo("orignal msg", recv_buf, bkcount, true);
                    printPacketInfo("missed BluetoothServer::ClientReadThreadFunc msg", recv_ptr, count, true);
                    printflag = true;
                }
				count --;
				recv_ptr ++;
			}

		}while(count > 0);
#else
		if(strstr(recv_buf, "open-") == &recv_buf[0]){
		    openBluetooth();
		    string msg = "open-ok";
		    ::send(client_fd,msg.c_str(),msg.length(),0);
		}else if(strstr(recv_buf, "close-") == &recv_buf[0]){
		    closeBluetooth();
		    string msg = "close-ok";
		    ::send(client_fd,msg.c_str(),msg.length(),0);
		}else if(strstr(recv_buf, "visibility-") == &recv_buf[0]){
		    if(strstr(recv_buf, "true") != NULL)
			setBluetoothVisibility(true);
		    else
			setBluetoothVisibility(false);
		}else if(strstr(recv_buf, "disable-") == &recv_buf[0]){
		    disableBluetooth();
		    string msg = "disable-ok";
		    ::send(client_fd,msg.c_str(),msg.length(),0);
		}else if(strstr(recv_buf, "answercall-") == &recv_buf[0]){
		    hsAnswerCall();
		}else if(strstr(recv_buf, "hangup-") == &recv_buf[0]){
		    hsHangUp();
		}else if(strstr(recv_buf, "startdisc-") == &recv_buf[0]) {
		    string duration;
		    int head_len = strlen("startdisc-");
		    duration.assign(recv_buf+head_len,count-head_len);
		    startDiscovery(client_fd, atoi(duration.c_str()));
		}else if(strstr(recv_buf, "stopdisc-") == &recv_buf[0]) {
		    stopDiscovery(client_fd);
		}else if(strstr(recv_buf, "creatbond-") == &recv_buf[0]) {
		    string name;
		    int head_len = strlen("creatbond-");
		    name.assign(recv_buf+head_len,count-head_len);
		    creatBond(name.c_str());
		}else if(strstr(recv_buf, "removebond-") == &recv_buf[0]) {
		    string name;
		    int head_len = strlen("removebond-");
		    name.assign(recv_buf+head_len,count-head_len);
		    removeBond(name.c_str());
		}else if(strstr(recv_buf, "a2dp-source-open-") == &recv_buf[0]) {
		    BD_ADDR addr;
		    memcpy(&addr,&recv_buf[strlen("a2dp-source-open-")],BD_ADDR_SIZE);

		    int ret = app_av_open(&addr);
		    if(ret == 0)
			::send(client_fd,"a2dp-source-open-ok",strlen("a2dp-source-open-ok"),0);
		    else
			::send(client_fd,"a2dp-source-open-failed",strlen("a2dp-source-open-failed"),0);

		}else if(strstr(recv_buf, "a2dp-source-close-") == &recv_buf[0]) {
		    int ret = app_av_close();

		    if(ret == 0)
			::send(client_fd,"a2dp-source-close-ok",strlen("a2dp-source-close-ok"),0);
		    else
			::send(client_fd,"a2dp-source-close-failed",strlen("a2dp-source-close-failed"),0);

		}else if(strstr(recv_buf, "a2dp-av-send-volume-") == &recv_buf[0]) {
		    int ret, volume;

			volume = atoi(&recv_buf[strlen("a2dp-av-send-volume-")]);
			ALOGD("a2dp-av-send-volume-: %d", volume);
			ret = app_av_rc_send_absolute_volume_vd_command(0, volume);

		    if(ret == 0)
			::send(client_fd,"a2dp-av-send-volume-ok",strlen("a2dp-av-send-volume-ok"),0);
		    else
			::send(client_fd,"a2dp-av-send-volume-failed",strlen("a2dp-av-send-volume-failed"),0);
#ifdef BLUETOOTH_AG_SUPPORT
		}else if(strstr(recv_buf, "ag-source-open-") == &recv_buf[0]) {
		    BD_ADDR addr;
		    memcpy(&addr,&recv_buf[strlen("ag-source-open-")],BD_ADDR_SIZE);

			int ret = app_ag_open(&addr);
			if(ret == 0)
			::send(client_fd,"ag-source-open-ok",strlen("ag-source-open-ok"),0);
		    else
			::send(client_fd,"ag-source-open-failed",strlen("ag-source-open-failed"),0);

		}else if(strstr(recv_buf, "ag-source-close-") == &recv_buf[0]) {
		    int ret = app_ag_close();

		    if(ret == 0)
			::send(client_fd,"ag-source-close-ok",strlen("ag-source-close-ok"),0);
		    else
			::send(client_fd,"ag-source-close-failed",strlen("ag-source-close-failed"),0);

		}else if(strstr(recv_buf, "ag-open-audio-") == &recv_buf[0]) {
		    app_ag_open_audio();
			::send(client_fd,"ag-open-audio-ok",strlen("ag-open-audio-ok"),0);

		}else if(strstr(recv_buf, "ag-close-audio-") == &recv_buf[0]) {
		    app_ag_close_audio();
			::send(client_fd,"ag-close-audio-ok",strlen("ag-close-audio-ok"),0);
#endif
		}else{
		    ALOGE("no valid data be received!");
		}
#endif
	    }
    }
	}
    ALOGV("client read thread over \n");
}

} //namespace android
void BluetoothServer_instance(){
	android::BluetoothServer::getInstance();
}
