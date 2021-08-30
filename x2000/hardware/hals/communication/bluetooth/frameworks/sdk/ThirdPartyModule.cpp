//#define LOG_NDEBUG 0
#define LOG_TAG "ThirdPartyModule"

//#include <dlog.h>
#include <signal.h>
#include <unistd.h>
//#include <utils/Log.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "ThirdPartyModule.h"
#include "SyncDataTools.h"

namespace android {

static const int CAP = 4 * 1024;
#define ALOGV printf
#define ALOGD printf
#define ALOGE printf
#define UNIX_DOMAIN "/var/run/thirdparty_socket"
#define BUFFER_SIZE (1024*4)

static void printPacketInfo(const char* title, const char* buf, int size,bool realy)
{
	if(realy)
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

ThirdPartyModule::ThirdPartyModule(const string & name)
{
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC,&tp);
    ALOGD("%s %ld.%06ld", __FUNCTION__,tp.tv_sec,tp.tv_nsec/1000);
    mModuleName = name;
    if(mModuleName.length() >= 15){
	ALOGE("ModuleName(%s) must be less than 15!",name.c_str());
	exit(1);
    }

    mIsBinded = false;
    mBindType = NONE;
    mModule = this;
    memset(mAddress, 0, sizeof(mAddress));
    mNeedPrintData = false;//true;//
    isregisterOK = false;
    isSenddone = 0;
    startSocketClient();
    string msg = "register-";
    msg.append(name);
    int times = 0;
    while(::send(mSocketFd, msg.c_str(), msg.length(), 0) < 0){
        ALOGE("send failed %d.",mSocketFd);
        if(times++ >3)
        {
            clock_gettime(CLOCK_MONOTONIC,&tp);
            ALOGE("ThirdModule register fail.%s %ld.%06ld",tp.tv_sec,tp.tv_nsec/1000);
            break;
        }
        sleep(1);
    }
    pthread_mutex_init(&mSendLock, NULL);
    times = 0;
    while(isregisterOK == false) {
        usleep(10*1000);
        if(times++ > 40){
            clock_gettime(CLOCK_MONOTONIC,&tp);
            ALOGE("wait register failed %ld.%06ld.",tp.tv_sec,tp.tv_nsec/1000);
            break;
        }
    }
//   pthread_mutex_lock(&mMutex);
//   pthread_cond_wait(&mCondition, &mMutex);
//    pthread_mutex_unlock(&mMutex);
    if(isregisterOK) {
        clock_gettime(CLOCK_MONOTONIC,&tp);
        ALOGD("register ok. %ld.%06ld",tp.tv_sec,tp.tv_nsec/1000);
    }

}

ThirdPartyModule::~ThirdPartyModule()
{
    ALOGV("%s", __FUNCTION__);
    stopSocketClient();
    memset(mAddress, 0, sizeof(mAddress));
    mNeedPrintData = false;
    isregisterOK = false;
    isSenddone = 0;
    //pthread_cond_destroy(&mCondition);
    //pthread_mutex_destroy(&mMutex);
    pthread_mutex_destroy(&mSendLock);
}

int ThirdPartyModule::getSyncDataSize(const sp<SyncData> & data){
    char bytes[CAP];
    int size = SyncDataTools::data2Bytes(data,bytes);
    return size;
}
//#define PRINT_DATA_INFO
bool ThirdPartyModule::send(const sp<SyncData> & data)
{
    char bytes[CAP];
#ifdef PRINT_DATA_INFO
    int j;
    char prtinfo[1024];
#endif
    int waittimes=0;
    int i,datasize;
    pthread_mutex_lock(&mSendLock);
    while(isSenddone != 0)
    {
        ALOGE("isSenddone is not ready %d",waittimes);
        if(waittimes++ > 1000)
        {
            pthread_mutex_unlock(&mSendLock);
            return false;
        }
        usleep(1*1000);
    }
    int size = SyncDataTools::data2Bytes(data,bytes);
    if(size <= 0){
    	ALOGE("Error:serial failed!");
        pthread_mutex_unlock(&mSendLock);
    	return false;
    }

    datasize = (unsigned char)bytes[10];
    for(i=11; i<14; i++)
    {
        datasize <<=8;
        datasize += (unsigned char)bytes[i] & 0xff;
    }
    //ALOGD("size=%d",datasize+14);

    if((size - datasize) != 14)
        ALOGE("!!!!wrong Third_send_data_size=%d, send buf size=%d error !!!",datasize,size);
#ifdef PRINT_DATA_INFO
    ALOGD("Third_send size=%d---datasize=%d -start-------",size,datasize);
    for(i=0; i<size; i++)
    {
        prtinfo[0]=0;
        for(j=0; (i<size && j<16); j++,i++)
        {
            sprintf(prtinfo+strlen(prtinfo), "%02X, ", bytes[i]);
        }
        ALOGD("Third_send | %s",prtinfo);
    }
    ALOGD("Third_send -end----------");
#endif

    if(mBindType == NONE){
    	ALOGE("Error:bluetooth is disconnect now!");
        pthread_mutex_unlock(&mSendLock);
    	return false;
    }

    if(size > 150 && mBindType == BLE){
	ALOGE("Error:ble send data must be < 150!");
    pthread_mutex_unlock(&mSendLock);
	return false;
    }

    string msg = "send-";
    msg.append(bytes,size);
    isSenddone = 1;
    int retlen = ::send(mSocketFd, msg.c_str(), msg.length(), 0);

//-------wait send ok here---s-180831------
    waittimes = 0;
    while(isSenddone != 2)
    {
        if(waittimes++ > 21000)//2.1s
        {
            ALOGE("send wait times=%dms too long,exit send()",waittimes/10);
            break;
        }
        usleep(1*100);
    }

    if(waittimes>0)
        ALOGV("send wait times=%d",waittimes);
//-------wait send ok here---e-180831------
    isSenddone = 0;
    pthread_mutex_unlock(&mSendLock);
    return true;
}

bool ThirdPartyModule::stopSocketClient(){
    pthread_join(mReadThread, NULL);
    if(mSocketFd > 0){
        ALOGD("stopSocketClient %d",mSocketFd);
	    close(mSocketFd);
	    mSocketFd = -1;
    }else
        ALOGE("no close socket %d <0",mSocketFd);

    return true;
}

bool ThirdPartyModule::startSocketClient(){
    ALOGV("%s", __FUNCTION__);
    int fd;
    struct sockaddr_un ser_addr;
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sun_family = AF_UNIX;
    strcpy(ser_addr.sun_path,UNIX_DOMAIN);
    fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);//clivia 180429
    if(fd < 0){
	ALOGE("socket error: %s\n",strerror(errno));
        return false;
    }

    int flags = fcntl(fd, F_GETFD);
    flags |= FD_CLOEXEC;
    fcntl(fd, F_SETFD, flags);

    int ret = connect(fd, (struct sockaddr *)&ser_addr, sizeof(ser_addr));
    if(ret < 0){
	ALOGE("connect socket error: %s\n",strerror(errno));
	return false;
    }

    mSocketFd = fd;
    ALOGD("%s,mSocketFd =%d", __FUNCTION__,mSocketFd);
    pthread_create(&mReadThread, NULL, ThirdPartyModule::ReadThreadWrapper, this);
    return true;
}

bool ThirdPartyModule::restartSocketClient(int retry_times){
    ALOGV("%s", __FUNCTION__);
    int fd;
    struct sockaddr_un ser_addr;
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sun_family = AF_UNIX;
    strcpy(ser_addr.sun_path,UNIX_DOMAIN);
    fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);//clivia 180429
    if(fd < 0){
	ALOGE("socket error: %s\n",strerror(errno));
        return false;
    }

    int flags = fcntl(fd, F_GETFD);
    flags |= FD_CLOEXEC;
    fcntl(fd, F_SETFD, flags);

    int ret = connect(fd, (struct sockaddr *)&ser_addr, sizeof(ser_addr));
    if(ret < 0){
	ALOGE("connect socket error: %s,retry_time:%d\n",strerror(errno),retry_times);
    close(fd);
	return false;
    }

    mSocketFd = fd;

    string msg = "register-";
    msg.append(mModuleName);
    int times = 0;
    while(::send(mSocketFd, msg.c_str(), msg.length(), 0) < 0){
        //ALOGE("send failed %d.",mSocketFd);
        if(times++ >3000)
        {
            ALOGE("ThirdModule register fail.");
            break;
        }
        usleep(1000);
    }


    ALOGD("%s,mSocketFd =%d,retry=%d,wait=%d", __FUNCTION__, mSocketFd, retry_times,times);
    return true;
}

void *ThirdPartyModule::ReadThreadWrapper(void *me) {

    ThirdPartyModule *module = static_cast<ThirdPartyModule *>(me);
    module->ReadThreadFunc();
    return NULL;
}

bool ThirdPartyModule::isOpened(){
    ALOGV("%s", __FUNCTION__);
    int fd = ::open("/sys/class/rfkill/rfkill0/state", O_RDONLY);
    if (fd < 0){
        ALOGE("open /sys/class/rfkill/rfkill0/state failed: %s ",strerror(errno));
        return false;
    }

    char buffer=0;
    ::read(fd, &buffer, 1);
    //clivia int state = ::atoi(buffer);
    ::close(fd);

    //clivia if(state == 1) return true;
    //clivia return false;
    return (buffer=='1') ? true : false;//clivia modify 20180413
}

void ThirdPartyModule::ReadThreadFunc(){
    char recv_buf[BUFFER_SIZE];
    int count = 1;
	const char *cmpstr[2]={"bind-state-change:","msg_send_ret:"};
	int cmpstrlen[2],i;
    int retry_times;

	for(i=0; i<2; i++)
	{
		cmpstrlen[i] =  strlen(cmpstr[i]);
	}
    while(1){
        if(mSocketFd <= 0){
            usleep(200*1000);
            if(isOpened()) {
                if(retry_times == 0)
                    usleep(400*1000);
                restartSocketClient(++retry_times);
            }
            if(mSocketFd > 0)
                retry_times = 0;
            else if(retry_times >40)
                break;
        }
    while(mSocketFd > 0){
        memset(recv_buf, 0 ,BUFFER_SIZE);
        count = recv(mSocketFd, recv_buf, BUFFER_SIZE, MSG_NOSIGNAL);
        if(count <= 0){
            ALOGE("socket %d is close ! error=%s(%d),errno=%d,count=%d",mSocketFd,strerror(errno),errno, count);
            close(mSocketFd);
            mSocketFd = -1;
            break;
        }else {
    #if 0//clivia
		if(strstr(recv_buf, "bind-state-change:") != NULL){
		    if(recv_buf[count-1] == '0'){
			mIsBinded = false;
			mBindType = NONE;
		    }else if(recv_buf[count-1] == '1'){
			mIsBinded = true;
			mBindType = BLE;
		    }else if(recv_buf[count-1] == '2'){
			mIsBinded = true;
			mBindType = BLUETOOTH;
		    }
                struct timespec tp;
                clock_gettime(CLOCK_MONOTONIC,&tp);

                ALOGD("bind-state-change: mIsBinded=%s,bindType=%s %ld.%ld",mIsBinded?"true":"false",(mBindType==BLE)?"BLE":((mBindType==BLUETOOTH)?"BLUETOOTH":"NONE"),tp.tv_sec,tp.tv_nsec);
                isregisterOK = true;
                //pthread_cond_signal(&mCondition);
            }else
    #endif
            {
                int msgsum =0;
                int bcount = count;
                int tlen;
                char *recv_ptr = recv_buf;
    //            if(count > strlen("msg_send_ret:") +1)
    //                ALOGD("total size=%d",count);
                do
                {
                    if(memcmp(recv_buf, cmpstr[0]/*"bind-state-change:"*/,cmpstrlen[0]/*strlen("bind-state-change:")*/) == 0){
                        tlen = cmpstrlen[0];//strlen("bind-state-change:");
                        if(recv_buf[tlen] == '0'){
                        mIsBinded = false;
                        mBindType = NONE;
                        }else if(recv_buf[tlen] == '1'){
                        mIsBinded = true;
                        mBindType = BLE;
                        }else if(recv_buf[tlen] == '2'){
                        mIsBinded = true;
                        mBindType = BLUETOOTH;
                        }
                        isregisterOK = true;
                        struct timespec tp;
                        clock_gettime(CLOCK_MONOTONIC,&tp);

                        ALOGD("bind-state-change: mIsBinded=%s,bindType=%s %ld.%06ld",mIsBinded?"true":"false",(mBindType==BLE)?"BLE":((mBindType==BLUETOOTH)?"BLUETOOTH":"NONE"),tp.tv_sec,tp.tv_nsec/1000);
                        count -= tlen + 1;
                        recv_ptr += tlen +1;
                        //pthread_cond_signal(&mCondition);
                    }else if(memcmp(recv_ptr, cmpstr[1]/*"msg_send_ret:"*/, cmpstrlen[1]/*strlen("msg_send_ret:")*/)==0)
                    {
                        tlen = cmpstrlen[1];//strlen("msg_send_ret:");
                        if(isSenddone == 1)
                            isSenddone=2;
                        count -= tlen +1;
                        recv_ptr += tlen +1;
                    }else
                    {
                        int remain_size=count;

                        sp<SyncData> syncData = new SyncData();
                        int retsize = SyncDataTools::bytes2Data(syncData,recv_ptr,remain_size);

                        if(retsize < 0){
                        ALOGE("function=%s,Error:syncdata parser failed!\n",__FUNCTION__);
                        break;
                        }else{
                        printPacketInfo("rcv-",recv_ptr,remain_size,true);
                        onRetrive(syncData);
                        }
                        remain_size -= retsize;
                        if(remain_size > strlen("msg_send_ret:") +1)
                            ALOGD("allsize=%d,size=%d", bcount, retsize);
                        else
                            ALOGD("size=%d",retsize);
                        msgsum++;
                        count -= retsize;
                        recv_ptr += retsize;
                        //delete syncData;
                    }
                }while(count > 0);

                if(msgsum > 1)
                {
                    ALOGD("msgsum:%d,allsize=%d", msgsum,bcount);
                }
            }

        }
        }
    }
    ALOGV("read thread over \n");
}

BIND_TYPE  ThirdPartyModule::getBindType(){
    return mBindType;
}

bool ThirdPartyModule::getBluetoothAddress(string & address){
    if(strlen(mAddress) > 11)
    {
        address = mAddress;
        return true;
    }

    FILE* pFile = fopen("/usr/data/bsa/bt_addr","r");
    if (NULL == pFile) {
	    ALOGE("can't open filep(%s)", "/usr/data/bsa/bt_addr");
	    return false;
    }

    fscanf(pFile, "%s", mAddress);
    fclose(pFile);
    if(mAddress == NULL || strlen(mAddress) < 11){
    	return false;
    }else{
    	address = mAddress;
    	return true;
    }
}

} //namespace android
