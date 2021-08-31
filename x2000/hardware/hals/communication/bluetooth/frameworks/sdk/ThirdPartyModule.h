#ifndef THIRDPARTYMODULE_H_
#define THIRDPARTYMODULE_H_

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
//#include <utils/RefBase.h>
#include <pthread.h>
#include "SyncData.h"

namespace android {
 /**
  * @class ThirdPartyModule ThirdPartyModule.h
  * @brief 蓝牙数据交互接口
  * 包括：发送数据，接收数据，获取绑定状态.
  */


class ThirdPartyModule{
 public:
   /**
    * @brief 构造函数
    * @param moduleName module名 长度必须小于等于15
    */
    ThirdPartyModule(const string & name);

   /**
    * @brief 放送数据给对端设备
    * @param data 需要发送的数据 类型为SyncData
    * @return true 发送成功 false 发送失败
    */
    bool send(const sp<SyncData> & data);

   /**
    * @brief 获取syncdata数据的实际长度
    * @param data 需要发送的数据 类型为SyncData
    * @return int syncdata数据的实际长度
    */
    int getSyncDataSize(const sp<SyncData> & data);

   /**
    * @brief 析构函数
    */
    virtual ~ThirdPartyModule();

   /**
    * @brief 接收对端数据的回调函数
    * @param data 接收到的数据 类型为SyncData
    */
    virtual void onRetrive(const sp<SyncData> & data)=0;

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
    ThirdPartyModule* mModule;
    bool isregisterOK;
    bool mNeedPrintData;
    volatile char isSenddone;
    //pthread_cond_t mCondition;
    //pthread_mutex_t mMutex;
    pthread_mutex_t mSendLock;
};
}
#endif  // THIRDPARTYMODULE_H_
