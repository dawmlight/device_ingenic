#ifndef HilinkModule_H_
#define HilinkModule_H_

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <utils/RefBase.h>
#include <pthread.h>
#include "SyncData.h"  

namespace android {
 /**
  * @class HilinkModule HilinkModule.h
  * @brief 蓝牙数据交互接口
  * 包括：发送数据，接收数据，获取绑定状态.
  */
/*enum BIND_TYPE{
	NONE,
	BLE,
	BLUETOOTH,
};*/

class HilinkModule{
 public:
   /** 
    * @brief 构造函数
    * @param moduleName module名 长度必须小于等于15
    */
    HilinkModule(const string & name);

   /** 
    * @brief 放送数据给对端设备 
    * @param data 需要发送的数据
	* @param size 发送数据的长度
    */
    int send(const unsigned char *buf, unsigned int len);


   /** 
    * @brief 析构函数
    */
    virtual ~HilinkModule();

   /** 
    * @brief 接收对端数据的回调函数
    * @param buf 接收到的数据
    * @param len 数据长度
    */
    virtual void onRetrive(const unsigned char *buf, unsigned int len)=0;

   /** 
    * @brief 获取当前蓝牙的绑定状态
    * @return true已绑定  false未绑定
    */
    bool isBinded(){return mIsBinded;};

   /** 
    * @brief 获取当前蓝牙使用的类型
    * @return BLUETOOTH 普通蓝牙.  BLE 低功耗蓝牙. NONE 未绑定
    */
    BIND_TYPE getBindType();

   /** 
    * @brief 获取设备的蓝牙MAC地址
    * @return bool true 获取成功 地址保存在形参addr中
    *              false 获取失败 
    */
    bool getBluetoothAddress(string & address);

 private:

    bool stopSocketClient();
    bool startSocketClient();
    bool isOpened();
    bool restartSocketClient(int retry_times);
    static void *ReadThreadWrapper(void *me);
    void ReadThreadFunc();

    bool mIsBinded;
    BIND_TYPE mBindType;
    int mSocketFd;
    pthread_t mReadThread;
    string mModuleName;
    char mAddress[20];
    HilinkModule* mModule;
    bool isregisterOK;
    bool mNeedPrintData;
    volatile char isSenddone;
    //pthread_cond_t mCondition;
    //pthread_mutex_t mMutex;
    pthread_mutex_t mSendLock;
};
}
#endif  // HilinkModule_H_
