#ifndef _WIFIINTERFACE_H_
#define _WIFIINTERFACE_H_

enum WIFI_STATE{
	CONNECTED,
	DISCONNECTED,
	TEMP_DISABLED,
	ASSOC_REJECT,
};
enum WIFI_SCAN_STATE{
	STARTED,
	RESULTS,
};

class WifiInterface
{
public:
	WifiInterface(){}
	~WifiInterface(){}
	//Sta
	virtual WifiErrorCode EnableWifi(void) = 0;
	virtual WifiErrorCode DisableWifi(void) = 0;
	virtual int IsWifiActive(void)  = 0;
	virtual WifiErrorCode Scan(void)  = 0;
	virtual WifiErrorCode GetScanInfoList(WifiScanInfo* result, unsigned int* size) = 0;

	virtual WifiErrorCode GetLinkedInfo(WifiLinkedInfo* result) = 0;
	virtual WifiErrorCode RegisterWifiEvent(WifiEvent* event)  = 0;
	virtual WifiErrorCode UnRegisterWifiEvent(const WifiEvent* event)  = 0;
	virtual WifiErrorCode GetDeviceMacAddress(unsigned char* result)  = 0;
	virtual WifiErrorCode AdvanceScan(WifiScanParams *params)  = 0;

	virtual WifiErrorCode AddDeviceConfig(const WifiDeviceConfig* config, int* result)  = 0;
	virtual WifiErrorCode GetDeviceConfigs(WifiDeviceConfig* result, unsigned int* size)  = 0;
	virtual WifiErrorCode RemoveDevice(int networkId)  = 0;
	virtual WifiErrorCode ConnectTo(int networkId) = 0;
	virtual WifiErrorCode Disconnect(void) = 0;
};

#endif /* _WIFIINTERFACE_H_ */
