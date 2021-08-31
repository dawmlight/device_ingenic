//#define LOG_NDEBUG 0
#define LOG_TAG "HilinkModule"

#include "SyncDataTools.h"
#include "HilinkModule.h"
#include <unistd.h>
//#include <dlog.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
//#include <utils/Log.h>

namespace android {

static const int CAP = 4 * 1024;
#define ALOGV printf
#define ALOGD printf
#define ALOGE printf
#define UNIX_DOMAIN "/var/run/hilink_socket"
#define BUFFER_SIZE (1024 * 8)

static void printPacketInfo(const char *title, const unsigned char *buf, unsigned int size, bool realy) {
    if (realy) {
        int j, i, k;
        char prtinfo[512], pr2[256];

        if (realy)
            ALOGD(" -%s-size=%d--- -start-------", title, size);
        for (k = i = 0; i < size; k++) {
            prtinfo[0] = 0;
            pr2[0] = 0;
            for (j = 0; (i < size && j < 16); j++, i++) {
                sprintf(pr2 + j, "%c", isgraph(buf[i]) ? buf[i] : ' ');
                sprintf(prtinfo + strlen(prtinfo), "%02X, ", ((unsigned int)buf[i]) & 0xff);
            }
            if (realy)
                ALOGD("%3x| %s -- %s", k, prtinfo, pr2);
        }
        if (realy)
            ALOGD(" -%s-size=%d--end----------", title, size);
    }
}

HilinkModule::HilinkModule(const string &name) {
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    ALOGD("%s %ld.%06ld", __FUNCTION__, tp.tv_sec, tp.tv_nsec / 1000);
    mModuleName = name;
    if (mModuleName.length() >= 15) {
        ALOGE("ModuleName(%s) must be less than 15!", name.c_str());
        exit(1);
    }

    mIsBinded = false;
    mBindType = NONE;
    mModule = this;
    memset(mAddress, 0, sizeof(mAddress));
    mNeedPrintData = false; //true;//
    isregisterOK = false;
    isSenddone = 0;
    startSocketClient();
    string msg = "register-";
    msg.append(name);
    int times = 0;
    while (::send(mSocketFd, msg.c_str(), msg.length(), 0) < 0) {
        ALOGE("send failed %d.", mSocketFd);
        if (times++ > 3) {
            clock_gettime(CLOCK_MONOTONIC, &tp);
            ALOGE("ThirdModule register fail.%s %ld.%06ld", tp.tv_sec, tp.tv_nsec / 1000);
            break;
        }
        sleep(1);
    }
    pthread_mutex_init(&mSendLock, NULL);
    times = 0;
    while (isregisterOK == false) {
        usleep(10 * 1000);
        if (times++ > 40) {
            clock_gettime(CLOCK_MONOTONIC, &tp);
            ALOGE("wait register failed %ld.%06ld.", tp.tv_sec, tp.tv_nsec / 1000);
            break;
        }
    }
    //   pthread_mutex_lock(&mMutex);
    //   pthread_cond_wait(&mCondition, &mMutex);
    //    pthread_mutex_unlock(&mMutex);
    if (isregisterOK) {
        clock_gettime(CLOCK_MONOTONIC, &tp);
        ALOGD("register ok. %ld.%06ld", tp.tv_sec, tp.tv_nsec / 1000);
    }
}

HilinkModule::~HilinkModule() {
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

//#define PRINT_DATA_INFO
int HilinkModule::send(const unsigned char *data, unsigned int size) {
    int waittimes=0;
    int ret=0;
    pthread_mutex_lock(&mSendLock);
/*    while (isSenddone != 0) {
        ALOGE("isSenddone is not ready %d", waittimes);
        if (waittimes++ > 1000) {
            pthread_mutex_unlock(&mSendLock);
            return -1;
        }
        usleep(1 * 1000);
    }
*/
    if(isSenddone)
        ALOGE("isSenddone is not ready %d", isSenddone);
    unsigned char buf[CAP]="hilink-send-";
    int len = strlen((const char*)buf);
    buf[len++]= size & 0xff;
    buf[len++]= (size>>8) & 0xff;
    memcpy(buf+len, data, size);
    isSenddone = 1;
    //ALOGD("start send:%s.",data);
    int retlen = ::send(mSocketFd, buf, len+size, 0);

    //ALOGD("send over,wait done");
    //-------wait send ok here---s-180831------
#if 0
    waittimes = 0;
    while (isSenddone == 1) {
        if (waittimes++ > 1000) //1000 * 100us = 100ms
        {
            //ALOGE("send wait times=%dms too long", waittimes);
            break;
        }
        usleep(1 * 100);
    }

    if(isSenddone == 2)
        ret = -1;
    else
        ret = 0;
    if (waittimes > 0)
        ALOGD("send wait times=%dus,ret:%d", waittimes*100,ret);
#else
    ALOGD("send %d data done",size);//等待发送完成信息有问题，等不到
#endif
    //-------wait send ok here---e-180831------
    isSenddone = 0;
    pthread_mutex_unlock(&mSendLock);
    return ret;
}

bool HilinkModule::stopSocketClient() {
    pthread_join(mReadThread, NULL);
    if (mSocketFd > 0) {
        ALOGD("stopSocketClient %d", mSocketFd);
        close(mSocketFd);
        mSocketFd = -1;
    } else
        ALOGE("no close socket %d <0", mSocketFd);

    return true;
}

bool HilinkModule::startSocketClient() {
    ALOGV("%s", __FUNCTION__);
    int fd;
    struct sockaddr_un ser_addr;
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sun_family = AF_UNIX;
    strcpy(ser_addr.sun_path, UNIX_DOMAIN);
    fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0); //clivia 180429
    if (fd < 0) {
        ALOGE("socket error: %s\n", strerror(errno));
        return false;
    }

    int flags = fcntl(fd, F_GETFD);
    flags |= FD_CLOEXEC;
    fcntl(fd, F_SETFD, flags);

    int ret = connect(fd, (struct sockaddr *)&ser_addr, sizeof(ser_addr));
    if (ret < 0) {
        ALOGE("connect socket error: %s\n", strerror(errno));
        return false;
    }

    mSocketFd = fd;
    ALOGD("%s,mSocketFd =%d", __FUNCTION__, mSocketFd);
    pthread_create(&mReadThread, NULL, HilinkModule::ReadThreadWrapper, this);
    return true;
}

bool HilinkModule::restartSocketClient(int retry_times) {
    ALOGV("%s", __FUNCTION__);
    int fd;
    struct sockaddr_un ser_addr;
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sun_family = AF_UNIX;
    strcpy(ser_addr.sun_path, UNIX_DOMAIN);
    fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0); //clivia 180429
    if (fd < 0) {
        ALOGE("socket error: %s\n", strerror(errno));
        return false;
    }

    int flags = fcntl(fd, F_GETFD);
    flags |= FD_CLOEXEC;
    fcntl(fd, F_SETFD, flags);

    int ret = connect(fd, (struct sockaddr *)&ser_addr, sizeof(ser_addr));
    if (ret < 0) {
        ALOGE("connect socket error: %s,retry_time:%d\n", strerror(errno), retry_times);
        close(fd);
        return false;
    }

    mSocketFd = fd;

    string msg = "register-";
    msg.append(mModuleName);
    int times = 0;
    while (::send(mSocketFd, msg.c_str(), msg.length(), 0) < 0) {
        //ALOGE("send failed %d.",mSocketFd);
        if (times++ > 3000) {
            ALOGE("ThirdModule register fail.");
            break;
        }
        usleep(1000);
    }

    ALOGD("%s,mSocketFd =%d,retry=%d,wait=%d", __FUNCTION__, mSocketFd, retry_times, times);
    return true;
}

void *HilinkModule::ReadThreadWrapper(void *me) {

    HilinkModule *module = static_cast<HilinkModule *>(me);
    module->ReadThreadFunc();
    return NULL;
}

bool HilinkModule::isOpened() {
    ALOGV("%s", __FUNCTION__);
    int fd = ::open("/sys/class/rfkill/rfkill0/state", O_RDONLY);
    if (fd < 0) {
        ALOGE("open /sys/class/rfkill/rfkill0/state failed: %s ", strerror(errno));
        return false;
    }

    char buffer = 0;
    ::read(fd, &buffer, 1);
    //clivia int state = ::atoi(buffer);
    ::close(fd);

    //clivia if(state == 1) return true;
    //clivia return false;
    return (buffer == '1') ? true : false; //clivia modify 20180413
}

static const char * const cmpstr[4] = {"bind-state-change:", "msg_send_ret:", "hilink-recv-","ble-close_evt-"};

void HilinkModule::ReadThreadFunc() {
    int count = 1;
    int cmpstrlen[3], i;
    int retry_times;
    unsigned char recv_buf[BUFFER_SIZE+256];

    while (1) {
        if (mSocketFd <= 0) {
            usleep(200 * 1000);
            if (isOpened()) {
                if (retry_times == 0)
                    usleep(400 * 1000);
                restartSocketClient(++retry_times);
            }
            if (mSocketFd > 0)
                retry_times = 0;
            else if (retry_times > 40)
                break;
        }
        while (mSocketFd > 0) {
            for (i = 0; i < 3; i++) {
                cmpstrlen[i] = strlen(cmpstr[i]);
            }
            memset(recv_buf, 0, BUFFER_SIZE);
            count = recv(mSocketFd, recv_buf, BUFFER_SIZE, MSG_NOSIGNAL);
            if (count <= 0) {
                ALOGE("socket %d is close ! error=%s(%d),errno=%d,count=%d", mSocketFd, strerror(errno), errno, count);
                close(mSocketFd);
                mSocketFd = -1;
                break;
            } else {
                {
                    int msgsum = 0;
                    int bcount = count;
                    unsigned int tlen;
                    unsigned char *recv_ptr = recv_buf;
                    bool isneedprint = true;
                    //            if(count > strlen("msg_send_ret:") +1)
                    //                ALOGD("total size=%d",count);
                    //printPacketInfo("hilinkModule recv-",recv_buf, count, true);
                    do {
                        if (memcmp(recv_buf, cmpstr[0] /*"bind-state-change:"*/, cmpstrlen[0] /*strlen("bind-state-change:")*/) == 0) {
                            tlen = cmpstrlen[0]; //strlen("bind-state-change:");
                            if (recv_buf[tlen] == '0') {
                                mIsBinded = false;
                                mBindType = NONE;
                            } else if (recv_buf[tlen] == '1') {
                                mIsBinded = true;
                                mBindType = BLE;
                            } else if (recv_buf[tlen] == '2') {
                                mIsBinded = true;
                                mBindType = BLUETOOTH;
                            }
                            isregisterOK = true;
                            struct timespec tp;
                            clock_gettime(CLOCK_MONOTONIC, &tp);

                            ALOGD("bind-state-change: mIsBinded=%s,bindType=%s %ld.%06ld", mIsBinded ? "true" : "false", (mBindType == BLE) ? "BLE" : ((mBindType == BLUETOOTH) ? "BLUETOOTH" : "NONE"), tp.tv_sec, tp.tv_nsec / 1000);
                            count -= tlen + 1;
                            recv_ptr += tlen + 1;
                            //pthread_cond_signal(&mCondition);
                            isneedprint = true;
                        } else if (memcmp(recv_ptr, cmpstr[1] /*"msg_send_ret:"*/, cmpstrlen[1] /*strlen("msg_send_ret:")*/) == 0) {
                            tlen = cmpstrlen[1]; //strlen("msg_send_ret:");
                            ALOGD("send ret %c",recv_ptr[cmpstrlen[1]]);
                            if(isSenddone ==1) {
                                if(recv_ptr[cmpstrlen[1]]=='1')//-1
                                    isSenddone = 2;//send fail
                                else
                                    isSenddone = 3;//send succ
                            }
                            count -= tlen + 1;
                            recv_ptr += tlen + 1;
                            isneedprint = true;
                        } else if(memcmp(recv_ptr, cmpstr[2]/*"recv-"*/, cmpstrlen[2]) == 0) {
                            tlen = (unsigned char)recv_ptr[cmpstrlen[2]+1];
                            tlen <<=8;
                            tlen += (unsigned char)recv_ptr[cmpstrlen[2]];
                            if(tlen > 0) {
                                //tlen = (unsigned char)recv_ptr[cmpstrlen[2]] + ((unsigned char)recv_ptr[cmpstrlen[2]+1]<<8);
                                printPacketInfo("hilinkModule recv-",(recv_ptr+cmpstrlen[2]+2), tlen, true);
                                onRetrive((const unsigned char*)(recv_ptr+cmpstrlen[2]+2), tlen);
                                msgsum++;
                                count -= cmpstrlen[2]+2 +tlen;
                                recv_ptr += cmpstrlen[2]+2 +tlen;
                            }else {
                                ALOGD("cmpstr:%s,len:%d",cmpstr[2],cmpstrlen[2]);
                                printPacketInfo("!!!hilinkModule wrong msg len!!!",recv_ptr, count, true);
                                tlen = 1;
                                recv_ptr += count;
                                count = 0;
                            }
                            isneedprint = true;
                        } else if(memcmp(recv_ptr, cmpstr[3], cmpstrlen[3]) == 0) {
                            tlen = cmpstrlen[3]; //strlen("msg_send_ret:");
                            if(tlen > 0)
                                onRetrive((const unsigned char*)recv_ptr, tlen);
                            else {
                                ALOGD("cmpstr:%s,len:%d",cmpstr[3],cmpstrlen[3]);
                                printPacketInfo("!!!hilinkModule wrong msg len!!!",recv_ptr, count, true);
                                tlen = 1;
                                recv_ptr += count;
                                count = 0;
                            }
                            count -= tlen;
                            recv_ptr += tlen;
                            isneedprint = true;
                        } else {
                            int remain_size = count;
                            if(isneedprint) {
                                isneedprint = false;
                                printPacketInfo("hilinkModule wrong msg",recv_ptr, count, true);
                            }
                            count --;
                            recv_ptr ++;
                        }
                    } while (count > 0);

                    if (msgsum > 1) {
                        ALOGD("msgsum:%d,allsize=%d", msgsum, bcount);
                    }
                }
            }
        }
    }
    ALOGV("read thread over \n");
}

BIND_TYPE HilinkModule::getBindType() {
    return mBindType;
}

bool HilinkModule::getBluetoothAddress(string &address) {
    if (strlen(mAddress) > 11) {
        address = mAddress;
        return true;
    }

    FILE *pFile = fopen("/usr/data/bsa/bt_addr", "r");
    if (NULL == pFile) {
        ALOGE("can't open filep(%s)", "/usr/data/bsa/bt_addr");
        return false;
    }

    fscanf(pFile, "%s", mAddress);
    fclose(pFile);
    if (mAddress == NULL || strlen(mAddress) < 11) {
        return false;
    } else {
        address = mAddress;
        return true;
    }
}

} //namespace android
