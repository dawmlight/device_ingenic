//#define LOG_NDEBUG 0
#define LOG_TAG "HilinkServer"
//#include <dlog.h>
#include <signal.h>
#include <utils/Log.h>
//#include <monitor/ProcessMonitor.h>
#include "HilinkServer.h"
#include <sys/socket.h>
#include <sys/un.h>

#include "BleProtocol.h"

namespace android {

#ifdef __cplusplus
extern "C"
{
#endif
#include "app_av.h"
extern int setBTVisibility(bool discoverable, bool connectable);
#ifdef __cplusplus
}
#endif


#define UNIX_DOMAIN "/var/run/hilink_socket"
#define BUFFER_SIZE 4096 //(10240*4) //4096//1024
static const string MODULE_NAME = "thirdparty_m";

static void printPacketInfo(const char* title, const unsigned char* buf, unsigned int size,bool realy)
{
	if(realy)
	{
	    int j,i,k;
	    char prtinfo[512],pr2[256];

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
            ALOGD("%3x| %s -- %s",k,prtinfo,pr2);
	    }
        ALOGD(" -%s-size=%d--end----------",title,size);
	}
}


HilinkServer* HilinkServer::sInstance = NULL;

HilinkServer* HilinkServer::getInstance(RefBase* protocol)
{
    ALOGV("%s", __FUNCTION__);
    if(sInstance == NULL){
	    sInstance = new HilinkServer(protocol);
    }
    return sInstance;
}

HilinkServer::HilinkServer(RefBase* protocol){
    pthread_mutex_init(&msendLock, NULL);
    mDone = false;
    mIsBinded = false;
    mProtocol = protocol;
    startSocketServer();
}

HilinkServer::~HilinkServer()
{
    stopSocketServer();
    pthread_mutex_destroy(&msendLock);
}

bool HilinkServer::registerModule(string module,int client_fd)
{
    struct timespec tp;
    ALOGV("%s,fd=%d", __FUNCTION__,client_fd);
    MODULEMAP::iterator it= mModuleMap.find(module.c_str());
    if(it == mModuleMap.end()) {
	mModuleMap.insert(MODULEMAP::value_type(module, client_fd));

	string msg = "bind-state-change:";
	switch(mType){
	case NONE:
	    msg.append("0");
	    break;
	case BLE:
	    msg.append("1");
	    break;
	case BLUETOOTH:
	    msg.append("2");
	    break;
	}

	::send(client_fd,msg.c_str(),msg.length(),0);

    clock_gettime(CLOCK_MONOTONIC,&tp);
	ALOGD("register module(%s) success!,%ld.%06ld.",module.c_str(),tp.tv_sec,tp.tv_nsec/1000);
    system("wpa_cli scan &");
	return true;
    }else{
    clock_gettime(CLOCK_MONOTONIC,&tp);
	ALOGE("register module(%s) failed,%ld.%06ld..system have a some module exist!",module.c_str(),tp.tv_sec,tp.tv_nsec/1000);
	return false;
    }
}

bool HilinkServer::unRegisterModule(int client_fd)
{
    ALOGD("%s,fd=%d", __FUNCTION__,client_fd);

    MODULEMAP::iterator it;
    for(it = mModuleMap.begin(); it != mModuleMap.end(); it++) {
	if(it->second == client_fd){
	    mModuleMap.erase(it);
	    ALOGD("unRegister module(%s) success,fd is %d",(it->first).c_str(), it->second);
	    return true;
	}
    }
    ALOGD("unRegister module fail,fd is %d", it->second);
    return false;
}

void HilinkServer::BindedStateChange(bool isBinded,bool isBle){
    ALOGV("%s", __FUNCTION__);
    mIsBinded = isBinded;
    string msg = "bind-state-change:";
    if(mIsBinded){
	mType = isBle ? BLE : BLUETOOTH;
	if(isBle)
	    msg.append("1");
	else
	    msg.append("2");
    }else{
	msg.append("0");
	mType = NONE;
    }
    MODULEMAP::iterator it;
    for(it = mModuleMap.begin(); it != mModuleMap.end(); it++) {
	  //send data to a client
	int client_fd = it->second;
	::send(client_fd,msg.c_str(),msg.length(),0);
    }
}

void HilinkServer::senddone(int client_fd,int ret)
{
    ALOGD("%s ret:%d", __FUNCTION__,ret);

	string msg = "msg_send_ret:";
	switch(ret){
	case 0:
	    msg.append("0");
	    break;
	case -1:
	default:
	    msg.append("1");
	    break;
	}
/*    MODULEMAP::iterator it;
    for(it = mModuleMap.begin(); it != mModuleMap.end(); it++) {
	  //send data to a client
	int client_fd = it->second;
	::send(client_fd,msg.c_str(),msg.length(),0);
    }
*/
	::send(client_fd,msg.c_str(),msg.length(),0);
}
void HilinkServer::onRetrive(const char * moduleName,unsigned char * data,unsigned int size)
{
    ALOGD("%s,size=%d", __FUNCTION__,size);
    MODULEMAP::iterator it= mModuleMap.find(moduleName);
    if(it == mModuleMap.end()) {
	ALOGE("onRetrive failed . no module(%s) exist!",moduleName);
    printPacketInfo("Unrespons msg",data,size,true);
	return ;
    }
    unsigned char buf[BUFFER_SIZE]="hilink-recv-";
    int len = strlen((const char*)buf);
    buf[len++] = size & 0xff;
    buf[len++] = (size>>8) & 0xff;
    memcpy(buf+len, data, size);
      //send data to a client
    int client_fd = it->second;
    ::send(client_fd, buf, len+size,0);
    ALOGD("hilink server Retrive %d",size);
    //printPacketInfo("hilink server onRetrive",buf,len+size,true);
}

int HilinkServer::sendTimes(string module,unsigned char* bytes,unsigned int size, int *used_size)
{

	const char msgsubstr[]="hilink-send-";
	const int msg_head_len = 14;
	const int off_msg_size = 10;
    unsigned int packSum = 0;
    unsigned int cursize;
    unsigned char *src;
    int newsize=0,strsize,total=0;
    int ret;

	*used_size = 0;
    if(size <= 0){
    	ALOGE("Error:serial failed!");
    	return -1;
    }

    ALOGV("%s wait lock", __FUNCTION__);
    //pthread_mutex_lock(&msendLock);

	//printPacketInfo("sendTimes",bytes,size,true);

    strsize = strlen((char *)msgsubstr);
    src = (unsigned char*)bytes;

    //cursize = (unsigned int)src[0] + ((unsigned int)src[1]<<8);
    cursize = src[1];
    cursize <<= 8;
    cursize += src[0];
#ifdef PRINT_DATA_INFO
    ALOGD("total=%d cursize=%d,size=%d",total,cursize,size);
#endif
    total = cursize + 2;

    ret = ((BleProtocol *)mProtocol)->sendPacket(module.c_str(),(char*)src+2,(int)cursize, true);
    src += total;

	*used_size = total;

    //pthread_mutex_unlock(&msendLock);
    return ret;
}

void HilinkServer::stopSocketServer(){
    void *dummy;
    mDone = true;
    pthread_join(mAcceptThread, &dummy);
    close(mListenfd);
}

void HilinkServer::startSocketServer(){
    struct sockaddr_un ser_addr;
    ser_addr.sun_family = AF_UNIX;
    strncpy(ser_addr.sun_path,UNIX_DOMAIN,sizeof(ser_addr.sun_path)-1);
    unlink(UNIX_DOMAIN);//delete already exist point
    mListenfd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);//clivia 180429
    if(mListenfd < 0){
        ALOGE("error: socket creation failed; skt_fd:%d\n", mListenfd);
        return;
    }
    if(bind(mListenfd, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) < 0){
    	ALOGE(" bind error: %s(errno: %d)\n",strerror(errno),errno);
	close(mListenfd);
	return;
    }
    if(listen(mListenfd, 1) < 0){
        ALOGE("error: hstp socket listen failed\n");
        close(mListenfd);
        return;
    }

    pthread_create(&mAcceptThread, NULL, HilinkServer::AcceptThreadWrapper, this);
}

void *HilinkServer::AcceptThreadWrapper(void *me) {
    HilinkServer *source = static_cast<HilinkServer *>(me);
    source->AcceptThreadFunc();
    return NULL;
}

void *HilinkServer::ClientReadThreadWrapper(void *arg) {
    THREAD_ARG *args = (THREAD_ARG *)arg;
    HilinkServer *server = static_cast<HilinkServer *>(args->server);
    server->ClientReadThreadFunc(args->client_fd);
    return NULL;
}

void *HilinkServer::SetBTVisibiltiy( void *ptr )
{       pthread_detach(pthread_self());
		usleep(2000*1000);
		//ALOGD("--------------clivia setBTVisibility pid:%d",pthread_self());
		setBTVisibility(true,true);
        pthread_exit(0) ;

}

void HilinkServer::AcceptThreadFunc(){
    //char buf[BUFFER_SIZE];
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
	pthread_create(&mReadThread, NULL, HilinkServer::ClientReadThreadWrapper, &arg);
    }
}

void HilinkServer::ClientReadThreadFunc(int client_fd_arg){
    unsigned char recv_buf[BUFFER_SIZE];
    int count = 1;
	const char *cmpstr[]={"register-","hilink-send-"};
	unsigned char *recv_ptr;
	int cmpstrlen[2],i,used_size,printflag;
	int client_fd = client_fd_arg;
	pthread_t mythread;

	for(i=0; i<2; i++)
	{
		cmpstrlen[i] =  strlen(cmpstr[i]);
	}

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
    setBTVisibility(true,true);
    while(1){
    pthread_mutex_lock(&msendLock);
	memset(recv_buf, 0 ,BUFFER_SIZE);
	count = recv(client_fd, recv_buf, BUFFER_SIZE, MSG_NOSIGNAL);
	if(count <= 0){
		setBTVisibility(false,false);
		ALOGE("socket is close ! error=%s(%d), errno=%d,count=%d",strerror(errno),errno,count);
		close(client_fd);
		unRegisterModule(client_fd);
		client_fd = -1;
    	pthread_mutex_unlock(&msendLock);
		break;
	}else {
		recv_ptr = recv_buf;
		printflag = 0;
        printPacketInfo("HilinkServer received ", recv_buf,count,true);
		do
		{
			if(memcmp(recv_ptr,cmpstr[1], cmpstrlen[1])==0)//"send-"
			{
				//int head_len = strlen("send-");
				//string module;
				recv_ptr += cmpstrlen[1];
				count -= cmpstrlen[1];
				MODULEMAP::iterator it;
				for(it = mModuleMap.begin(); it != mModuleMap.end(); it++) {
					if(it->second == client_fd){
						string module = it->first;
						//send data to SppProtocol
						int ret = sendTimes(module,recv_ptr,count,&used_size);
						count -=  used_size;
						recv_ptr += used_size;
                        //ALOGD("buf still size:%d",count);
						//senddone(client_fd,ret);
						break;
					}
				}
			}else if(memcmp(recv_ptr, cmpstr[0], cmpstrlen[0]) == 0)//"register-"
			{
				string module;
				//int head_len = strlen("register-");
				module.assign((char*)recv_ptr+cmpstrlen[0], count-cmpstrlen[0]);
				registerModule(module,client_fd);

				count -= cmpstrlen[0]+1;
				recv_ptr += cmpstrlen[0]+1;
			    //clivia pthread_create(&mythread, NULL, &SetBTVisibiltiy, NULL);
			}else
			{
				count --;
				recv_ptr ++;
				ALOGE("invalid data be received!");
				if(printflag ==0)// just print once
				{
					printPacketInfo("invalid data", recv_ptr,count,true);
					printflag=1;
				}
			}
		}while(count > 0);
	    }
    	pthread_mutex_unlock(&msendLock);
    }
	}
    ALOGV("client read thread over \n");
}

} //namespace android
