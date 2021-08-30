#include <iostream>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/poll.h>
#include <cstring>
#include <sys/types.h>
#include <dirent.h>
#include <sstream>
#include <wpa_ctrl.h>
#include <fstream>

extern "C"{
#include <wifi_device.h>
}


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "wifiService.hpp"

static constexpr char ctrl_iface_dir[] = "/var/run/wpa_supplicant";
static constexpr char wifiInfo[] = "wifi.info";
WifiInterface *WifiService::wifiInterface = NULL;
std::mutex WifiService::mutex;
WifiInterface *WifiService::Instance()
{
	std::lock_guard<std::mutex> lck(mutex);
	if(wifiInterface == NULL)
		wifiInterface = new WifiService();
	return wifiInterface;
}



WifiService::WifiService()
{
	wConfig = new WifiConfig(std::string(wifiInfo));
	evStop = 0;
}

WifiService::~WifiService()
{
	delete wConfig;
}

void WifiService::ByteToHexStr(const unsigned char* source, char* dest, int sourceLen)
{
    short i;
    unsigned char highByte, lowByte;

    for (i = 0; i < sourceLen; i++)
    {
        highByte = source[i] >> 4;
        lowByte = source[i] & 0x0f ;

        highByte += 0x30;

        if (highByte > 0x39)
			dest[i * 2] = highByte + 0x07;
        else
			dest[i * 2] = highByte;

        lowByte += 0x30;
        if (lowByte > 0x39)
            dest[i * 2 + 1] = lowByte + 0x07;
        else
            dest[i * 2 + 1] = lowByte;
    }
}

void WifiService::Hex2Str( const char *sSrc,  char *sDest, int nSrcLen )
{
    int  i;
    char szTmp[3];

    for( i = 0; i < nSrcLen; i++ )
    {
        sprintf( szTmp, "%02X", (unsigned char) sSrc[i] );
		sDest[i * 2] = szTmp[0];
		sDest[i * 2 + 1] = szTmp[1];
    }
}

void WifiService::HexStrToByte(const char* source, char* dest, int sourceLen)
{
	short i;
	unsigned char highByte, lowByte;

	for (i = 0; i < sourceLen; i += 2)
	{
		highByte = toupper(source[i]);
		lowByte  = toupper(source[i + 1]);

		if (highByte > 0x39)
			highByte -= 0x37;
		else
			highByte -= 0x30;

		if (lowByte > 0x39)
			lowByte -= 0x37;
		else
			lowByte -= 0x30;

		dest[i / 2] = (highByte << 4) | lowByte;
	}
}

int WifiService::utf8HexlStringToCharString(const char *src, char *destBuf)
{
	int i = 0;
	int j = 0;
	int utf8 = 0;

	while(src[j] != '\0')
	{
		if(strncmp(&src[j], "\\x", 2) != 0 && strncmp(&src[j], "\\X", 2) != 0 )
		{
			destBuf[i] = src[j];
			i = i + 1;
			j = j + 1;
		}
		else
		{
			utf8 = 1;
			HexStrToByte(&src[j+2], &destBuf[i], 2);
			i = i+1;
			j = j+4;
		}
	}
	destBuf[i] = '\0';

	return utf8;
}


std::string WifiService::ssidToUtf8(std::string& ssid)
{
	char *s = new char[ssid.size() * 2];
	utf8HexlStringToCharString(ssid.c_str(),s);
	std::string str = s;
	delete[] s;
	return str;
}

int WifiService::scanMultiBytes(const char *pSrc)
{
	std::size_t i;
	for (i = 0; i < strlen(pSrc); i++)
	{
		if (*(pSrc+i) & 0x80)
			return 1;
	}
	return 0;
}
void WifiService::systemExec(std::string cmd)
{
	int rc = system(cmd.c_str());
	if (WIFSIGNALED(rc) &&
		(WTERMSIG(rc) == SIGINT || WTERMSIG(rc) == SIGQUIT))
	{
		HILOG_E("ERROR: exec %s error! %s",cmd.c_str(),strerror(errno));
	}
}

void WifiService::ipalloc(char *ssid)
{
	WifiDeviceConfig *config = wConfig->getConfig();
	unsigned int count = wConfig->getConfigCount();
	int dhcp = 1;
	std::string ip;
	std::string netmask;
	std::string gateway;
	std::string dns0;
	std::string dns1;
	for(unsigned int i = 0;i < count;i++){
		if(strcmp(config[i].ssid,ssid) == 0){
			if(config[i].ipType == STATIC_IP){
				struct in_addr in;
				if(config[i].staticIp.ipAddress == 0){
					HILOG_E("STATIC IP failed! 0x%08x\n",config[i].staticIp.ipAddress);
					break;
				}
				in.s_addr = config[i].staticIp.ipAddress;
				ip = inet_ntoa(in);

				if(config[i].staticIp.gateway == 0){
					HILOG_E("STATIC GATEWAY failed! 0x%08x\n",config[i].staticIp.gateway);
					break;
				}

				in.s_addr = config[i].staticIp.gateway;
				gateway = inet_ntoa(in);

				if(config[i].staticIp.netmask == 0){
					HILOG_E("STATIC NETMASK failed! 0x%08x\n",config[i].staticIp.netmask);
					break;
				}
				in.s_addr = config[i].staticIp.netmask;
				netmask = inet_ntoa(in);

				if(config[i].staticIp.dnsServers[0] == 0){
					HILOG_E("STATIC DNS0 failed! 0x%08x\n",config[i].staticIp.dnsServers[0]);
					break;
				}

				in.s_addr = config[i].staticIp.dnsServers[0];
				dns0 = inet_ntoa(in);

				if(config[i].staticIp.dnsServers[1] == 0){
					dns1 = "";
				}else{
					in.s_addr = config[i].staticIp.dnsServers[1];
					dns1 = inet_ntoa(in);
				}
				dhcp = 0;
			}
			break;
		}
	}
	if(dhcp){
		std::string cmd = "wifiproxy.sh dhcp " + ifName;
		systemExec(cmd);
	}else{
		std::string cmd = "wifiproxy.sh dhcp";
		systemExec(cmd);
		cmd = "ifconfig " + ifName + " " + ip + " netmask " + netmask;
		systemExec(cmd);
		cmd = "route add default gw " + gateway;
		systemExec(cmd);
		std::ofstream out("/tmp/resolv.conf");

		out << "nameserver ";
		out << dns0 << std::endl;

		if(dns1.size() > 1){
			out << "nameserver ";
			out << dns0 << std::endl;
		}
	}
}

void WifiService::eventDispatch(char *msg,int len)
{
	std::lock_guard<std::mutex> lck(mutex);
	typedef std::pair<enum WIFI_STATE,std::string> WifiState;
	int state;

	const std::vector<WifiState> connectedString = {
		{CONNECTED,"CTRL-EVENT-CONNECTED"},
		{DISCONNECTED,"CTRL-EVENT-DISCONNECTED"},
		{TEMP_DISABLED,"CTRL-EVENT-SSID-TEMP-DISABLED"},
		{ASSOC_REJECT,"CTRL-EVENT-ASSOC-REJECT"}
	};
	typedef std::pair<enum WIFI_SCAN_STATE,std::string> WifiScanState;
	const std::vector<WifiScanState> scanString = {
		{STARTED,"CTRL-EVENT-SCAN-STARTED"},
		{RESULTS,"CTRL-EVENT-SCAN-RESULTS"}
	};
	int i = 0;
	for(;i < connectedString.size();i++){
		const WifiState& st = connectedString[i];
		if(strstr(msg,st.second.c_str())){
			state = (int)st.first;
			break;
		}
	}

	if(i < connectedString.size()){
		for(auto it = wifiEvents.begin();it != wifiEvents.end();it++){
			if((*it)->OnWifiConnectionChanged){

				WifiLinkedInfo result;
				getLinkedInfo(&result);
				if(state == 0){
					ipalloc(result.ssid);
				}
				mutex.unlock();
				(*it)->OnWifiConnectionChanged(state,&result);
				mutex.lock();
			}

		}
		return;
	}

	i = 0;
	for(;i < scanString.size();i++){
		const WifiScanState& st = scanString[i];
		if(strstr(msg,st.second.c_str())){
			state = (int)st.first;
			break;
		}
	}

	if(i <  scanString.size()){
		for(auto it = wifiEvents.begin();it != wifiEvents.end();it++){
			if((*it)->OnWifiScanStateChanged){
				mutex.unlock();
				(*it)->OnWifiScanStateChanged(state,0);
				mutex.lock();
			}
		}
	}

}

void WifiService::eventHandle()
{
	int res;
	int ctrlfd = wpa_ctrl_get_fd(event);
	if(ctrlfd < 0){
		HILOG_E("Error ctrlfd = %d %s", ctrlfd,strerror(errno));
		return;
	}
	struct pollfd rfds[2];
	memset(rfds, 0, 2 * sizeof(struct pollfd));

	rfds[0].fd = ctrlfd;
	rfds[0].events |= POLLIN | POLLPRI;

	while(evStop == 0){
		res = poll(rfds, 1, -1);
		if (res < 0) {
			HILOG_E("Error poll = %d\n", res);
			return;
		}
		evMutex.lock();
		if(evStop == 0){
			while (wpa_ctrl_pending(event) > 0) {
				char buf[4096];
				size_t len = sizeof(buf) - 1;
				if (wpa_ctrl_recv(event, buf, &len) == 0) {
					buf[len] = '\0';
					eventDispatch(buf,len);
				} else {
					HILOG_E("Could not read pending message.\n");
					break;
				}
			}
		}
		evMutex.unlock();
	}
	evStop = 2;
}

std::string WifiService::get_default_ifname()
{
	std::string ifname;
	std::fstream in("/proc/net/wireless");
	char data[4096];
	in.read(data,4096);
	char* d = data;
	std::string str;
	int l = readBufferLine(d,4096,str);
	d += l;
	str.clear();
	l = readBufferLine(d,4096,str);
	d += l;
	str.clear();
	l = readBufferLine(d,4096,str);
	std::size_t found = str.find(":");

	if(found != std::string::npos){
		str = str.substr(0,found);
		strcpy(data,str.c_str());
		d = trimspace(data);
		ifname = d;
	}else
		ifname = "wlan0";
	in.close();
	return ifname;
}

bool WifiService::openConnection()
{

	if(ifName.size() == 0)
	{
		ifName = get_default_ifname();
	}
	std::string ifname = ifName;
	std::string socketname = std::string(ctrl_iface_dir) + "/" + ifname;

	int timeout = 30;

	ctrl = NULL;
	event = NULL;
	while(--timeout){
		if(access(socketname.c_str(),F_OK) == 0){
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	if(timeout == 0){
		HILOG_E("access %s timeout",socketname.c_str());
		return false;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ctrl = wpa_ctrl_open(socketname.c_str());
    if (ctrl == NULL) {
		HILOG_E("ctrl open %s failed!,%s",socketname.c_str(),strerror(errno));
		goto end;
    }

	event = wpa_ctrl_open(socketname.c_str());
    if (event == NULL) {
		HILOG_E("event open %s failed!,%s",socketname.c_str(),strerror(errno));
		goto end;
    }

	if (wpa_ctrl_attach(event)) {
		HILOG_E("Error: Failed to attach to wpa_supplicant.");
		goto end;
    }
	return true;
end:
	if(ctrl){
		wpa_ctrl_close(ctrl);
	}
	if(event){
		wpa_ctrl_close(event);
	}

    return false;
}

void WifiService::closeConnection()
{
	if(ctrl){
		wpa_ctrl_close(ctrl);
	}
	if(event){
		wpa_ctrl_detach(event);
		wpa_ctrl_close(event);
	}
}

void WifiService::wpaCliMsgCb(char *msg, size_t len)
{
    HILOG_D("%s\n", msg);
}

int WifiService::wpaCtrlCommand(struct wpa_ctrl *ctrl, const char *cmd, char *data,int size)
{
    size_t len = size;
    int ret;
	// HILOG_D("%s %s\n", __FUNCTION__,cmd);
    if (ctrl == NULL) {
		HILOG_E("Not connected to wpa_supplicant - command dropped.\n");
		return -1;
    }
	int timeout = 10;
	while(--timeout)
	{
		memset(data,0,size);
		ret = wpa_ctrl_request(ctrl, cmd, strlen(cmd), data, &len,WifiService::wpaCliMsgCb);
		if(ret >= 0)
			break;
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	if (ret == -2) {
		HILOG_E("'%s' command timed out.\n", cmd);
		return -2;
	} else if (ret < 0) {
		HILOG_E("'%s' command failed.\n", cmd);
		return -1;
	}
	data[len] = 0;
	// HILOG_D("data = %s ==== len: %d\n",data,len);
    return len;
}

int WifiService::wpaCtrlRecv(struct wpa_ctrl *ctrl,char* data,size_t size)
{
	if(wpa_ctrl_pending(ctrl)){
		int ret = wpa_ctrl_recv(ctrl,data,&size);
		if(ret < 0)
			return ret;
	}else
		return 0;
	return size;
}

std::string WifiService::startWith(const char *s,int len,std::string key)
{
	const char *d = s;
	while(len){
		std::string str;
	   	int l = readBufferLine(d,len,str);

		std::size_t found1 = str.find(key);
		if(found1 == 0)
		{
			found1 += key.size();
			std::string sret = str.substr(found1);
			return sret;
		}
		if(l == 0)
			break;
		len -= l;
		d += l;
	}
	return "";
}

int WifiService::readBufferLine(const char *data,int sz,std::string& s)
{
	int i;
	for( i = 0;i < sz;i++) {
		if(data[i] == '\n') {
			return  i + 1;
		}
		if(data[i] == 0)
			return -1;
		s += data[i];
	}
	return 0;
}

bool WifiService::parserScanInfoLine(char* sline,WifiScanInfo* result)
{
	char delims[] = "	";
	char *str = std::strtok(sline, delims);
	int index = 0;
	while(str != 0){
		switch(index){
		case 0:
		{
			unsigned char *mac = result->bssid;
			sscanf((const char*)str,"%02x:%02x:%02x:%02x:%02x:%02x",
				   &mac[0],&mac[1],&mac[2],&mac[3],&mac[4],&mac[5]);
			break;
		}
		case 1:
			result->frequency = atoi(str);
			break;
		case 2:
			result->rssi = atoi(str);
			break;
		case 3:
		{
			char *s0 = strstr(str,"WPA-PSK");
			char *s1 = strstr(str,"WPA2-PSK");
			char *s2 = strstr(str,"WEP");
			if(s0 && s1){
				result->securityType = WIFI_SEC_TYPE_SAE;
			}else if(s0 || s1){
				result->securityType = WIFI_SEC_TYPE_PSK;
			}else if(s2){
				result->securityType = WIFI_SEC_TYPE_WEP;
			}else {
				result->securityType = WIFI_SEC_TYPE_OPEN;
			}
			break;
		}

		case 4:
		{
			std::string str1 = str;
			std::string s = ssidToUtf8(str1);
			strncpy(result->ssid, s.c_str(), WIFI_MAX_SSID_LEN);
			break;
		}
		default:
			HILOG_E("no parse: %s\n",str);
		}
		index++;
		str = std::strtok(NULL, delims);
	}

	if(index > 4)
		return true;
	return false;
}

WifiErrorCode WifiService::EnableWifi(void)
{
	std::lock_guard<std::mutex> lck(mutex);
	if(ifName.size() == 0)
		ifName = get_default_ifname();
	std::string cmd = "wifiproxy.sh up " + ifName;
	system(cmd.c_str());
	evStop = 0;
	openConnection();
	eventThread = new std::thread(&WifiService::eventHandle,this);
	eventThread->detach();

	return WIFI_SUCCESS;
}

WifiErrorCode WifiService::DisableWifi(void)
{
	std::lock_guard<std::mutex> lck(mutex);

	evStop = 1;
	evMutex.lock();
	closeConnection();
	evMutex.unlock();
	system("wifiproxy.sh down wlan0");
	while(evStop != 2){
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	return WIFI_SUCCESS;
}


int WifiService::IsWifiActive(void)
{
	std::lock_guard<std::mutex> lck(mutex);
	int ret = WIFI_STA_NOT_ACTIVE;
	char *data = (char*)malloc(4096);
	int r = wpaCtrlCommand(ctrl,"STATUS",data,4096);
    if(r < 0){
		HILOG_E("ERROR:STATUS command failed!\n");
		free(data);
		return ret;
	}

	std::string state = startWith(data,r,"wpa_state=");
	std::string mode = startWith(data,r,"mode=");

	if(mode == "station"){
		if(state == "COMPLETED")
			ret = WIFI_STA_ACTIVE;
	}
    // ap mode TODO:
	free(data);
	return ret;
}

WifiErrorCode WifiService::Scan(void)
{
	std::lock_guard<std::mutex> lck(mutex);
	char data[4096];
	WifiErrorCode ret = ERROR_WIFI_UNKNOWN;
	memset(data, 0, 4096);
    int r = wpaCtrlCommand(ctrl, "SCAN",data,4096);
    if(r < 0){
	    return ret;
    }
	if(strstr(data,"OK")){
		ret = WIFI_SUCCESS;
	}
	if(strstr(data,"FAIL-BUSY")){
		ret = ERROR_WIFI_BUSY;
	}
	return ret;
}

WifiErrorCode WifiService::GetScanInfoList(WifiScanInfo* result, unsigned int* size)
{
	std::lock_guard<std::mutex> lck(mutex);
	char data[8192];
	int count = *size;
	WifiErrorCode ret = ERROR_WIFI_UNKNOWN;
	int headLen = 0;
	int id = 0;
    int r = wpaCtrlCommand(ctrl, "SCAN_RESULTS",data,4096);
	if(r < 0){
	    return ret;
    }
	headLen += r;
	char* d = data;
	int i = 0;
	int head = 1;
	while(1){
		std::string sline;
		int len = readBufferLine(d,headLen,sline);
		if(len == 0){
			memcpy(data,d,headLen);
			d = &data[headLen];
			memset(d,0,4096);
			int len = wpaCtrlRecv(ctrl,d,4096);
			if(len == 0)
				break;
			headLen += len;
			d = data;
			continue;
		}else if(len < 0){
			break;
		}
		headLen -= len;
		d += len;
		if(head){
			head = 0;
		}else{
			char *s = new char[sline.size() + 1];
			std::strcpy(s, sline.c_str());
			if(parserScanInfoLine(s,&result[id])){
				int search = 0;
				for(int i = 0;i < id;i++){
					if(strcmp(result[id].ssid,result[i].ssid) == 0){
						search = 1;
						break;
					}
				}
				if(search == 0){
					id++;
					if(id >= count){
						delete[] s;
						break;
					}
				}
			}
			delete[] s;
		}
	}
	*size = id;
	return WIFI_SUCCESS;
}

char* WifiService::trimspace(char* s)
{
	char *s0 = s;
	while(*s){
		if(*s != ' '){
			s0 = s;
			break;
		}
		s++;
	}
	while(*s++);
	s -=1;
	while(s != s0){
		if(*s != ' ' && *s != '\n' && *s != '\r')
			break;
		s--;
	}
	*(s+1) = 0;
	return s0;
}

bool WifiService::parserNetworkLine(char* sline,NetWorks& net)
{
	char delims[] = "\t";
	char *ss = new char[strlen(sline) + 1];
	char *str = strtok(sline, delims);

	int index = 0;
	memset(&net,0,sizeof(NetWorks));
	while(str != 0){
		strcpy(ss,str);
		char *s = trimspace(ss);
		switch(index){
		case 0:
			net.netId = strtoul(s,NULL,0);
			break;
		case 1:
		{
			std::string ussid = s;
			std::string ssid = ssidToUtf8(ussid);
			strcpy(net.ssid,ssid.c_str());
			break;
		}
		case 2:
			break;
		case 3:
			if(strstr(s,"CURRENT"))
				net.flags = 1;
			else if(strstr(s,"DISABLED"))
				net.flags = 2;
			else if(strstr(s,"ENABLED"))
				net.flags = 3;
			break;
		default:
			HILOG_E("no parse: %s\n",str);
		}
		index++;
		str = strtok(NULL, delims);
	}
	delete[] ss;
	if(index >= 1)
		return true;
	return false;
}


bool WifiService::ListNetworks(std::vector<NetWorks> &nets)
{
    char data[4096];
	int ret = wpaCtrlCommand(ctrl, "LIST_NETWORKS",data,4096);
    if(ret < 0){
		HILOG_E("ERROR:LIST_NETWORKS command failed!\n");
		return false;
    }

	int headLen = ret;
	std::string s;
	char *d = data;
	int len = readBufferLine(d,headLen,s);
	headLen -= len;
	d += len;

	while(headLen){
		s.clear();
		len = readBufferLine(d,headLen,s);
		if(len == 0)
			break;
		NetWorks net;
		char *sline = new char[s.size() + 1];
		strcpy(sline,s.c_str());
		if(parserNetworkLine(sline,net))
			nets.push_back(net);
		delete[] sline;
		headLen -= len;
		d += len;
	}
	return true;
}

void WifiService::RemoveNetWorks(int netId)
{
	char data[4096];
	char cmd[32];
	snprintf(cmd,32,"REMOVE_NETWORK %d",netId);
	int ret = wpaCtrlCommand(ctrl, cmd,data,4096);
	if(ret < 0){
		HILOG_E("ERROR:REMOVE_NETWORK command failed! netId:%d\n",netId);
	}
	if(strncmp(data,"OK",2) != 0){
		HILOG_E("ERROR:REMOVE_NETWORK failed!");
	}

}

bool WifiService::addNetWorks(const WifiDeviceConfig* config,int *result)
{
	char data[4096];
	std::stringstream cmd;
	int ret = wpaCtrlCommand(ctrl, "ADD_NETWORK",data,4096);
	if(ret < 0){
		HILOG_E("ERROR:ADD_NETWORK command failed!");
		return false;
	}

	int netId = atoi(data);
	std::string ssid;
	cmd << "SET_NETWORK " << netId << " ssid \"" << config->ssid << "\"";
	ret = wpaCtrlCommand(ctrl, cmd.str().c_str(),data,4096);
	if(ret < 0){
		HILOG_E("ERROR:SET_NETWORK ssid(%s) command failed!",config->ssid);
		return false;
	}

	if(strncmp(data,"OK",2) != 0){
		HILOG_E("ERROR:SET_NETWORK ssid(%s) failed!",config->ssid);
		return false;
	}

	cmd << "SET_NETWORK " << netId << " KEY_MGMT ";
	switch(config->securityType){
	case WIFI_SEC_TYPE_OPEN:
		cmd << "NONE";
		break;
	case WIFI_SEC_TYPE_WEP:
		cmd << "NONE";
		ret = wpaCtrlCommand(ctrl, cmd.str().c_str(),data,4096);
		if(ret < 0){
			HILOG_E("ERROR:SED_NETWORK WIFI_SEC_TYPE_WEP command failed!");
			return false;
		}
		if(strncmp(data,"OK",2) != 0){
			HILOG_E("ERROR:SED_NETWORK WIFI_SEC_TYPE_WEP failed!");
			return false;
		}
		cmd.clear();
		cmd.str("");
		cmd << "SET_NETWORK " << netId << " wep_key0 ";
		if(config->wapiPskType == WIFI_PSK_TYPE_HEX)
			cmd << config->preSharedKey;
		else if(config->wapiPskType == WIFI_PSK_TYPE_ASCII)
			cmd << "\"" << config->preSharedKey << "\"";
		else{
			HILOG_E("ERROR: WIFI_SEC_TYPE_WEP: wapiPskType is not support!");
			return false;
		}


		ret = wpaCtrlCommand(ctrl, cmd.str().c_str(),data,4096);
		if(ret < 0){
			HILOG_E("ERROR:SET_NETWORK key command failed!");
			return false;
		}

		if(strncmp(data,"OK",2) != 0){
			HILOG_E("ERROR:SET_NETWORK key failed!");
			return false;
		}

		cmd.clear();
		cmd.str("");
		cmd << "SET_NETWORK " << netId << " wep_tx_keyidx 0";
		break;
	case WIFI_SEC_TYPE_PSK:
	case WIFI_SEC_TYPE_SAE:
		cmd.clear();
		cmd.str("");
		cmd << "SET_NETWORK " << netId << " psk ";
		if(config->wapiPskType == WIFI_PSK_TYPE_HEX)
			cmd << config->preSharedKey;
		else if(config->wapiPskType == WIFI_PSK_TYPE_ASCII)
			cmd << "\"" << config->preSharedKey << "\"";
		else{
			HILOG_E("ERROR: WIFI_SEC_TYPE_WEP: wapiPskType is not support!");
			return false;
		}
		break;
	default:
		HILOG_E("ERROR: securityType(%d) is not support!\n",config->securityType);
		return false;
	}

	ret = wpaCtrlCommand(ctrl, cmd.str().c_str(),data,4096);
	if(ret < 0){
		HILOG_E("ERROR:SET_NETWORK key command failed!");
		return false;
	}

	if(strncmp(data,"OK",2) != 0){
		HILOG_E("ERROR:SET_NETWORK key failed!");
		return false;
	}
	cmd.clear();
	cmd.str("");
	cmd << "ENABLE_NETWORK " << netId;
	ret = wpaCtrlCommand(ctrl, cmd.str().c_str(),data,4096);
	if(ret < 0){
		HILOG_E("ERROR:ENABLE_NETWORK key command failed!");
		return false;
	}

	if(strncmp(data,"OK",2) != 0){
		HILOG_E("ERROR:ENABLE_NETWORK key failed!");
		return false;
	}

	*result = netId;
	return true;
}

bool WifiService::saveNetWorks()
{
	char data[4096];
	std::stringstream cmd;
	int ret = wpaCtrlCommand(ctrl, "SAVE_CONFIG",data,4096);
	if(ret < 0){
		HILOG_E("ERROR:SAVE_CONFIG command failed!");
		return false;
	}

	if(strncmp(data,"OK",2) != 0){
		HILOG_E("ERROR:SAVE_CONFIG failed!");
		return false;
	}
	return true;
}

WifiErrorCode WifiService::AddDeviceConfig(const WifiDeviceConfig* config, int* result)
{
	std::lock_guard<std::mutex> lck(mutex);
	std::vector<NetWorks> nets;
	ListNetworks(nets);
	int netid = -1;
	for(int i = 0;i < nets.size();i++){
		if(strcmp(nets[i].ssid,config->ssid) == 0){
			netid = nets[i].netId;
			break;
		}
	}
	if(netid != -1){
		RemoveNetWorks(netid);
		wConfig->delConfig(netid);
	}
	bool ret = addNetWorks(config,result);
	if(!ret)
		return ERROR_WIFI_UNKNOWN;

	if(saveNetWorks()){
		if(wConfig->needSyncNetwork()){
			nets.clear();
			ListNetworks(nets);
			wConfig->syncNetwork(nets);
		}
		wConfig->addConfig(config);
		return WIFI_SUCCESS;
	}
	return ERROR_WIFI_UNKNOWN;
}

WifiErrorCode WifiService::GetDeviceConfigs(WifiDeviceConfig* result, unsigned int* size)
{
	std::lock_guard<std::mutex> lck(mutex);
	int count = std::min(*size,wConfig->getConfigCount());
	std::vector<NetWorks> nets;
	if(wConfig->needSyncNetwork()){
		if(ListNetworks(nets)){
			wConfig->syncNetwork(nets);
		}else{
			return ERROR_WIFI_BUSY;
		}
	}
	memcpy(result,wConfig->getConfig(),count * sizeof(WifiDeviceConfig));
	*size = count;
	return WIFI_SUCCESS;

}

WifiErrorCode WifiService::RemoveDevice(int networkId)
{
	std::lock_guard<std::mutex> lck(mutex);
	RemoveNetWorks(networkId);
	wConfig->delConfig(networkId);
	return WIFI_SUCCESS;
}

WifiErrorCode WifiService::ConnectTo(int networkId)
{
	std::lock_guard<std::mutex> lck(mutex);
	char data[4096];
	std::stringstream cmd;
	WifiErrorCode ret = WIFI_SUCCESS;
	cmd << "SELECT_NETWORK " << networkId;
	int r = wpaCtrlCommand(ctrl, cmd.str().c_str(),data,4096);
	if(r < 0){
		HILOG_E("ERROR:SELECT_NETWORK command failed!");
		ret = ERROR_WIFI_INVALID_ARGS;

	}else if(strncmp(data,"OK",2) != 0){
		HILOG_E("ERROR:SELECT_NETWORK %d failed!",networkId);
		ret = ERROR_WIFI_INVALID_ARGS;
	}
	return ret;
}

WifiErrorCode WifiService::Disconnect(void)
{
	std::lock_guard<std::mutex> lck(mutex);
	char data[4096];
	std::string cmd;
	WifiErrorCode ret = WIFI_SUCCESS;
	cmd = "DISCONNECT";
	int r = wpaCtrlCommand(ctrl, cmd.c_str(),data,4096);
	if(r < 0){
		HILOG_E("ERROR:DISCONNECT command failed!");
		ret = ERROR_WIFI_UNKNOWN;

	}else if(strncmp(data,"OK",2) != 0){
		HILOG_E("ERROR:DISCONNECT failed!");
		ret = ERROR_WIFI_UNKNOWN;
	}
	return ret;
}

WifiErrorCode WifiService::getLinkedInfo(WifiLinkedInfo* result)
{

	WifiErrorCode ret = ERROR_WIFI_UNKNOWN;
	char data[4096];

	int r = wpaCtrlCommand(ctrl, "STATUS",data,4096);
    if(r < 0){
		HILOG_E("ERROR:STATUS command failed!\n");
	}else{
		std::string ussid = startWith(data,r,"ssid=");
		std::string ssid = ssidToUtf8(ussid);
		std::string bssid = startWith(data,r,"bssid=");
		std::string state = startWith(data,r,"wpa_state=");
		std::string ipaddr = startWith(data,r,"ip_address=");
		int r = wpaCtrlCommand(ctrl, "SIGNAL_POLL",data,4096);
		if(r < 0){
			HILOG_E("ERROR:STATUS command failed!\n");
		}else{
			std::string rssi = startWith(data,r,"RSSI=");

			if(ipaddr.size() > 0 && state == "COMPLETED")
			{
				result->connState = WIFI_CONNECTED;
			}else{
				result->connState = WIFI_DISCONNECTED;
			}

			strcpy(result->ssid,ssid.c_str());
			unsigned char* mac = result->bssid;

			sscanf((const char*)bssid.c_str(),"%02x:%02x:%02x:%02x:%02x:%02x",
				   &mac[0],&mac[1],&mac[2],&mac[3],&mac[4],&mac[5]);

			result->rssi = strtol(rssi.c_str(),0,0);

			strcpy(result->ssid,ssid.c_str());
			struct in_addr addr;
			inet_aton(ipaddr.c_str(),&addr);
			result->ipAddress = addr.s_addr;
			ret = WIFI_SUCCESS;
		}
	}
	return ret;
}

WifiErrorCode WifiService::GetLinkedInfo(WifiLinkedInfo* result)
{
	std::lock_guard<std::mutex> lck(mutex);
	getLinkedInfo(result);
}

WifiErrorCode WifiService::RegisterWifiEvent(WifiEvent* event)
{
	std::lock_guard<std::mutex> lck(mutex);
	wifiEvents.push_back(event);
	return WIFI_SUCCESS;
}

WifiErrorCode WifiService::UnRegisterWifiEvent(const WifiEvent* event)
{
	std::lock_guard<std::mutex> lck(mutex);
	WifiErrorCode ret = ERROR_WIFI_INVALID_ARGS;
	for(auto it = wifiEvents.begin();it != wifiEvents.end();it++)
	{
		if(event == *it){
			wifiEvents.erase(it);
			ret = WIFI_SUCCESS;
			break;
		}
	}
	return ret;
}

WifiErrorCode WifiService::GetDeviceMacAddress(unsigned char* result)
{
	std::lock_guard<std::mutex> lck(mutex);
	if(ifName.size() == 0)
		ifName = get_default_ifname();
	std::string fn = "/sys/class/net/" + ifName + "/address";
	std::ifstream sys(fn.c_str());
	std::string s;
	WifiErrorCode ret = ERROR_WIFI_UNKNOWN;
	if(sys.good()){
		sys >> s;
		unsigned char* mac = result;
		sscanf((const char*)s.c_str(),"%02x:%02x:%02x:%02x:%02x:%02x",
			   &mac[0],&mac[1],&mac[2],&mac[3],&mac[4],&mac[5]);

		sys.close();
		ret = WIFI_SUCCESS;
	}
	return ret;
}

WifiErrorCode WifiService::AdvanceScan(WifiScanParams *params)
{
	std::lock_guard<std::mutex> lck(mutex);
	return ERROR_WIFI_NOT_SUPPORTED;
}
