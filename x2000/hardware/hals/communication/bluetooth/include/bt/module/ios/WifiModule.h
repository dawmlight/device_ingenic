#ifndef WIFI_MODULE_H_
#define WIFI_MODULE_H_

#include <stdio.h>
#include <protocol/SyncModule.h>
#include "WpaInterface.h"

namespace android {
class WifiModule : public SyncModule {
 public:
     static WifiModule* getInstance(RefBase* protocol);
     virtual ~WifiModule();
 private:
    WifiModule(RefBase* protocol);
    static WifiModule* sInstance;
    WpaInterface* mWpaIf = NULL;
    char* mConnectedSsid = NULL;
    char* mIpAddr = NULL;
    string mIOSSSID;
    void onRetrive(const sp<SyncData> & data);
    int openWifi();
    int closeWifi();
    static void wifiStateCallBack(WIFI_STATE event);
    int connectWifi(const char* ssid, const char* pwd);
    int getWifiStatus();
    int onWifiStateChanged(int event);
    int sendWifiState();
    int sendWifiConnectSuccess();
    int sendWifiApState();
    int sendConnectIPhoneApState();
    void sendWifiList();
};
}
#endif  // WIFI_MODULE_H_
