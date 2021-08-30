#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
extern "C" {
#include <wifi_device.h>
}
#include <console_menu/ConsoleMenu.h>


void OnWifiConnectionChanged(int state, WifiLinkedInfo* info)
{
	std::vector<std::string> wifiState = {"CONNECTED","DISCONNECTED","TEMP_DISABLED","ASSOC_REJECT"};
	if(state < wifiState.size()){
		std::cout << "WifiConnectionChanged: " << wifiState[state] << std::endl;
		std::cout << "ssid: " << info->ssid << std::endl;
		printf("bssid: %02x:%02x:%02x:%02x:%02x:%02x\n",
			   info->bssid[0],info->bssid[1],info->bssid[2],
			   info->bssid[3],info->bssid[4],info->bssid[5]);

		std::cout << "rssi: " << info->rssi << std::endl;
		std::cout << "connState: " << info->connState << std::endl;
		std::cout << "disconnectedReason: " << info->disconnectedReason << std::endl;
		struct in_addr in;
		in.s_addr = info->ipAddress;
		std::cout << "ipAddress: " << inet_ntoa(in) << std::endl;
	}
}

void OnWifiScanStateChanged(int state, int size)
{
	std::vector<std::string> wifiScanState = {"STARTED","RESULTS"};
	std::cout << "WifiScanStateChanged: " << wifiScanState[state] << std::endl;
}

void OnHotspotStateChanged(int state)
{
//TODO:
	printf("No Support\n");
}
void OnHotspotStaJoin(StationInfo* info)
{
//TODO:
	printf("No Support\n");
}
void OnHotspotStaLeave(StationInfo* info)
{
//TODO:
	printf("No Support\n");
}

static WifiEvent event = {
	.OnWifiConnectionChanged = OnWifiConnectionChanged,
	.OnWifiScanStateChanged = OnWifiScanStateChanged,
	.OnHotspotStateChanged = OnHotspotStateChanged,
	.OnHotspotStaJoin = OnHotspotStaJoin,
	.OnHotspotStaLeave = OnHotspotStaLeave
};


struct WifiOpt
{
	WifiScanInfo *info;
	WifiDeviceConfig *config;
	WifiLinkedInfo* linkInfo;
};

int SelectScanInfo(struct WifiOpt *opt,unsigned int size)
{
	ConsoleMenu sel_menu("Select Scan Info",std::cout);
	int curIndex = 0;
	int selIndex = 0;
	std::stringstream title;
	do{
		title.clear();
		title.str("");
		title << "up " << curIndex;
		sel_menu.addMenuItem('u',title.str(),[&selIndex](){selIndex = 10;});

		int count = 0;
		for(int i = 0;i < 10;i++){
			if(curIndex + i < size){
				title.clear();
				title.str("");

				if(opt[curIndex + i].linkInfo){
					if(opt[curIndex + i].linkInfo->connState == WIFI_CONNECTED)
						title << " & ";
					else
						title << "   ";
				} else {
					title << "   ";
				}

				if(opt[curIndex + i].config){
					title << " * ";
				}else{
					title << "   ";
				}

				title << opt[curIndex + i].info->ssid;
				title << " \t ";
				title << opt[curIndex + i].info->rssi;
				sel_menu.addMenuItem('0' + i,title.str(),[&selIndex,i](){
					selIndex = i;
				});
				count++;
			}
		}

		title.clear();
		title.str("");
		title << "down " << size - count - curIndex;
		sel_menu.addMenuItem('n',title.str(),[&selIndex,&curIndex](){
			selIndex = 11;
		});

		sel_menu.addMenuItem('b',"goback",[&selIndex,&curIndex](){
			selIndex = -1;
		});

		sel_menu.display();
		char key;
		std::cin >> key;
		sel_menu.handleKey(key);
		printf("selIndex = %d\n",selIndex);
		if(selIndex == 10){
			int d = curIndex - 10;
			if(d < 0)
				d = 0;
			curIndex = d;
		}else if(selIndex == 11){
			int d = curIndex + 10;
			if(d >= size)
				curIndex = size - 10;
			else
				curIndex = d;
		}
		printf("curIndex = %d\n",curIndex);
	}while(selIndex >= 10);
	if(selIndex < 0)
		return -1;
	return selIndex + curIndex;
}

bool inputIP(const char *title,int& ip,bool check)
{
	char sip[20];
	struct in_addr addr;
	int ret;
	do{
		memset(sip,0,sizeof(sip));
		std::cout << title << ":";
		std::cin >> sip;
		if((sip[0] | sip[1]) == 0)
			return false;
		ret = inet_aton(sip,&addr);
		if(ret == 0)
		{
			if(check)
				std::cout << "input error." << std::endl;
			else
				ip = 0;
		}else
			ip = addr.s_addr;
	}while(ret == 0 && check);
	return true;
}

bool InputWifiDeviceConfig(WifiDeviceConfig& config)
{
	std::cout << "ssid: " << config.ssid << std::endl;
	std::string pwd;
	std::cout << "pwd: ";
	std::cin >> pwd;
	strncpy(config.preSharedKey,pwd.c_str(),WIFI_MAX_KEY_LEN);

	std::cout << "dhcp(y/n):";
	char dhcp[10];
	std::cin.clear();
	std::cin >> dhcp;
	config.ipType = DHCP;
	if(dhcp[0] != 'y')
	{
		bool ret = inputIP("IP",config.staticIp.ipAddress,true);
		if(!ret)
			return false;

		ret = inputIP("GATEWAY",config.staticIp.gateway,true);
		if(!ret)
			return false;

		ret = inputIP("NETMASK",config.staticIp.netmask,true);
		if(!ret)
			return false;

		ret = inputIP("DNS0",config.staticIp.dnsServers[0],true);
		if(!ret)
			return false;
		inputIP("DNS1",config.staticIp.dnsServers[1],false);
		config.ipType = STATIC_IP;
	}
	std::cin.clear();
	return true;
}

int SelecSetOperator(struct WifiOpt *wifiopt)
{
	std::string title = wifiopt->info->ssid;
	title += " operator";
	ConsoleMenu sel_opt(title, std::cout);
	int select = -1;
	sel_opt.addMenuItem('r', "reConnect", [&select](){select = 1;});
	sel_opt.addMenuItem('c', "Config",    [&select](){select = 2;});
	sel_opt.addMenuItem('d', "Delete",    [&select](){select = 3;});
	sel_opt.addMenuItem('x', "goback",    [&select](){select = -1;});
	char key;
	sel_opt.display();
	std::cin >> key;
	sel_opt.handleKey(key);
	return select;
}

bool WifiConfig(WifiScanInfo *info)
{
	WifiDeviceConfig config;
	bool ret = false;
	strcpy(config.ssid,info->ssid);
	memcpy(config.bssid,info->bssid,WIFI_MAC_LEN);
	config.securityType = info->securityType;
	config.freq = info->frequency;
	config.wapiPskType = WIFI_PSK_TYPE_ASCII;

	int sel = InputWifiDeviceConfig(config);
	if(sel >= 0){
		int res;
		printf("add config %s %s\n",config.ssid,info->ssid);
		ret = AddDeviceConfig(&config,&res);
		if(ret == WIFI_SUCCESS){
			std::cout << "conntect to " << res << std::endl;
			ret = ConnectTo(res);
			if(ret != WIFI_SUCCESS){
				std::cerr << "Connect " << res << " failed!" << std::endl;
			}else{
				ret = true;
			}
		}else {
			std::cerr << "AddDeviceConfig " << res << " failed!" << std::endl;
		}
	}
	return ret;
}

void WifiDisconnect()
{
	WifiErrorCode ret = Disconnect();
	if(ret != WIFI_SUCCESS){
		std::cerr << "Disconnect Error!" << std::endl;
	}
}

void WifiSetting()
{
	WifiErrorCode ret = Scan();
	if(ret == WIFI_SUCCESS){

		WifiScanInfo info[50];
		unsigned int size = 50;

		ret = GetScanInfoList(info,&size);

		WifiDeviceConfig wifiConfig[50];
		unsigned int configCount = 50;

		if(GetDeviceConfigs(wifiConfig,&configCount) != WIFI_SUCCESS){
			configCount = 0;
			std::cerr << "GetDeviceConfigs failed!" << std::endl;
		}

		WifiLinkedInfo linkInfo;
		WifiLinkedInfo *pLinkInfo = &linkInfo;

		if(GetLinkedInfo(&linkInfo) != WIFI_SUCCESS){
			pLinkInfo = NULL;
			std::cerr << "GetLinkedInfo failed!" << std::endl;
		}

		struct WifiOpt wifiopt[50];
		for(int i = 0;i < size;i++){
			wifiopt[i].info = &info[i];
			wifiopt[i].config = NULL;
			wifiopt[i].linkInfo = NULL;
			for(int j = 0;j < configCount;j++)
			{
				if(strcmp(info[i].ssid,wifiConfig[j].ssid) == 0){
					wifiopt[i].config = &wifiConfig[j];
					break;
				}
			}
			if(pLinkInfo){
				if(strcmp(info[i].ssid,pLinkInfo->ssid) == 0){
					wifiopt[i].linkInfo = pLinkInfo;
				}
			}
		}
		int sel = 0;
		while(sel >= 0){
			sel = SelectScanInfo(wifiopt,size);
			if(sel >= 0){
				if(wifiopt[sel].linkInfo){
					std::cout << "are you disconnect ? (y/n):";
					char c;
					std::cin.clear();
					std::cin >> c;
					if(c == 'y'){
						WifiDisconnect();
					}
					sel = -1;
				}else if(wifiopt[sel].config){
					int sel_opt = SelecSetOperator(&wifiopt[sel]);
					if(sel_opt == 1){
						int res = wifiopt[sel].config->netId;
						WifiErrorCode r = ConnectTo(res);
						if(r != WIFI_SUCCESS){
							std::cerr << "Connect " << res << " failed!" << std::endl;
						}
						sel = -1;
					}else if(sel_opt == 2){
						WifiConfig(wifiopt[sel].info);
						sel = -1;
					}else if(sel_opt == 3){
						if(RemoveDevice(wifiopt[sel].config->netId) != WIFI_SUCCESS)
						{
							std::cerr << "Remove Device ID "  << wifiopt[sel].config->netId << std::endl;
						}
						sel = -1;
					}else{
						sel = 0;
					}
				}else {
					std::cout << "are you Connect ? (y/n):";
					char c;
					std::cin.clear();
					std::cin >> c;
					if(c == 'y'){
						WifiConfig(wifiopt[sel].info);
					}
					sel = -1;
				}
			}
		}
	}else{
		std::cerr << "Wifi Scan failed!" << std::endl;
	}
	std::cin.clear();
}

int main(int argc, char *argv[])
{
	WifiErrorCode ret = RegisterWifiEvent(&event);
	unsigned char wifiMac[6];
	GetDeviceMacAddress(wifiMac);
	printf("Mac %02x:%02x:%02x:%02x:%02x:%02x\n",
		   wifiMac[0],wifiMac[1],wifiMac[2],
		   wifiMac[3],wifiMac[4],wifiMac[5]);

	ConsoleMenu wifi_menu("Wifi Opterator Menu", std::cout);
	int select = 0;
	wifi_menu.addMenuItem('e', "EnableWifi", [&select](){EnableWifi(); select = 1;});
	bool app_running = true;
	wifi_menu.addMenuItem('x', "Exit", [&app_running]() { app_running = false; });
	wifi_menu.display();
	char key;
	while (app_running) {
		std::cin >> key;
		wifi_menu.handleKey(key);
		switch(select){
		case 1:
			wifi_menu.eraseMenuItemWithKey('e');
			wifi_menu.addMenuItem('d', "DisableWifi", [&select](){DisableWifi();select = 2;});
			wifi_menu.addMenuItem('s', "Setting", [](){WifiSetting();});
			break;
		case 2:
			wifi_menu.eraseMenuItemWithKey('d');
			wifi_menu.eraseMenuItemWithKey('s');
			wifi_menu.addMenuItem('e', "EnableWifi", [&select](){EnableWifi(); select = 1;});
			break;
		}
		wifi_menu.display();
	}
	ret = UnRegisterWifiEvent(&event);
    return 0;
}
