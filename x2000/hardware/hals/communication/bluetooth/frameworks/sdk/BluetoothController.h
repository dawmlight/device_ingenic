 /**
 * @defgroup wireless WireLess模块
*/
 /**
 * @defgroup Bluetooth
 * @ingroup wireless
 * @brief 蓝牙交互模块
 *
 * IMCP使用的是Broadcom Corporation （博通公司）提供的BSA蓝牙协议栈,相比blueZ ,BSA 具有更好的交互性和可读性。便于客户快速基于相关profile做自己的业务处理
 *
 * @section about 相关库和头文件
 * 使用此接口需要引用的库为libBluetoothSDK.so 
 *
 * 使用此接口需要引用的头文件为 ThirdPartyModule.h  SyncData.h Type.h BluetoothController.h
 *
 * 相关的类说明 
 * BluetoothController类提供了蓝牙控制接口，如开关蓝牙，解除蓝牙绑定，接打电话。
 * ThirdPartyModule类提供了蓝牙的数据传输接口。
 * SyncData类定义了ThirdPartyModule收发蓝牙数据的数据类型。

 * @section function Bluetooth模块的基本功能
 * @subsection 开关Bluetooth
 *  
 *IMCP有两种配置蓝牙开关的方式，
 * 一种是在版级配置文件里设置ro.bluetooth.open.enable,如果设置为true ，说明设备烧录后第一次开机蓝牙自动开启
 * 如果此参数设置为false ，设备烧录后第一次开机，蓝牙默认关闭状态，需要用户通过第二种方式开启。
 * 配置文件的具体路径在/device/版级/config/device.conf
 * 
 * 第二种方式就是通过sdk的接口类BluetoothController。开启示例如下：
 *
 * @code
 * BluetoothController* bc = BluetoothController::getInstance();
 * bc->open(); //开启蓝牙
 * bc->close(); //关闭蓝牙
 * @endcode
 *
 * @attention 无论哪种开启方式。IMCP数据库会记录上一次开机时蓝牙状态。例如当前蓝牙为开启状态，那么设备重启机后，蓝牙会自动开启。
 * 且蓝牙开启后会一直对外可见,直到与一个手机设备绑定。
 *
 * @subsection Bluetooth扫描
 *
 * @code
 * //开启扫描
 * BluetoothController* bc = BluetoothController::getInstance();
 * bc->start_discovery(discCallBack);
 * void discCallBack(DISC_EVT event, REMOTE_DEVICE *p_data){
 *  switch(event) {
 *  case DISC_NEW_EVT:{ //扫描到新蓝牙设备
 *      //p_data->name为蓝牙名称, p_data->bd_addr为蓝牙MAC地址
 *	ALOGE("\tName:%s----Bdaddr:%s", p_data->name, p_data->bd_addr);	
 *  }
 *  case DISC_CMPL_EVT:{ // 扫描完成
 *	ALOGE("Discovery complete");
 *	break;
 *  }
 *   default:
 *	ALOGE("disc_cback unknown event:%d", event);
 *	break;
 *   }
 * }
 * @endcode
 *
 * @code
 * //停止扫描
 * BluetoothController* bc = BluetoothController::getInstance();
 * bc->stop_discovery(); 
 * @endcode
 *
 * @subsection Bluetooth配对
 *
 * @code
 * BluetoothController* bc = BluetoothController::getInstance();
 * bc->creat_bond("test"); //请求与蓝牙名称为test的设备进行配对
 * bc->remove_bond("test"); //取消与蓝牙名称为test的设备配对
 * @endcode
 *
 * @subsection Bluetooth绑定和数据交互
 * IMCP基于SPP协议(Serial Port Profile),对蓝牙绑定和数据传输做了一套自己的实现机制:
 * 1.使用IMCP的设备只做server端，对端设备做client端。绑定请求只能由对端设备发起
 * 2.绑定唯一性：IMCP某一时刻只与一个蓝牙设备绑定。如一个手机与此设备绑定后，IMCP 蓝牙会对外不可见，而且不再响应其他手机设备的绑定请求
 * 3.重连机制：当前设备与手机设备绑定后，由于某种原因（手机与设备超出有效距离，或者一端设备重启）断开。两天设备会时刻保持不断重连探测，直到再次重连成功。
 * 4.数据交互对象SyncData:IMCP定义了自己的蓝牙模块SyncModule和蓝牙数据类型SyncData。
 * SyncModule是蓝牙数据交互的接口，当手机设备和当前IMCP设备绑定后，双方都定义并注册了同名的SyncModule后。那么两台设备就建立好了数据交互的通路。用户可以把需要发送的信息封装成SyncData,通过SyncMole发送给对方的同名SyncModule。同时用户也通过SyncModule接收来自对方的同名SyncModule的信息.
 * IMCP提供了两套SDK用于蓝牙链接和数据交互。一套手机端SDK:Android_sync_framework.jar用于Android手机设备 IOS_sync_framwork.so 用于Iphone手机设备。IMCP_SDK里的libbluetoothSDK.so即可以对接Android手机，也可以对接iphone手机

 * @subsection connectdemo 蓝牙连接示例代码
 * @code
 *  //Android手机端代码示例
 *       IntentFilter filter = new IntentFilter();
 *       filter.addAction(DefaultSyncManager.RECEIVER_ACTION_STATE_CHANGE);
 *       registerReceiver(mBluetoothReceiver, filter);
 * 
 *       private final BroadcastReceiver mBluetoothReceiver = new BroadcastReceiver() {
 *         @Override
 *         public void onReceive(Context context, Intent intent) {
 *             if (DefaultSyncManager.RECEIVER_ACTION_STATE_CHANGE.equals(intent.getAction())) {
 * 
 *               int state = intent.getIntExtra(DefaultSyncManager.EXTRA_STATE,
 *                       DefaultSyncManager.IDLE);
 *               boolean isConnect = (state == DefaultSyncManager.CONNECTED) ? true : false;
 *               if (isConnect) {
 *                   String addr = mManager.getLockedAddress();
 *                   if (addr.equals("")) {
 * 		        //对端设备已经绑定到其他手机设备
 *                       mManager.disconnect();
 *                   } else {
 * 		        //链接成功
 *                       mManager.setLockedAddress(addr);
 *                   }
 *               } else {
 * 		        //链接失败
 *               }
 *           }
 *     }
 * 
 *     private DefaultSyncManager mManager;
 *     mManager = DefaultSyncManager.getDefault();	
 *     if(mManager.isConnect()) {
 *        //已经处于绑定状态
 *     }else{
 *         mManager.connect(device.getAddress()); //device为之前扫描出来的且已经配对过的BluetootDevice,device的扫描和配对可以参考android API或者 Android_sync_demo
 *     }
 * 
 *   private void unBond() {
 *       //解除绑定
 *       new Thread(new Runnable() {
 *           @Override
 *           public void run() {
 *               try {
 *                   mManager.setLockedAddress("", true);
 *                   try {
 *                       Thread.sleep(1000);
 *                   } catch (Exception e) {
 *                   }
 *                   mManager.disconnect();
 *               } catch (Exception e) {
 *                   e.printStackTrace();
 *               }
 *           }
 *       }).start();
 *   }
 * @endcode
 *
 * IOS手机端代码示例请参考IOSSyncDemo
 * IMCP设备端的蓝牙响应配对和绑定动作由系统内部BluetoothManager服务完成，对外不可见。
 * IMCP设备端可以在ThirdPartyModule内的bool isBinded()获取到蓝牙绑定状态变化。
 *
 * @subsection 数据交互
 *
 * @code
 *  //Android手机端代码示例
 *   public class SyncDataTestModule extends SyncModule{
 * 	public static String MODULE_NAME = "M_test"; // 长度必须小于15
 * 
 * 	private SyncDataTestModule(Context context) {
 * 		super(MODULE_NAME, context);
 * 	}
 * 
 * 	@Override
 * 	protected void onRetrive(SyncData data) {
 * 		super.onRetrive(data);
 * 		String msg = data.getString("msg_res");	
 * 		Log.v("收到对端设备数据:"+msg); //I'm a robot
 * 	}
 * 
 * 	protected boolean sendSyncData() {
 * 		SyncData data = new SyncData();
 * 		data.putString("msg_type", "who are you ?");
 * 		try {
 * 			send(data);
 * 		} catch (SyncException e) {
 * 			Log.e(TAG, "---send sync failed:" + e);
 * 		}
 * 		return true;
 * 	}
 * 
 * 	SyncDataTestModule module = new SyncDataTestModule();
 *       if (module.isConnected()) {
 *           module.sendSyncData();
 *       } else {
 *           showFailedDialog("蓝牙连接失败");
 *       }
 *
 * @endcode
 *
 * @code
 *  //IMCP端代码示例
 *  class SyncDataTestModule : public ThirdPartyModule ;
 *  static const string MODULE_NAME = "M_test";// 长度必须小于15
 * 
 *  SyncDataTestModule::SyncDataTestModule() 
 *      :ThirdPartyModule(MODULE_NAME){
 *        //Note:not do any task in here
 *  }
 * 
 *  void SyncDataTestModule::onRetrive(const sp<SyncData> & data)
 *  {
 *      String msg = data->getString("msg_type");
 *   
 *      if(strcmp(msg.c_str(),"who are you ?")) {
 *   	sendRetriveToPhone();
 *      }
 *  }
 *   
 *  void SyncDataTestModule::sendRetriveToPhone(){
 *     if(isBinded() == true){
 *   	sp<SyncData> syncdata = new SyncData();
 *   	syncdata->putString("msg_res","I'm a robot");
 *   	bool ret = send(syncdata);
 *   	if(ret == false)
 *   	    ALOGE("send data failed");
 *     }else{
 *     	ALOGV("bt is un bind");
 *     }
 *  }

 * @endcode
 * @attention SyncModule说明:1. IMCP设备端和手机端注册的Module的名字(MODULE_NAME)必须相同.
 *                          2. Module的名字长度要小于15
 *                          3. BLE通信时，数据包的长度不能超过150，否则会发送失败.IMCP_SDK里使用int getSyncDataSize(const sp<SyncData> & data)来获取要发送的SyncData大小。IOS手机端使用XXX来获取要发送的SyncData大小。
 *
 * @subsection 蓝牙耳机
 * IMCP支持HFP(Hands-free Profile)。当IMCP蓝牙开启后，用户可以像连接其他蓝牙耳机方式一样，搜索并连接此设备。
 * 当IMCP做蓝牙耳机时，如果来电，对于接听或者挂掉的操作，IMCP在版级按键处理模块做了对接。
 * @remarks  xxxxxxxxx
 * 
 *
 * @subsection A2DP
 * IMCP支持A2DP(Advanced Audio Distribution Profile)。并且支持两种角色audio source 和 audio sink。发送音频流的设备是source，接收音频流的设备是sink，比如手机是source，蓝牙耳机是sink。 但是两种角色不能共存。就是用户的产品只能选择做其中一种角色。
 *
 * A2DP sink : IMCP做sink端时，和蓝牙耳机的使用方式相同。当IMCP蓝牙开启后，用户可以像连接其他蓝牙耳机方式一样，搜索并连接此设备。
 * A2DP source :
 * @remarks xxxxxxxxx
 *
 * @subsection BLE
 * IMCP支持蓝牙低功耗（BLE），当手机设备为IOS时。蓝牙的连接和数据交互使用的就是BLE。
 * 使用ble进行蓝牙绑定和数据传输时。IMCP_SDK使用和普通蓝牙相同的ThirdSyncModule接口和数据类型SyncData 。
 * IOS手机端的SDK为xxx
 *
 * @subsection datademo IOS示例代码
 * @code
 * @endcode
 *
 * @subsection 休眠
 * IMCP下的蓝牙，在没有数据传输时会自动进入休眠状态，以节省功耗。当有数据交互时，蓝牙会自动唤醒。
 *
 */

#ifndef BLUETOOTH_CONTROLLER_H_
#define BLUETOOTH_CONTROLLER_H_

#include <stdio.h>
#include <string>
#include "BluetoothUtils.h"

/**
 * @file
 * IMCP 蓝牙控制接口
 */
using std::string;
class BluetoothController{
public:
    static BluetoothController* getInstance();

    virtual ~BluetoothController();
    /**
     * @fn bool open()
     *
     * 打开蓝牙.
     *
     * @retval true 成功.
     * @retval false 失败.
     *
     */
    bool open();
    /**
     * @fn bool close()
     *
     * 关闭蓝牙.
     *
     * @retval true 成功.
     * @retval false 失败.
     *
     */
    bool close();

    /**
     * @fn bool isOpened()
     *
     * 判断当前蓝牙是否已经开启.
     *
     * @retval true 已经开启 .
     * @retval false 未开启.
     *
     */
    bool isOpened();

    /**
     * @fn bool setBleBeacon(int state)
     *
     * 设置蓝牙华为beacon帧状态.
     *
     * @param state 0:unreg beacon, 1:reg beacon , else: turn off
     * @retval true 成功.false 失败.
     *
     */
    bool setBleBeacon(int state);

    /**
     * @fn bool setBluetoothVisibility(bool enable)
     *
     * 设置蓝牙可见性.
     *
     * @param enable true 开启蓝牙可见性. false 关闭蓝牙可见性
     * @retval true 成功.false 失败.
     *
     */
    bool setBluetoothVisibility(bool enable);
    /**
     * @fn bool disconnect()
     *
     * 断开与绑定设备的连接.
     *
     * @retval true 成功.
     * @retval false 失败.
     *
     */
    bool disconnect();

    /**
     * @fn bool A2DPSouceLinkUp(const BD_ADDR addr)
     *
     * 媒体音频连接.
     *
     * @param addr 对端设备的蓝牙mac地址
     * @retval true 成功.false 失败.
     *
     */
    bool A2DPSouceLinkUp(const BD_ADDR addr);

    /**
     * @fn bool A2DPSouceLinkDown()
     *
     * 断开媒体音频连接.
     *
     * @retval true 成功.false 失败.
     *
     */
    bool A2DPSouceLinkDown();

    /**
     * @fn bool HeadSetAnswerCall()
     *
     * 接听电话.
     *
     * @retval true 成功.false 失败.
     *
     */
    bool HeadSetAnswerCall();

    /**
     * @fn bool HeadSetHangUp()
     *
     * 关掉或拒接电话.
     *
     * @retval true 成功.false 失败.
     *
     */
    bool HeadSetHangUp();

    /**
     * @fn bool startDiscovery()
     *
     * 开始扫描附近的蓝牙设备
     * type 0: all,else audio type
     * @retval true 成功.
     * @retval false 失败.
     *
     */

    bool startDiscovery(void (*disc_back)(DISC_EVT event, REMOTE_DEVICE *p_data), int duration=8, int type=0);

    /**
     * @fn bool getLinked()
     *
     * 获取连接过的蓝牙设备列表
     *
     * @retval true 成功.
     * @retval false 失败.
     *
     */
    bool getLinked(void (*disc_back)(DISC_EVT event, REMOTE_DEVICE *p_data), int duration=8);
    /**
     * @fn bool dellinked()
     *
     * 从连接过的设备列表里删除指定地址的蓝牙设备
     *
     * @retval true 成功.
     * @retval false 失败.
     *
     */
    bool dellinked(const BD_ADDR addr);
    /**
     * @fn bool getLinking()
     *
     * 获取连接中的蓝牙设备列表
     *
     * @retval true 成功.
     * @retval false 失败.
     *
     */
    bool getLinking(void (*disc_back)(DISC_EVT event, REMOTE_DEVICE *p_data), int duration=8);
    /**
     * @fn bool stopDiscovery()
     *
     * 停止扫描附近的蓝牙设备
     *
     * @retval true 成功.
     * @retval false 失败.
     *
     */    
    bool stopDiscovery();

    /**
     * @fn bool creatBond()
     *
     * 主动请求与某一设备的蓝牙配对
     * 
     * @parm   设备名称
     * @retval true 成功.
     * @retval false 失败.
     *
     */

    bool creatBond(const string & bd_name);

    /**
     * @fn bool removeBond()
     *
     * 取消与某一设备的蓝牙配对
     * 
     * @parm   设备名称
     * @retval true 成功.
     * @retval false 失败.
     *
     */
    bool removeBond(const string & bd_name);

    bool A2DPSendVolume(int volume=80);
    bool A2DPSocketReady();

	bool HFPAGLinkUp(const BD_ADDR addr);

	bool HFPAGLinkDown();

	bool AGOpenAudio();

	bool AGCloseAudio();

    bool A2DPNotify(void (*a2dp_callback)(int state));
    bool A2DPCheckConnect(int &is_connect);
private:
    BluetoothController();
    static BluetoothController* sInstance;


    bool stopSocketClient();
    bool startSocketClient();
	void printPacketInfo(const char* title, const char* buf, int size,bool realy);
    bool restartSocketClient(int retry_times);
    static void *ReadThreadWrapper(void *me);
    void ReadThreadFunc();
    int waitTrue(bool *isTrue, int timeout);
    int mavIsConnected;
    int mSocketFd;
    pthread_t mReadThread;

    pthread_cond_t mCondition;
    pthread_cond_t mAvCondition;
    pthread_condattr_t mAvConditionAttr;
    pthread_mutex_t mMutex;

};

#endif //BLUETOOTH_CONTROLLER_H_
