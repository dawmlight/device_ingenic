//#define LOG_NDEBUG 0
#define LOG_TAG "BluetoothController"

#include "BluetoothController.h"
//#include <dlog.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
//#include <utils/Log.h>

static const int CAP = 4 * 1024;
#define ALOGV printf
#define ALOGD printf
#define ALOGE printf
#define UNIX_DOMAIN "/var/run/bluetooth_socket"
#define BUFFER_SIZE 1024

const char *btcmdStr[] = {
    "open-ok", "close-ok", "disable-ok", "disccmpl-",
    "a2dp-source-open-ok", "a2dp-source-open-failed", "a2dp-source-close-ok", "a2dp-source-close-failed",
    "dellinked-ok", "dellinked-failed", "linkingcmpl-", "linking-",
    "linkedcmpl-", "linked-", "discovery-", "a2dp-source-open-",
    "a2dp-source-close", "dellinked-",
    "ag-source-open-", "ag-source-close-", "a2dp-av-notify-",
    "a2dp-av-check_connect-",
    //            "a2dp-av-send-volume-ok","a2dp-av-send-volume-failed",

};
typedef enum {
    CMD_OPEN_OK_IDX = 0,              //     0
    CMD_CLOSE_OK_IDX,                 //    1
    CMD_DISABLE_OK_IDX,               //  2
    CMD_DISCMPL_IDX,                  //     3
    CMD_A2DP_SOURCE_OPEN_OK_IDX,      // 4
    CMD_A2DP_SOURCE_OPEN_FAILED_IDX,  // 5
    CMD_A2DP_SOURCE_CLOSE_OK_IDX,     //    6
    CMD_A2DP_SOURCE_CLOSE_FAILED_IDX, //    7
    CMD_DELLINKED_OK_IDX,             //                8
    CMD_DELLINKED_FAILED_IDX,         //            9
    CMD_LINKINGCMPL_IDX,              //                 10
    CMD_LINKING_IDX,                  //                     11
    CMD_LINKEDCMPL_IDX,               //     12
    CMD_LINKED_IDX,                   //          13
    CMD_DISCOVERY_IDX,                //       14
    CMD_A2DP_SOURCE_OPEN_IDX,         //     15
    CMD_A2DP_SOURCE_CLOSE_IDX,        //   16
    CMD_DELLINKED_IDX,                //       17
    CMD_AG_SOURCE_OPEN,
    CMD_AG_SOURCE_CLOSE,
    CMD_A2DP_AV_NOTIFY,
    CMD_A2DP_AV_CHECK_CONNECT,
    CMD_A2DP_AV_SEND_VOLUME_OK,     //
    CMD_A2DP_AV_SEND_VOLUME_FAILED, //
} BluetoothContrroller_cmd_type;

static bool avLinkUp = false;
static bool avLinkDn = false;
static bool dellink = false;
static bool ddisconnect = false;
static bool agLinked = false;

static bool avLinkWait = false;
static bool avLinkDnWait = false;
static bool isopenok = false;
static bool iscloseok = false;
static bool isdellinked = false;

void (*disc_call_back)(DISC_EVT event, REMOTE_DEVICE *p_data) = NULL;
void (*linked_call_back)(DISC_EVT event, REMOTE_DEVICE *p_data) = NULL;
void (*linking_call_back)(DISC_EVT event, REMOTE_DEVICE *p_data) = NULL;
void (*a2dp_notify_call_back)(int state) = NULL;

BluetoothController *BluetoothController::sInstance = NULL;
BluetoothController *BluetoothController::getInstance() {
    ALOGV("%s", __FUNCTION__);
    if (sInstance == NULL) {
        sInstance = new BluetoothController();
    }
    return sInstance;
}

BluetoothController::BluetoothController() {
    ALOGV("%s", __FUNCTION__);
    mavIsConnected = 0;
    startSocketClient();

    pthread_mutex_init(&mMutex, NULL);
    pthread_cond_init(&mCondition, NULL);

    int ret = pthread_condattr_init(&mAvConditionAttr);
    {
    ret = pthread_condattr_setclock(&mAvConditionAttr, CLOCK_MONOTONIC);
    ret = pthread_cond_init(&mAvCondition, &mAvConditionAttr);
    }
    //pthread_cond_init(&mAvCondition, NULL);

}

BluetoothController::~BluetoothController() {
    ALOGV("%s", __FUNCTION__);
    stopSocketClient();

    pthread_cond_destroy(&mCondition);
    pthread_cond_destroy(&mAvCondition);
    pthread_condattr_destroy(&mAvConditionAttr);
    pthread_mutex_destroy(&mMutex);
}

bool BluetoothController::open() {
    ALOGV("%s", __FUNCTION__);

    if (isOpened() == true)
        return true;

    string msg = "open-";
    int retlen = ::send(mSocketFd, msg.c_str(), msg.length(), 0);
    if (retlen > 0) {
        pthread_cond_wait(&mCondition, &mMutex);
        return true;
    } else
        return false;
}

bool BluetoothController::setBleBeacon(int state) {
    ALOGD("%s", __FUNCTION__);
    if (isOpened() == false) {
        ALOGE("setBLEbeacon failed:must open bluetooth first.");
        return false;
    }

    string msg = "hwbeacon-";
    if (state == 0)
        msg.append("0");
    else if(state == 1)
        msg.append("1");
    else if(state == 2)
        msg.append("2");
    else if(state == 3)
        msg.append("3");
    else //if(state == 4)
        msg.append("4");
    int retlen = ::send(mSocketFd, msg.c_str(), msg.length(), 0);
    if (retlen > 0) {
        return true;
    } else
        return false;
}

bool BluetoothController::setBluetoothVisibility(bool enable) {
    ALOGV("%s", __FUNCTION__);
    if (isOpened() == false) {
        ALOGE("setBluetoothVisibility failed:must open bluetooth first.");
        return false;
    }

    string msg = "visibility-";
    if (enable)
        msg.append("true");
    else
        msg.append("false");

    int retlen = ::send(mSocketFd, msg.c_str(), msg.length(), 0);
    if (retlen > 0) {
        return true;
    } else
        return false;
}
int BluetoothController::waitTrue(bool *isTrue, int timeout) {
    int times = 0;
    while (*isTrue == false) {
        if (times++ > timeout)
            break;
        usleep(5 * 1000);
    }
    return times * 5;
}

bool BluetoothController::isOpened() {
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

bool BluetoothController::close() {
    ALOGV("%s", __FUNCTION__);

    if (isOpened() == false)
        return true;

    string msg = "close-";
    int retlen = ::send(mSocketFd, msg.c_str(), msg.length(), 0);
    if (retlen > 0) {
        pthread_cond_wait(&mCondition, &mMutex);
        return true;
    } else
        return false;
}

bool BluetoothController::HeadSetAnswerCall() {
    ALOGV("%s", __FUNCTION__);

    string msg = "answercall-";
    int retlen = ::send(mSocketFd, msg.c_str(), msg.length(), 0);
    if (retlen > 0)
        return true;
    else
        return false;
}

bool BluetoothController::HeadSetHangUp() {
    ALOGV("%s", __FUNCTION__);

    string msg = "hangup-";
    int retlen = ::send(mSocketFd, msg.c_str(), msg.length(), 0);
    if (retlen > 0)
        return true;
    else
        return false;
}

bool BluetoothController::disconnect() {
    ALOGV("%s", __FUNCTION__);
    string msg = "disable-";
    int retlen = ::send(mSocketFd, msg.c_str(), msg.length(), 0);
    if (retlen > 0)
        return true;
    else
        return false;
}

bool BluetoothController::creatBond(const string &bd_name) {
    ALOGV("%s", __FUNCTION__);
    string msg = "creatbond-";
    msg.append(bd_name);
    int retlen = ::send(mSocketFd, msg.c_str(), msg.length(), 0);
    if (retlen > 0)
        return true;
    else
        return false;
}

bool BluetoothController::removeBond(const string &bd_name) {
    ALOGV("%s", __FUNCTION__);
    string msg = "removebond-";
    msg.append(bd_name);
    int retlen = ::send(mSocketFd, msg.c_str(), msg.length(), 0);
    if (retlen > 0)
        return true;
    else
        return false;
}
bool BluetoothController::dellinked(const BD_ADDR addr) {
    ALOGD("%s", __func__);

    char msg[50] = "dellinked-";
    dellink = false;
    isdellinked = false;
    memcpy(&msg[strlen("dellinked-")], addr, BD_ADDR_SIZE);
    int retlen = ::send(mSocketFd, msg, strlen("dellinked-") + BD_ADDR_SIZE, 0);
    if (retlen > 0) {
#if 1
        int times = waitTrue(&isdellinked, 1000);
        ALOGD("after wait %d ms,dellinked return %s", times, dellink ? "true" : "false");
        return dellink;
#else
        pthread_cond_wait(&mCondition, &mMutex);
        if (dellink == true)
            return true;
#endif
    }
    return false;
}

bool BluetoothController::getLinked(void (*disc_back)(DISC_EVT event, REMOTE_DEVICE *p_data), int duration) {
    ALOGD("%s", __FUNCTION__);
    linked_call_back = disc_back;
    string msg = "getlinked-";
    msg.append(std::to_string(duration));
    int retlen = ::send(mSocketFd, msg.c_str(), msg.length(), 0);
    if (retlen > 0)
        return true;
    else
        return false;
}

bool BluetoothController::getLinking(void (*disc_back)(DISC_EVT event, REMOTE_DEVICE *p_data), int duration) {
    ALOGD("%s", __FUNCTION__);
    linking_call_back = disc_back;
    string msg = "getlinking-";
    msg.append(std::to_string(duration));
    ALOGD("%s,%d", msg.c_str(), msg.length());
    int retlen = ::send(mSocketFd, msg.c_str(), msg.length(), 0);
    if (retlen > 0)
        return true;
    else
        return false;
}

bool BluetoothController::startDiscovery(void (*disc_back)(DISC_EVT event, REMOTE_DEVICE *p_data), int duration, int type) {
    ALOGV("%s", __FUNCTION__);
    disc_call_back = disc_back;
    string msg = "startdisc-";
    msg.append(std::to_string(duration));
    msg.append("-");
    msg.append(std::to_string(type));
    msg.append("-");
    int retlen = ::send(mSocketFd, msg.c_str(), msg.length(), 0);
    if (retlen > 0)
        return true;
    else
        return false;
}

bool BluetoothController::A2DPSocketReady() {
    ALOGD("%s() socket:%d", __FUNCTION__, mSocketFd);
    string msg = "a2dp-av-socket-ready-";
    int retlen = ::send(mSocketFd, msg.c_str(), msg.length(), 0);
    ALOGD(" ret=%d", retlen);
    if (retlen > 0)
        return true;
    else
        return false;
}

bool BluetoothController::A2DPSendVolume(int volume) {
    ALOGD("%s() volume= %d, socket:%d", __FUNCTION__, volume, mSocketFd);
    string msg = "a2dp-av-send-volume-";
    msg.append(std::to_string(volume));
    int retlen = ::send(mSocketFd, msg.c_str(), msg.length(), 0);
    ALOGD(" ret=%d", retlen);
    if (retlen > 0)
        return true;
    else
        return false;
}

bool BluetoothController::stopDiscovery() {
    ALOGV("%s", __FUNCTION__);
    string msg = "stopdisc-";
    int retlen = ::send(mSocketFd, msg.c_str(), msg.length(), 0);
    if (retlen > 0)
        return true;
    else
        return false;
}
bool BluetoothController::A2DPCheckConnect(int &is_connect) {
    ALOGV("%s", __FUNCTION__);
    string msg = "a2dp-av-check_connect-";
    int retlen = ::send(mSocketFd, msg.c_str(), msg.length(), 0);
    if (retlen <= 0) {
        is_connect = 0;
        return false;
    }
#if 0
    struct timeval now;
    struct timespec outtime;

    pthread_mutex_lock(&g_mutex);

    gettimeofday(&now, NULL);
    outtime.tv_sec = now.tv_sec;
    outtime.tv_nsec = now.tv_usec * 1000;
    pthread_cond_timedwait(&mAvCondition, &mMutex);
#else
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    ALOGV("%s-pthread_cond_timedwait 1s--", __FUNCTION__);
    tv.tv_sec += 1;
    int ret = pthread_cond_timedwait(&mAvCondition, &mMutex, &tv);
    if(ret == 0)
        ALOGV("%s-done, av_connect=%d,ret = %d", __FUNCTION__, mavIsConnected,ret);
    else {
        mavIsConnected = 0;
        ALOGV("%s-fail, ret = %d", __FUNCTION__, mavIsConnected,ret);
    }
#endif
    is_connect = mavIsConnected;
    return true;
}
/**
 * void (*a2dp_callback)(BD_ADDR addr, int state)
 * state 1:av_start, 0:av_stop
 **/
bool BluetoothController::A2DPNotify(void (*a2dp_callback)(int state)) {
    ALOGV("%s", __FUNCTION__);
    string msg = "a2dp-av-notify-";
    int cb_state = 0;
    if (a2dp_callback) {
        cb_state = 1;
        a2dp_notify_call_back = a2dp_callback;
    } else {
        a2dp_notify_call_back = NULL;
    }
    msg.append(std::to_string(cb_state));
    int retlen = ::send(mSocketFd, msg.c_str(), msg.length(), 0);
    if (retlen > 0)
        return true;
    else
        return false;
}

bool BluetoothController::HFPAGLinkUp(const BD_ADDR addr) {
    ALOGV("%s", __FUNCTION__);
    if (isOpened() == false)
        return false;

    char msg[50] = "ag-source-open-";
    memcpy(&msg[strlen("ag-source-open-")], addr, BD_ADDR_SIZE);
    agLinked = false;
    int retlen = ::send(mSocketFd, msg, strlen(msg) + BD_ADDR_SIZE, 0);
    if (retlen > 0) {
        pthread_cond_wait(&mCondition, &mMutex);
        if (agLinked)
            return true;
    }
    return false;
}

bool BluetoothController::HFPAGLinkDown() {
    ALOGV("%s", __FUNCTION__);
    if (isOpened() == false)
        return false;

    agLinked = false;
    char *msg = "ag-source-close-";

    int retlen = ::send(mSocketFd, msg, strlen(msg), 0);
    if (retlen > 0) {
        pthread_cond_wait(&mCondition, &mMutex);
        if (agLinked)
            return true;
    }
    return false;
}

bool BluetoothController::AGOpenAudio() {
    ALOGV("%s", __FUNCTION__);
    if (isOpened() == false)
        return false;

    char *msg = "ag-open-audio-";

    int retlen = ::send(mSocketFd, msg, strlen(msg), 0);
    if (retlen > 0)
        return true;
    else
        return false;
}

bool BluetoothController::AGCloseAudio() {
    ALOGV("%s", __FUNCTION__);
    if (isOpened() == false)
        return false;

    char *msg = "ag-close-audio-";

    int retlen = ::send(mSocketFd, msg, strlen(msg), 0);
    if (retlen > 0)
        return true;
    else
        return false;
}

bool BluetoothController::A2DPSouceLinkUp(const BD_ADDR addr) {
    static int maxwait = 0;
    if (isOpened() == false)
        return false;

    char msg[50] = "a2dp-source-open-";
    memcpy(&msg[strlen("a2dp-source-open-")], addr, BD_ADDR_SIZE);
    avLinkWait = false;
    avLinkUp = false;
    int retlen = ::send(mSocketFd, msg, strlen("a2dp-source-open-") + BD_ADDR_SIZE, 0);
    if (retlen > 0) {
#if 1
        //        struct timespec ctp;
        int times = waitTrue(&avLinkWait, 2000);
        if (maxwait < times)
            maxwait = times;
        //        usleep(100*1000);
        /*
        clock_gettime(CLOCK_MONOTONIC,&ctp);
        if(ctp.tv_nsec < tp.tv_nsec)
        {
            ctp.tv_sec = ctp.tv_sec-tp.tv_sec -1;
            ctp.tv_nsec = 1000000000UL + ctp.tv_nsec - tp.tv_nsec;
        }else
        {
            ctp.tv_sec = ctp.tv_sec-tp.tv_sec;
            ctp.tv_nsec = ctp.tv_nsec - tp.tv_nsec;
        }

        if((ctp.tv_sec > maxtp.tv_sec) || ((ctp.tv_sec==maxtp.tv_sec)&&(maxtp.tv_nsec<ctp.tv_nsec)))
            memcpy(&maxtp, &ctp, sizeof(ctp));
        ALOGD("wait %d ms(max:%d ms),exec time: %ld%03ldms(max:%ld%03ldms),return %s", times, maxwait,\
            ctp.tv_sec, ctp.tv_nsec/1000000UL,\
            maxtp.tv_sec, maxtp.tv_nsec/1000000UL,\
            avLinkUp?"true":"false");
*/
        return avLinkUp;
#else
        pthread_cond_wait(&mCondition, &mMutex);
        if (avLinkUp)
            return true;
#endif
    }
    return false;
}

bool BluetoothController::A2DPSouceLinkDown() {
    static int maxwait = 0;
    if (isOpened() == false)
        return false;

    avLinkDn = false;
    avLinkDnWait = false;
    char *msg = "a2dp-source-close-";

    int retlen = ::send(mSocketFd, msg, strlen(msg), 0);
    if (retlen > 0) {
#if 1
        //        struct timespec ctp,ctp1;
        int times = waitTrue(&avLinkDnWait, 2000);
        //        if (times > maxwait)
        //            maxwait = times;
        //        usleep(100*1000);
        /*        clock_gettime(CLOCK_MONOTONIC,&ctp);
        if(ctp.tv_nsec < tp.tv_nsec)
        {
            ctp.tv_sec = ctp.tv_sec-tp.tv_sec -1;
            ctp.tv_nsec = 1000000000UL + ctp.tv_nsec - tp.tv_nsec;
        }else
        {
            ctp.tv_sec = ctp.tv_sec-tp.tv_sec;
            ctp.tv_nsec = ctp.tv_nsec - tp.tv_nsec;
        }
        if((ctp.tv_sec > maxtp.tv_sec) || ((ctp.tv_sec==maxtp.tv_sec)&&(maxtp.tv_nsec<ctp.tv_nsec)))
            memcpy(&maxtp, &ctp, sizeof(ctp));
        ALOGD("wait %d ms(max:%d ms),exec time: %d%03ldms(max:%ld%03ldms),return %s",times,maxwait,\
            ctp.tv_sec, ctp.tv_nsec/1000000UL,\
            maxtp.tv_sec, maxtp.tv_nsec/1000000UL,\
             avLinkDn?"true":"false");
*/
        return avLinkDn;
#else
        pthread_cond_wait(&mCondition, &mMutex);
        if (avLinkDn)
            return true;
#endif
    }
    return false;
}

bool BluetoothController::stopSocketClient() {
    pthread_join(mReadThread, NULL);
    if (mSocketFd > 0) {
        ::close(mSocketFd);
        mSocketFd = -1;
    }
    return true;
}

bool BluetoothController::startSocketClient() {
    ALOGD("%s,mReadThread:%d", __FUNCTION__, mReadThread);
    bool ret = true;
    int fd;
    mSocketFd = -1;
    struct sockaddr_un ser_addr;
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sun_family = AF_UNIX;
    strcpy(ser_addr.sun_path, UNIX_DOMAIN);
    fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0); //clivia 180429
    if (fd < 0) {
        ALOGE("socket error: %s\n", strerror(errno));
        //return false;
        ret = false;
    } else {

        int flags = fcntl(fd, F_GETFD);
        flags |= FD_CLOEXEC;
        fcntl(fd, F_SETFD, flags);

        int ret = connect(fd, (struct sockaddr *)&ser_addr, sizeof(ser_addr));
        if (ret < 0) {
            ALOGE("connect socket error: %s\n", strerror(errno));
            ::close(fd);
            //return false;
            ret = false;
        }
    }

    mSocketFd = fd;
    ALOGD("mSocketFd =%d", mSocketFd);
    pthread_create(&mReadThread, NULL, BluetoothController::ReadThreadWrapper, this);
    //return true;
    return ret;
}
bool BluetoothController::restartSocketClient(int retry_times) {
    ALOGV("%s", __FUNCTION__);
    int fd;
    struct sockaddr_un ser_addr;
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sun_family = AF_UNIX;
    strcpy(ser_addr.sun_path, UNIX_DOMAIN);
    fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0); //clivia 180429
                                                         //    ALOGD("volume btc fd =%d",fd);
    if (fd < 0) {
        ALOGE("socket error: %s\n", strerror(errno));
        return false;
    }

    int flags = fcntl(fd, F_GETFD);
    flags |= FD_CLOEXEC;
    fcntl(fd, F_SETFD, flags);

    int ret = connect(fd, (struct sockaddr *)&ser_addr, sizeof(ser_addr));
    if (ret < 0) {
        ALOGE("connect socket error: %s,times:%d\n", strerror(errno), retry_times);
        ::close(fd);
        mSocketFd = -1;
        return false;
    }

    mSocketFd = fd;
    ALOGD("mSocketFd =%d,times=%d", mSocketFd, retry_times);
    return true;
}

void *BluetoothController::ReadThreadWrapper(void *me) {

    BluetoothController *module = static_cast<BluetoothController *>(me);
    module->ReadThreadFunc();
    return NULL;
}

void BluetoothController::printPacketInfo(const char *title, const char *buf, int size, bool realy) {
    if (/*mNeedPrintData || */ realy) {
        int j, i, k;
        char prtinfo[512], pr2[256];

        if (realy)
            ALOGE(" -%s-size=%d--- -start-------", title, size);
        else
            ALOGD(" -%s-size=%d--- -start-------", title, size);
        for (k = i = 0; i < size; k++) {
            prtinfo[0] = 0;
            pr2[0] = 0;
            for (j = 0; (i < size && j < 16); j++, i++) {
                sprintf(pr2 + j, "%c", isgraph(buf[i]) ? buf[i] : ' ');
                sprintf(prtinfo + strlen(prtinfo), "%02X, ", ((unsigned int)buf[i]) & 0xff);
            }
            if (realy)
                ALOGE("%3x| %s -- %s", k, prtinfo, pr2);
            else
                ALOGD("%3x| %s -- %s", k, prtinfo, pr2);
        }
        if (realy)
            ALOGE(" -%s-size=%d--end----------", title, size);
        else
            ALOGD(" -%s-size=%d--end----------", title, size);
    }
}

void BluetoothController::ReadThreadFunc() {
    char recv_buf[BUFFER_SIZE];
    char *recv_ptr;
    int count = 1, retry_times;
    bool printflag;

    while (1) {
        if (mSocketFd <= 0) {
            usleep(200 * 1000);
            if (isOpened()) {
                if (retry_times == 0)
                    usleep(400 * 1000);
                restartSocketClient(++retry_times);
            }
            if (mSocketFd > 0) {
                retry_times = 0;
                if(a2dp_notify_call_back){//在唤醒重新连接好后，重新注册音箱回调
                    A2DPNotify(a2dp_notify_call_back);
                }
            } else if (retry_times > 40) {
                ALOGE("socket wrong, retry open");
                retry_times = 0;
                //break;
            }
        }
        while (mSocketFd > 0) {
            memset(recv_buf, 0, BUFFER_SIZE);
            count = recv(mSocketFd, recv_buf, BUFFER_SIZE, MSG_NOSIGNAL);
            if (count <= 0) {
                ALOGE("socket is close ! error=%s(%d)", strerror(errno), errno);
                ::close(mSocketFd);
                mSocketFd = -1;
                break;
            } else {
                /**/
#if 1
                recv_ptr = recv_buf;
                printflag = false;
                do {
                    if (memcmp(btcmdStr[CMD_OPEN_OK_IDX], recv_ptr, strlen(btcmdStr[CMD_OPEN_OK_IDX])) == 0) //"open-ok"
                    {
                        isopenok = true;
                        count -= strlen(btcmdStr[CMD_OPEN_OK_IDX]);
                        recv_ptr += strlen(btcmdStr[CMD_OPEN_OK_IDX]);
                    } else if (memcmp(btcmdStr[CMD_CLOSE_OK_IDX], recv_ptr, strlen(btcmdStr[CMD_CLOSE_OK_IDX])) == 0) //"close-ok"
                    {
                        iscloseok = true;
                        count -= strlen(btcmdStr[CMD_CLOSE_OK_IDX]);
                        recv_ptr += strlen(btcmdStr[CMD_CLOSE_OK_IDX]);
                    } else if (memcmp(btcmdStr[CMD_DISABLE_OK_IDX], recv_ptr, strlen(btcmdStr[CMD_DISABLE_OK_IDX])) == 0) //"disable-ok"
                    {
                        ddisconnect = true;
                        count -= strlen(btcmdStr[CMD_DISABLE_OK_IDX]);
                        recv_ptr += strlen(btcmdStr[CMD_DISABLE_OK_IDX]);
                    } else if (memcmp(btcmdStr[CMD_DISCMPL_IDX], recv_ptr, strlen(btcmdStr[CMD_DISCMPL_IDX])) == 0) //"disccmpl-"
                    {
                        if (disc_call_back != NULL) {
                            disc_call_back(DISC_CMPL_EVT, NULL);
                            disc_call_back = NULL;
                        }
                        count -= strlen(btcmdStr[CMD_DISCMPL_IDX]);
                        recv_ptr += strlen(btcmdStr[CMD_DISCMPL_IDX]);
                    } else if (memcmp(btcmdStr[CMD_A2DP_SOURCE_OPEN_IDX], recv_ptr, strlen(btcmdStr[CMD_A2DP_SOURCE_OPEN_IDX])) == 0) //"a2dp-source-open-"
                    {
                        if (memcmp(btcmdStr[CMD_A2DP_SOURCE_OPEN_OK_IDX], recv_ptr, strlen(btcmdStr[CMD_A2DP_SOURCE_OPEN_OK_IDX])) == 0) //ok
                        {
                            avLinkUp = true;
                            count -= strlen(btcmdStr[CMD_A2DP_SOURCE_OPEN_OK_IDX]);
                            recv_ptr += strlen(btcmdStr[CMD_A2DP_SOURCE_OPEN_OK_IDX]);
                        } else if (memcmp(btcmdStr[CMD_A2DP_SOURCE_OPEN_FAILED_IDX], recv_ptr, strlen(btcmdStr[CMD_A2DP_SOURCE_OPEN_FAILED_IDX])) == 0) //failed
                        {
                            avLinkUp = false;
                            count -= strlen(btcmdStr[CMD_A2DP_SOURCE_OPEN_FAILED_IDX]);
                            recv_ptr += strlen(btcmdStr[CMD_A2DP_SOURCE_OPEN_FAILED_IDX]);
                        }
                        avLinkWait = true;
                    } else if (memcmp(btcmdStr[CMD_A2DP_SOURCE_CLOSE_IDX], recv_ptr, strlen(btcmdStr[CMD_A2DP_SOURCE_CLOSE_IDX])) == 0) //"a2dp-source-close-"
                    {
                        if (memcmp(btcmdStr[CMD_A2DP_SOURCE_CLOSE_OK_IDX], recv_ptr, strlen(btcmdStr[CMD_A2DP_SOURCE_CLOSE_OK_IDX])) == 0) //ok
                        {
                            avLinkDn = true;
                            count -= strlen(btcmdStr[CMD_A2DP_SOURCE_CLOSE_OK_IDX]);
                            recv_ptr += strlen(btcmdStr[CMD_A2DP_SOURCE_CLOSE_OK_IDX]);
                        } else if (memcmp(btcmdStr[CMD_A2DP_SOURCE_CLOSE_FAILED_IDX], recv_ptr, strlen(btcmdStr[CMD_A2DP_SOURCE_CLOSE_FAILED_IDX])) == 0) //failed
                        {
                            avLinkDn = false;
                            count -= strlen(btcmdStr[CMD_A2DP_SOURCE_CLOSE_FAILED_IDX]);
                            recv_ptr += strlen(btcmdStr[CMD_A2DP_SOURCE_CLOSE_FAILED_IDX]);
                        }
                        avLinkDnWait = true;
                    } else if (memcmp(btcmdStr[CMD_DELLINKED_IDX], recv_ptr, strlen(btcmdStr[CMD_DELLINKED_IDX])) == 0) //"dellinked-"
                    {
                        if (memcmp(btcmdStr[CMD_DELLINKED_OK_IDX], recv_ptr, strlen(btcmdStr[CMD_DELLINKED_OK_IDX])) == 0) //"dellinked-ok"
                        {
                            dellink = true;
                            count -= strlen(btcmdStr[CMD_DELLINKED_OK_IDX]);
                            recv_ptr += strlen(btcmdStr[CMD_DELLINKED_OK_IDX]);
                        } else if (memcmp(btcmdStr[CMD_DELLINKED_FAILED_IDX], recv_ptr, strlen(btcmdStr[CMD_DELLINKED_FAILED_IDX])) == 0) //"dellinked-failed"
                        {
                            dellink = false;
                            count -= strlen(btcmdStr[CMD_DELLINKED_FAILED_IDX]);
                            recv_ptr += strlen(btcmdStr[CMD_DELLINKED_FAILED_IDX]);
                        }
                        isdellinked = true;
                    } else if (memcmp(btcmdStr[CMD_LINKINGCMPL_IDX], recv_ptr, strlen(btcmdStr[CMD_LINKINGCMPL_IDX])) == 0) //"linkingcmpl-"
                    {
                        if (linking_call_back != NULL) {
                            linking_call_back(DISC_CMPL_EVT, NULL);
                            linking_call_back = NULL;
                        }
                        count -= strlen(btcmdStr[CMD_LINKINGCMPL_IDX]);
                        recv_ptr += strlen(btcmdStr[CMD_LINKINGCMPL_IDX]);
                    } else if (memcmp(btcmdStr[CMD_LINKING_IDX], recv_ptr, strlen(btcmdStr[CMD_LINKING_IDX])) == 0) //"linking-"
                    {
                        int msglen = strlen(btcmdStr[CMD_LINKING_IDX]);
                        REMOTE_DEVICE msg;
                        if (linking_call_back != NULL) {
                            memset(&msg, 0, sizeof(msg));
                            msg.bd_type = recv_ptr[msglen];
                            memcpy(msg.bd_addr, &recv_ptr[msglen + 1], BD_ADDR_SIZE);
                            msglen += 1 + BD_ADDR_SIZE;
                            strcpy(msg.name, &recv_ptr[msglen]);
                            msglen += strlen(&recv_ptr[msglen]) + 1;
                            //ALOGD("linking- ! name=%s. addr=%s)", msg.name, msg.bd_addr);
                            linking_call_back(DISC_NEW_EVT, &msg);
                        }
                        count -= msglen;
                        recv_ptr += msglen;
                    } else if (memcmp(btcmdStr[CMD_LINKEDCMPL_IDX], recv_ptr, strlen(btcmdStr[CMD_LINKEDCMPL_IDX])) == 0) //"linkedcmpl-"
                    {
                        if (linked_call_back != NULL) {
                            linked_call_back(DISC_CMPL_EVT, NULL);
                            linked_call_back = NULL;
                        }
                        count -= strlen(btcmdStr[CMD_LINKEDCMPL_IDX]);
                        recv_ptr += strlen(btcmdStr[CMD_LINKEDCMPL_IDX]);
                    } else if (memcmp(btcmdStr[CMD_LINKED_IDX], recv_ptr, strlen(btcmdStr[CMD_LINKED_IDX])) == 0) //"linked-"
                    {
                        int msglen = strlen(btcmdStr[CMD_LINKED_IDX]);
                        REMOTE_DEVICE msg;
                        if (linked_call_back != NULL) {
                            memset(&msg, 0, sizeof(msg));
                            msg.bd_type = recv_ptr[msglen];
                            memcpy(msg.bd_addr, &recv_ptr[msglen + 1], BD_ADDR_SIZE);
                            msglen += 1 + BD_ADDR_SIZE;
                            strcpy(msg.name, &recv_ptr[msglen]);
                            msglen += strlen(&recv_ptr[msglen]) + 1;
                            //ALOGD("linked- ! name=%s. addr=%s)", msg.name, msg.bd_addr);
                            linked_call_back(DISC_NEW_EVT, &msg);
                        }
                        count -= msglen;
                        recv_ptr += msglen;
                    } else if (memcmp(btcmdStr[CMD_DISCOVERY_IDX], recv_ptr, strlen(btcmdStr[CMD_DISCOVERY_IDX])) == 0) //"discovery-"
                    {
                        int msglen = strlen(btcmdStr[CMD_DISCOVERY_IDX]);
                        REMOTE_DEVICE msg;
                        if (disc_call_back) {
                            memset(&msg, 0, sizeof(msg));
                            msg.bd_type = recv_ptr[msglen];
                            memcpy(msg.bd_addr, &recv_ptr[msglen + 1], BD_ADDR_SIZE);
                            msglen += 1 + BD_ADDR_SIZE;
                            strcpy(msg.name, &recv_ptr[msglen]);
                            msglen += strlen(&recv_ptr[msglen]) + 1;
                            disc_call_back(DISC_NEW_EVT, &msg);
                        }
                        count -= msglen;
                        recv_ptr += msglen;
#ifdef BLUETOOTH_AG_SUPPORT
                    } else if (memcmp(btcmdStr[CMD_AG_SOURCE_OPEN], recv_ptr, strlen(btcmdStr[CMD_AG_SOURCE_OPEN])) == 0) { //"ag-source-open-"
                        int msglen = strlen(btcmdStr[CMD_AG_SOURCE_OPEN]);
                        if (strncmp(recv_buf, "ok", 2) == 0) {
                            msglen += 2;
                            agLinked = true;
                        } else { //"failed"
                            msglen += strlen("failed");
                        }
                        pthread_cond_signal(&mCondition);
                        count -= msglen;
                        recv_ptr += msglen;
                    } else if (memcmp(btcmdStr[CMD_AG_SOURCE_CLOSE], recv_ptr, strlen(btcmdStr[CMD_AG_SOURCE_CLOSE])) == 0) { //"ag-source-close-"
                        int msglen = strlen(btcmdStr[CMD_AG_SOURCE_CLOSE]);
                        if (strncmp(recv_buf, "ok", 2) == 0) {
                            msglen += 2;
                            agLinked = true;
                        } else {
                            msglen += strlen("failed");
                        }
                        pthread_cond_signal(&mCondition);
                        count -= msglen;
                        recv_ptr += msglen;
#endif
                    } else if (memcmp(btcmdStr[CMD_A2DP_AV_NOTIFY], recv_ptr, strlen(btcmdStr[CMD_A2DP_AV_NOTIFY])) == 0) {
                        int msglen = strlen(btcmdStr[CMD_A2DP_AV_NOTIFY]);
                        int state;
                        if ('0' == recv_ptr[msglen])
                            state = 0;
                        else
                            state = 1;
                        ALOGD("recv  %s,state:%d.", recv_ptr, state);
                        if (a2dp_notify_call_back) {
                            a2dp_notify_call_back(state);
                        }
                        count -= msglen + 1;
                        recv_ptr += msglen + 1;
                    } else if (memcmp(btcmdStr[CMD_A2DP_AV_CHECK_CONNECT], recv_ptr, strlen(btcmdStr[CMD_A2DP_AV_CHECK_CONNECT])) == 0) {
                        int msglen = strlen(btcmdStr[CMD_A2DP_AV_CHECK_CONNECT]);
                        if ('y' == recv_ptr[msglen])
                            mavIsConnected = 1;
                        else
                            mavIsConnected = 0;
                        ALOGD("av_check_connect  %s,state:%d.", recv_ptr, mavIsConnected);
                        pthread_cond_signal(&mAvCondition);
                        //todo send back to caller
                        count -= msglen + 2;
                        recv_ptr += msglen + 2;
                    } else {
                        if (printflag == false) {
                            printPacketInfo("missed BluetoothController::ReadThreadFunc msg", recv_ptr, count, true);
                            printflag = true;
                        }
                        count--;
                        recv_ptr++;
                        //ALOGD("%s() else strstr(recv_buf), recv() count= %d", __func__, count);
                    }
                } while (count > 0);
#else
                if (strstr(recv_buf, "open-ok") == &recv_buf[0]) {
                    pthread_cond_signal(&mCondition);
                } else if (strstr(recv_buf, "close-ok") == &recv_buf[0]) {
                    pthread_cond_signal(&mCondition);
                } else if (strstr(recv_buf, "disable-ok") == &recv_buf[0]) {
                    pthread_cond_signal(&mCondition);
                } else if (strstr(recv_buf, "disccmpl-") == &recv_buf[0]) {
                    disc_call_back(DISC_CMPL_EVT, NULL);
                } else if (strstr(recv_buf, "a2dp-source-open-") == &recv_buf[0]) {
                    if (strstr(recv_buf, "ok") != NULL)
                        avLinked = true;
                    pthread_cond_signal(&mCondition);
                } else if (strstr(recv_buf, "a2dp-source-close-") == &recv_buf[0]) {
                    if (strstr(recv_buf, "ok") != NULL)
                        avLinked = true;
                    pthread_cond_signal(&mCondition);
                } else if (strstr(recv_buf, "discovery-") != NULL) {
                    REMOTE_DEVICE msg;
                    memset(&msg, 0, sizeof(msg));
                    memcpy(msg.bd_addr, &recv_buf[strlen("discovery-")], BD_ADDR_SIZE);
                    strcpy(msg.name, &recv_buf[strlen("discovery-") + BD_ADDR_SIZE]);

                    disc_call_back(DISC_NEW_EVT, &msg);
#ifdef BLUETOOTH_AG_SUPPORT
                } else if (strstr(recv_buf, "ag-source-open-") == &recv_buf[0]) {
                    if (strstr(recv_buf, "ok") != NULL)
                        agLinked = true;
                    pthread_cond_signal(&mCondition);
                } else if (strstr(recv_buf, "ag-source-close-") == &recv_buf[0]) {
                    if (strstr(recv_buf, "ok") != NULL)
                        agLinked = true;
                    pthread_cond_signal(&mCondition);
#endif
                } else {
                    ALOGD("%s() else strstr(recv_buf), recv() count= %d", __func__, count);
                }
#endif
            }
        }
    }

    ALOGV("read thread over \n");
}
