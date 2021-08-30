#ifndef _WIFICONFIG_H_
#define _WIFICONFIG_H_
#include <vector>
#include <string>
#include <wifi_device_config.h>
#include <wifi_error_code.h>

struct NetWorks
{
	int netId;
	char ssid[WIFI_MAX_SSID_LEN];
	int flags;
};

class WifiConfig
{
public:
	WifiConfig(std::string fn);
	~WifiConfig();

	void syncNetwork(std::vector<NetWorks>& network);

	WifiDeviceConfig *getConfig(){
		return wifiConfig;
	}

	unsigned int getConfigCount() {
		return wifiConfigCount;
	}

	void addConfig(const WifiDeviceConfig *config);
	void delConfig(int netId);
	void dumpConfig();
	bool needSyncNetwork()
	{
		return !sync;
	}
private:
	bool sync;
	WifiDeviceConfig *wifiConfig;
	unsigned int wifiConfigCount;
	std::string filename;
	inline uint8_t crc7_byte(uint8_t crc, uint8_t data);
	inline uint8_t crc7(uint8_t crc, uint8_t *buffer, int len);
	void readConfig();
	void saveConfig();

};

#ifndef HILOG_E
#define HILOG_E(...) do{ printf("ERR:");printf(__VA_ARGS__);printf("\n");}while(0)
#endif

#ifndef HILOG_D
#define HILOG_D(...) do{ printf("DBG:");printf(__VA_ARGS__);printf("\n");}while(0)
#endif


#ifndef HILOG_I
#define HILOG_I(...) do{ printf("Info:");printf(__VA_ARGS__);printf("\n");}while(0)
#endif


#ifndef HILOG_W
#define HILOG_W(...) do{ printf("Warn:");printf(__VA_ARGS__);printf("\n");}while(0)
#endif

#endif /* _WIFICONFIG_H_ */
