#ifndef __WIFISERVICE_H__
#define __WIFISERVICE_H__
#include <thread>
#include <list>
#include <mutex>

#include "wifiConfig.hpp"
#include "wifiInterface.hpp"

class WifiService: public WifiInterface
{
public:
	WifiService();
	virtual ~WifiService();
	static WifiInterface *Instance();



	virtual WifiErrorCode EnableWifi(void);
	virtual WifiErrorCode DisableWifi(void);
	virtual int IsWifiActive(void);
	virtual WifiErrorCode Scan(void);
	virtual WifiErrorCode GetScanInfoList(WifiScanInfo* result, unsigned int* size);

	virtual WifiErrorCode GetLinkedInfo(WifiLinkedInfo* result);
	virtual WifiErrorCode RegisterWifiEvent(WifiEvent* event);
	virtual WifiErrorCode UnRegisterWifiEvent(const WifiEvent* event);
	virtual WifiErrorCode GetDeviceMacAddress(unsigned char* result);
	virtual WifiErrorCode AdvanceScan(WifiScanParams *params);

	virtual WifiErrorCode AddDeviceConfig(const WifiDeviceConfig* config, int* result);
	virtual WifiErrorCode GetDeviceConfigs(WifiDeviceConfig* result, unsigned int* size);
	virtual WifiErrorCode RemoveDevice(int networkId);
	virtual WifiErrorCode ConnectTo(int networkId);
	virtual WifiErrorCode Disconnect(void);
private:
	static WifiInterface *wifiInterface;
	static std::mutex mutex;
	std::mutex evMutex;
	std::string startWith(const char *s,int len,std::string key);

	int wpaCtrlCommand(struct wpa_ctrl *ctrl, const char *cmd, char *data,int size);
	int wpaCtrlRecv(struct wpa_ctrl *ctrl,char* data,size_t size);
	static void wpaCliMsgCb(char *msg, size_t len);
	struct wpa_ctrl *ctrl;
	struct wpa_ctrl *event;
	bool openConnection();
	void closeConnection();
	std::thread* eventThread;
	volatile int evStop;
	std::list<WifiEvent*> wifiEvents;
	void eventHandle();

	void ByteToHexStr(const unsigned char* source, char* dest, int sourceLen);

	void Hex2Str( const char *sSrc,  char *sDest, int nSrcLen);
	void HexStrToByte(const char* source, char* dest, int sourceLen);
	int utf8HexlStringToCharString(const char *src, char *destBuf);
	int scanMultiBytes(const char *pSrc);

	std::string ssidToUtf8(std::string& ssid);
	bool parserNetworkLine(char* sline,NetWorks& net);

	bool ListNetworks(std::vector<NetWorks> &nets);
	void RemoveNetWorks(int netId);
	void eventDispatch(char *msg,int len);
	std::string get_default_ifname();
	int readBufferLine(const char *data,int sz,std::string& s);

	bool parserScanInfoLine(char* sline,WifiScanInfo* result);
	WifiConfig *wConfig;
	bool addNetWorks(const WifiDeviceConfig* config,int *result);
	bool saveNetWorks();
	std::string ifName;
	char* trimspace(char* s);
	WifiErrorCode getLinkedInfo(WifiLinkedInfo* result);
	void systemExec(std::string cmd);
	void ipalloc(char *ssid);
};
#endif /* __WIFISERVICE_H__ */
