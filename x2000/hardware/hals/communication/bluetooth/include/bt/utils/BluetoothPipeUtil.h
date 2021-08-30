#ifndef PIPE_UTIL_H_
#define PIPE_UTIL_H_

#include <stdio.h>
#include <pthread.h>
#include <string>
#include <utils/RefBase.h>

#define PIPE_NAME "/usr/data/bt_fifo"
#define PIPE_BUF_QUIT "q"
#define PIPE_BUF_INCALL_HANGUP "0"
#define PIPE_BUF_INCALL_ANSWER "1"
#define PIPE_BUF_BT_UNBIND "2"
#define PIPE_BUF_RTSP_START "3"
#define PIPE_BUF_RTSP_STOP "4"
#define PIPE_BUF_RTSP_ERROR "5"
#define PIPE_BUF_RTMP_START "6"
#define PIPE_BUF_RTMP_STOP "7"
#define PIPE_BUF_RTMP_ERROR "8"

using namespace std;

namespace android {
class PipeUtil{
public:
    static PipeUtil* getInstance();
    static string strlowwer(string str);
    virtual ~PipeUtil();
private:
    PipeUtil();
    static PipeUtil* sInstance;
    pthread_t mReadPipeThread;
    bool mDone;
    bool mRtmpStart;
    string mBtName;
    static void *ReadPipeThreadWrapper(void *me);
    void readPipeThreadFunc();
    int createPipe();
    int writePipe(const char* buf);
    void handleEvent(char *buffer);
    string getIpAddress();
};
}  // namespace android

#endif  // PIPE_UTIL_H_
