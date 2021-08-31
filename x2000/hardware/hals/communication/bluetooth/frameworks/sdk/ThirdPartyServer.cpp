//#define LOG_NDEBUG 0
#define LOG_TAG "ThirdPartyServer"
//#include <dlog.h>
#include <signal.h>
#include <utils/Log.h>
//#include <monitor/ProcessMonitor.h>
#include "ThirdPartyServer.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include "SppProtocol.h"
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


#define UNIX_DOMAIN "/var/run/thirdparty_socket"
#define BUFFER_SIZE 4096 //(10240*4) //4096//1024
static const string MODULE_NAME = "thirdparty_m";

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


ThirdPartyServer* ThirdPartyServer::sInstance = NULL;

ThirdPartyServer* ThirdPartyServer::getInstance(RefBase* protocol)
{
    ALOGV("%s", __FUNCTION__);
    if(sInstance == NULL){
	    sInstance = new ThirdPartyServer(protocol);
    }
    return sInstance;
}

ThirdPartyServer::ThirdPartyServer(RefBase* protocol){
    pthread_mutex_init(&msendLock, NULL);
    mDone = false;
    mIsBinded = false;
    mProtocol = protocol;
	//mNeedPrintData = false;
    startSocketServer();
}

ThirdPartyServer::~ThirdPartyServer()
{
    stopSocketServer();
    pthread_mutex_destroy(&msendLock);
}

bool ThirdPartyServer::registerModule(string module,int client_fd)
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
	return true;
    }else{
    clock_gettime(CLOCK_MONOTONIC,&tp);
	ALOGE("register module(%s) failed,%ld.%06ld..system have a some module exist!",module.c_str(),tp.tv_sec,tp.tv_nsec/1000);
	return false;
    }
}

bool ThirdPartyServer::unRegisterModule(int client_fd)
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

void ThirdPartyServer::BindedStateChange(bool isBinded,bool isBle){
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

bool ThirdPartyServer::senddone(int client_fd,int ret)
{
    ALOGV("%s ret:%d", __FUNCTION__,ret);

	string msg = "msg_send_ret:";
	switch(ret){
	case 0:
	    msg.append("0");
	    break;
	case 1:
	    msg.append("1");
	    break;
	default:
		msg.append("2");
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
void ThirdPartyServer::onRetrive(const char * moduleName,char * data,int size)
{
    ALOGD("%s,size=%d", __FUNCTION__,size);
    MODULEMAP::iterator it= mModuleMap.find(moduleName);
    if(it == mModuleMap.end()) {
	ALOGE("onRetrive failed . no module(%s) exist!",moduleName);
    printPacketInfo("Unrespons msg",data,size,true);
	return ;
    }

      //send data to a client
    int client_fd = it->second;
    ::send(client_fd,data,size,0);
}
#if 0
//#define PRINT_DATA_INFO
bool ThirdPartyServer::send(string module,char* bytes,int size)
{
#ifdef PRINT_DATA_INFO
    int j,i,k;
    char prtinfo[1024];
#endif
    ALOGD("%s size=%d", __FUNCTION__,size);
    if(size <= 0){
    	ALOGE("Error:serial failed!");
    	return false;
    }

    if(mIsBinded == false){
	ALOGE("%s:now state is unbind ", __FUNCTION__);
	return false;
    }
    //pthread_mutex_lock(&msendLock);
#ifdef PRINT_DATA_INFO
    //if(size > 137)
    {
	    ALOGD(" size=%d--- -start-------",size);
	    for(k=i=0; i<size;k++)
	    {
	        prtinfo[0]=0;
	        sprintf(prtinfo,"%3x | ",i>>4);
	        for(j=0; (i<size && j<16); j++,i++)
	        {
	            sprintf(prtinfo+strlen(prtinfo), "%02X, ", bytes[i]&0xff);
	        }
	        ALOGD("%3x| %s",k,prtinfo);
	    }
	    ALOGD(" -end----------");
	}
#endif
    if(mProtocol != NULL){
	if(mType == BLUETOOTH){
	    ((SppProtocol *)mProtocol)->sendPacket(module.c_str(),bytes,size);
	}else if(mType == BLE){
	    ((BleProtocol *)mProtocol)->sendPacket(module.c_str(),bytes,size);
	}
    }
    //pthread_mutex_unlock(&msendLock);
    return true;
}
#endif

bool ThirdPartyServer::sendTimes(string module,char* bytes,int size, int *used_size)
{

	const char msgsubstr[]={"send-"};
	const int msg_head_len = 14;
	const int off_msg_size = 10;
    unsigned int packSum = 0;
    int cursize;
    unsigned char *src;
    int newsize=0,strsize,total=0;
    bool ret;

	*used_size = 0;
    if(size <= 0){
    	ALOGE("Error:serial failed!");
    	return false;
    }

    if(mIsBinded == false){
	ALOGE("%s:now state is unbind ", __FUNCTION__);
	*used_size = size;
	return false;
    }
    ALOGV("%s wait lock", __FUNCTION__);
    //pthread_mutex_lock(&msendLock);

    ALOGV("%s size=%d", __FUNCTION__,size);

	//printPacketInfo("sendTimes",bytes,size,false);

    strsize = strlen((char *)msgsubstr);
    src = (unsigned char*)bytes;
    do
    {
    	if(packSum == 0)
    	{
    		//cursize = readInt(&src[10]);
    		//cursize = src[13];
    		cursize = (src[off_msg_size]<<24) + (src[off_msg_size+1]<<16) + (src[off_msg_size+2]<<8) + src[off_msg_size+3];
    		//memcpy(&cursize, src+10, sizeof(int));
    		newsize = cursize + msg_head_len;
#ifdef PRINT_DATA_INFO
    		ALOGD("total=%x pack=%d size=%X strsize=%d!!!!",total,packSum,cursize,strsize);
#endif
    		total = cursize + msg_head_len;

		    if(mProtocol != NULL){
            //ALOGD("pack=%d psize=0x%x",packSum,cursize + msg_head_len);
			if(mType == BLUETOOTH){
			    ret = ((SppProtocol *)mProtocol)->sendPacket(module.c_str(),(char*)src,(int)cursize + msg_head_len);
			}else if(mType == BLE){
			    ret = ((BleProtocol *)mProtocol)->sendPacket(module.c_str(),(char*)src,(int)cursize + msg_head_len);
			}
		    }
    		src += total;
    		packSum ++;
    	}
    	else
    	{
    		//dest = src+msg_head_len + strsize;//15;
    		//readInt(dest, &cursize);
    		cursize = (src[off_msg_size+strsize]<<24) + (src[off_msg_size+strsize+1]<<16) + \
				(src[off_msg_size+strsize+2]<<8) + src[off_msg_size+strsize+3];

    		//cursize = src[18];//[13+ strsize];
    		//memcpy(&cursize, src+10+ strsize, sizeof(int));
    		//cursize = src[10+ strsize]<<24 + src[11+ strsize]<<24 + src[12+ strsize]<<8 + src[13+ strsize];
#ifdef PRINT_DATA_INFO
    		ALOGD("total=%x,pack=%d size=%X, %02X.%02X.%02X.%02X !",total,packSum,cursize,src[15],
    			src[16],src[17],src[18]);
#endif
    		total += cursize + msg_head_len + strsize;//19;
    		if(total  > size)
    		{
    			ALOGE("!!!! error size=%d,need %d !!!!",size,total);
    			ret = false;
    			break;
    		}
   		    if(mProtocol != NULL){
            //ALOGD("pack=%d psize=0x%X",packSum,cursize + msg_head_len);
			if(mType == BLUETOOTH){
			    ret = ((SppProtocol *)mProtocol)->sendPacket(module.c_str(),(char*)src+strsize,(int)cursize + msg_head_len);
			}else if(mType == BLE){
			    ret = ((BleProtocol *)mProtocol)->sendPacket(module.c_str(),(char*)src+strsize,(int)cursize + msg_head_len);
			}
		    }
    		packSum ++;
    		src += cursize + msg_head_len + strsize;//19;
    	}
    }while(total < size);
	*used_size = total;
    if(packSum > 1)
		ALOGD(" %d package %d size,used %d.",packSum,size-(packSum-1)*5,total);

#ifdef PRINT_DATA_INFO
    if((size >100)&& (packSum >1))
    {
	    ALOGD("aft size=%d--- -start-------",size);
	    for(i=0; i<size;)
	    {
	        prtinfo[0]=0;
	        sprintf(prtinfo,"%3x | ",i>>4);
	        for(j=0; (i<size && j<16); j++,i++)
	        {
	            sprintf(prtinfo+strlen(prtinfo), "%02X, ", bytes[i]);
	        }
	        ALOGD("aft Server_send %s",prtinfo);
	    }
	    ALOGD("aft Server_send -end----------");
	}
#endif


    //pthread_mutex_unlock(&msendLock);
    return ret;
}
#if 0
void ThirdPartyServer::writeInt(unsigned char *ptr, unsigned int val)
{
	ptr[0]=(val>>24)&0xff;
	ptr[1]=(val>>16)&0xff;
	ptr[2]=(val>>8)&0xff;
	ptr[3]=val&0xff;
}
bool ThirdPartyServer::sendMulti(string module,char* bytes,int size)
{
#ifdef PRINT_DATA_INFO
    int j,i;
    char prtinfo[1024];
#endif
	const char msgsubstr[]={"send-"};
	const int msg_head_len = 14;
    unsigned int packSum = 0;
    int cursize;
    unsigned char *src,*dest;
    int newsize=0,strsize,total=0;


    if(size <= 0){
    	ALOGE("Error:serial failed!");
    	return false;
    }

    if(mIsBinded == false){
	ALOGE("%s:now state is unbind ", __FUNCTION__);
	return false;
    }
    //pthread_mutex_lock(&msendLock);

    ALOGD("%s size=%d", __FUNCTION__,size);
#ifdef PRINT_DATA_INFO
    if(size > 133)
    {
	    ALOGD("bef Server_sendMulti size=%d--- -start-------",size);
	    for(i=0; i<size;)
	    {
	        prtinfo[0]=0;
	        sprintf(prtinfo,"%3x | ",i>>4);
	        for(j=0; (i<size && j<16); j++,i++)
	        {
	            sprintf(prtinfo+strlen(prtinfo), "%02X, ", (unsigned char)bytes[i]);
	        }
	        ALOGD("bef Server_send %s",prtinfo);
	    }
	    ALOGD("befServer_send -end----------");
	}
#endif

    strsize = strlen((const char *)msgsubstr);
    src = dest = (unsigned char*)bytes;
    do
    {
    	if(packSum == 0)
    	{
    		//cursize = readInt(&src[10]);
    		//cursize = src[13];
    		cursize = (src[10]<<24) + (src[11]<<16) + (src[12]<<8) + src[13];
    		//memcpy(&cursize, src+10, sizeof(int));
    		newsize = cursize + msg_head_len;
    		ALOGD("Server_sendMulti pack=%d size=%X !!!!",packSum,cursize);
    		total = cursize + msg_head_len;
    		dest += cursize + msg_head_len;
    		src += cursize + msg_head_len;
    		packSum ++;
    	}
    	else
    	{
    		//cursize = src[13+ strsize];
    		cursize = (src[18]<<24) + (src[19]<<16) + (src[20]<<8) + src[21];
    		total += cursize+ msg_head_len + strsize;
    		ALOGD("Server_sendMulti pack=%d size=%X !!!!",packSum,cursize);
    		if(total  > size)
    		{
    			ALOGE("!!!! Server_sendMulti error size=%d,need %d !!!!",size,total);
    			break;
    		}
    		packSum ++;
    		memmove(dest, src+strsize+4, cursize + msg_head_len -4);
    		dest += cursize + msg_head_len -4;
    		newsize += cursize + msg_head_len-4;
    		src += cursize + msg_head_len + strsize;
    	}
    }while(total < size);
    if(packSum > 1)
	ALOGD("Server_sendMulti have %d package ",packSum);
    if(packSum > 1)
	    writeInt((unsigned char*)bytes, packSum);

#ifdef PRINT_DATA_INFO
    if((size >100)&& (packSum >1))
    {
	    ALOGD("aft Server_sendMulti size=%d--- -start-------",size);
	    for(i=0; i<size;)
	    {
	        prtinfo[0]=0;
	        sprintf(prtinfo,"%3x | ",i>>4);
	        for(j=0; (i<size && j<16); j++,i++)
	        {
	            sprintf(prtinfo+strlen(prtinfo), "%02X, ", bytes[i]);
	        }
	        ALOGD("aft Server_send %s",prtinfo);
	    }
	    ALOGD("aft Server_send -end----------");
	}
#endif


    if(mProtocol != NULL){
	if(mType == BLUETOOTH){
	    ((SppProtocol *)mProtocol)->sendPacket(module.c_str(),bytes,newsize);
	}else if(mType == BLE){
	    ((BleProtocol *)mProtocol)->sendPacket(module.c_str(),bytes,newsize);
	}
    }
    //pthread_mutex_unlock(&msendLock);
    return true;

}
#endif
void ThirdPartyServer::stopSocketServer(){
    void *dummy;
    mDone = true;
    pthread_join(mAcceptThread, &dummy);
    close(mListenfd);
}

void ThirdPartyServer::startSocketServer(){
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

    pthread_create(&mAcceptThread, NULL, ThirdPartyServer::AcceptThreadWrapper, this);
}

void *ThirdPartyServer::AcceptThreadWrapper(void *me) {
    ThirdPartyServer *source = static_cast<ThirdPartyServer *>(me);
    source->AcceptThreadFunc();
    return NULL;
}

void *ThirdPartyServer::ClientReadThreadWrapper(void *arg) {
    THREAD_ARG *args = (THREAD_ARG *)arg;
    ThirdPartyServer *server = static_cast<ThirdPartyServer *>(args->server);
    server->ClientReadThreadFunc(args->client_fd);
    return NULL;
}

void *ThirdPartyServer::SetBTVisibiltiy( void *ptr )
{       pthread_detach(pthread_self());
		usleep(2000*1000);
		//ALOGD("--------------clivia setBTVisibility pid:%d",pthread_self());
		setBTVisibility(true,true);
        pthread_exit(0) ;

}

void ThirdPartyServer::AcceptThreadFunc(){
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
	pthread_create(&mReadThread, NULL, ThirdPartyServer::ClientReadThreadWrapper, &arg);
    }
}

void ThirdPartyServer::ClientReadThreadFunc(int client_fd_arg){
    char recv_buf[BUFFER_SIZE];
    int count = 1;
	const char *cmpstr[]={"register-","send-"};
	char *recv_ptr;
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
#if 1
		recv_ptr = recv_buf;
		printflag = 0;
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
						sendTimes(module,recv_ptr,count,&used_size);
						count -=  used_size;
						recv_ptr += used_size;
						senddone(client_fd,1);
						break;
					}
				}
			}else if(memcmp(recv_ptr, cmpstr[0], cmpstrlen[0]) == 0)//"register-"
			{
				string module;
				//int head_len = strlen("register-");
				module.assign(recv_ptr+cmpstrlen[0], count-cmpstrlen[0]);
				registerModule(module,client_fd);

				count -= cmpstrlen[0]+1;
				recv_ptr += cmpstrlen[0]+1;
			    pthread_create(&mythread, NULL, &SetBTVisibiltiy, NULL);
			}else
			{
				count --;
				recv_ptr ++;
				ALOGE("no valid data be received!");
				if(printflag ==0)// just print once
				{
					printPacketInfo("invalid data", recv_ptr,count,true);
					printflag=1;
				}
			}
		}while(count > 0);
#else
		if(strstr(recv_buf, "register-") != NULL){
			printPacketInfo("register-", recv_buf,count,true);
		    string module;
		    int head_len = strlen("register-");
		    module.assign(recv_buf+head_len,count-head_len);
		    registerModule(module,client_fd);
		}else if(strstr(recv_buf, "send-") != NULL){
		    int head_len = strlen("send-");
		      //string module;
		    MODULEMAP::iterator it;
		    for(it = mModuleMap.begin(); it != mModuleMap.end(); it++) {
			if(it->second == client_fd){
			    string module = it->first;
			      //send data to SppProtocol
			    sendTimes(module,recv_buf+head_len,count-head_len,&used_size);
				senddone(client_fd,1);
			    break;
			}
		    }
		}else{
		    ALOGE("no valid data be received!");
			printPacketInfo("invalid data", recv_buf,count,true);
		}
#endif
	    }
    	pthread_mutex_unlock(&msendLock);
    }
	}
    ALOGV("client read thread over \n");
}

} //namespace android
