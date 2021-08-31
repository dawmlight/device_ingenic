/*****************************************************************************
 **
 **  Name:           Main.cpp
 **
 *****************************************************************************/
//#define LOG_NDEBUG 0
#define LOG_TAG "BtMain"

#include "wifi/WpaInterface.h"
#include "wifi_device.h"
#include "bsa_ble_api.h"
#include "BtMain.h"
#include "ohos_bt_gatt.h"
#include "ohos_bt_gatt_server.h"
#include <SettingsDB.h>
#include <utils/Log.h>

#ifdef __cplusplus
extern "C"
{
#endif
extern int app_mgr_get_addr(char * addr);
#ifdef __cplusplus
}
#endif

#define APP_BLE_SERVICE_IS_PRIMARY 1   //is primary
#define APP_BLE_SERVICE_NUM_HANDLE 6
typedef unsigned char   UINT8;
static UINT8 RSP_DEV_NAME[32] = "Hi-VIATON-2DDZ00";
static UINT8 MANU_DATA[] = {0x01, 0x31, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
////#define APP_BLE_REGISTER_UUID 0xe400
////#define APP_BLE_REGISTER_UUID2 0xe500
///*
////dev info
////"15f1e400-a277-43fc-a484-dd39ef8a9100"
//static UINT8 HILINK_SERVICE_DEV_INFO[MAX_UUID_SIZE] = {0x00, 0x91, 0x8a, 0xef, 0x39, 0xdd, 0x84, 0xa4, 0xfc, 0x43, 0x77, 0xa2, 0x00, 0xe4, 0xf1, 0x15};
////"15f1e401-a277-43fc-a484-dd39ef8a9100"
//static UINT8 HILINK_CHAR_DEV_INFO[MAX_UUID_SIZE] = {0x00, 0x91, 0x8a, 0xef, 0x39, 0xdd, 0x84, 0xa4, 0xfc, 0x43, 0x77, 0xa2, 0x01, 0xe4, 0xf1, 0x15};
//*/
////batch read
////"15f1e600-a277-43fc-a484-dd39ef8a9100"
static UINT8 HILINK_SERVICE_RW[MAX_UUID_SIZE] = {0x00, 0x91, 0x8a, 0xef, 0x39, 0xdd, 0x84, 0xa4, 0xfc, 0x43, 0x77, 0xa2, 0x00, 0xe6, 0xf1, 0x15};
////"15f1e601-a277-43fc-a484-dd39ef8a9100"
static UINT8 HILINK_CHAR_DATA_READ[MAX_UUID_SIZE] = {0x00, 0x91, 0x8a, 0xef, 0x39, 0xdd, 0x84, 0xa4, 0xfc, 0x43, 0x77, 0xa2, 0x01, 0xe6, 0xf1, 0x15};
////"15f1e602-a277-43fc-a484-dd39ef8a9100"
static UINT8 HILINK_CHAR_DATA_WRITE[MAX_UUID_SIZE] = {0x00, 0x91, 0x8a, 0xef, 0x39, 0xdd, 0x84, 0xa4, 0xfc, 0x43, 0x77, 0xa2, 0x02, 0xe6, 0xf1, 0x15};
////register
////"15f1e500-a277-43fc-a484-dd39ef8a9100"
static UINT8 HILINK_SERVICE_REGISTER[MAX_UUID_SIZE] = {0x00, 0x91, 0x8a, 0xef, 0x39, 0xdd, 0x84, 0xa4, 0xfc, 0x43, 0x77, 0xa2, 0x00, 0xe5, 0xf1, 0x15};
////"15f1e501-a277-43fc-a484-dd39ef8a9100"
static UINT8 HILINK_CHAR_REGISTER[MAX_UUID_SIZE] = {0x00, 0x91, 0x8a, 0xef, 0x39, 0xdd, 0x84, 0xa4, 0xfc, 0x43, 0x77, 0xa2, 0x01, 0xe5, 0xf1, 0x15};
static const unsigned char HILINK_SERVICE_SUM = 2;
static unsigned short hilink_ble_uuid[] = {0xe600, 0xe500};
static UINT8 *hilink_services[] = {HILINK_SERVICE_RW, HILINK_SERVICE_REGISTER};
//
static UINT8 *hilink_characteristic_of_data[] = {HILINK_CHAR_DATA_READ, HILINK_CHAR_DATA_WRITE};
static UINT8 hilink_characteristic_property_of_data[] = {
    BSA_GATT_CHAR_PROP_BIT_NOTIFY | BSA_GATT_CHAR_PROP_BIT_INDICATE,
    /*BSA_GATT_CHAR_PROP_BIT_NOTIFY | */ BSA_GATT_CHAR_PROP_BIT_WRITE,
};
//
static UINT8 *hilink_characteristic_of_reg[] = {HILINK_CHAR_REGISTER};
static UINT8 hilink_characteristic_property_of_reg[] = {
    BSA_GATT_CHAR_PROP_BIT_NOTIFY | BSA_GATT_CHAR_PROP_BIT_WRITE | BSA_GATT_CHAR_PROP_BIT_INDICATE,
};

#define BLE_BEACON_HUAWEI_UUID 0xFDEE
#include <fcntl.h>
//0x2DDZ --> 0X32 0X44 0X44 0X5A
static UINT8 unreg_data[] = {0x01, 0x01, 0x07, 0x04, 0x00, 0x11, 0x0f, 0x12, 0x32, 0x44, 0x44, 0x5A, 0xFF, 0x00, 0x04, 0x02, 0x39, 0x39};
static UINT8 reg_data[]   = {0x01, 0x01, 0x07, 0x04, 0x00, 0x11, 0x0f, 0x12, 0x32, 0x44, 0x44, 0x5A, 0xFF, 0x00, 0x0C, 0x02, 0x39, 0x39};

typedef enum {
    DEV_UNREG = 0,
    DEV_REG,
} REG_ST;
static int hilink_state = DEV_UNREG;
void hilink_set_state(REG_ST st) {
    int fd = open("/tmp/hilink_reg_state", O_RDWR | O_CREAT);
    if (fd >= 0) {
        UINT8 tc = '0';
        if (st == DEV_REG)
            tc = '1';
        write(fd, &tc, 1);
        close(fd);
        printf("write bt_hilink, tc:%x,st:%d\n", tc, st);
    } else {
        printf("write bt_hilink state fail\n");
    }
    hilink_state = st;
}
REG_ST hilink_get_state(void) {
    REG_ST st = DEV_UNREG; //unreg default
    int fd = open("/tmp/hilink_reg_state", O_RDONLY);
    if (fd >= 0) {
        UINT8 tc = '0';
        read(fd, &tc, 1);
        close(fd);
        if (tc == '1') {
            st = DEV_REG;
        }
        printf("read bt_hilink :%d,st:%d\n", tc, st);
    } else {
        printf("read fail, will write default bt_hilink state\n");
        hilink_set_state(st = DEV_UNREG);
    }
    hilink_state = st;
    return hilink_state;
}
#define DEV_INFO_READ_MAX_LEN 256
static int _get_sys_dev_info(char *dev_name, char *data, off_t off) {
    char buf[DEV_INFO_READ_MAX_LEN];
    int fd = open(dev_name, O_RDONLY);
    int len;
    char *ptr;
    int ret = -1;
    if (fd >= 0) {
        if (off > 0)
            lseek(fd, off, SEEK_SET);
        len = read(fd, buf, DEV_INFO_READ_MAX_LEN - 1);
        if (len > 0) {
            ret = 0;
            ptr = strchr(buf, '\n');
            if (ptr) {
                *ptr = 0;
                len = ptr - buf;
            }
            buf[len] = 0;
        }
        //printf("read %s\n", buf);
        close(fd);
        if (data != NULL) {
            memcpy(data, buf, len + 1);
        }
    } else {
        printf("wrong, can\'t open file %s\n", dev_name);
    }
    return ret;
}

#define MAX_SN_LENGTH 20
static char product_sn[MAX_SN_LENGTH] = "";
static int get_product_sn(char *sn) {
    char buf[DEV_INFO_READ_MAX_LEN];
    int ret = _get_sys_dev_info((char *)"/dev/mmcblk0p4", buf, 1024);
    strncpy(sn, buf, MAX_SN_LENGTH - 1);
    sn[MAX_SN_LENGTH - 1] = 0;
    return ret;
}

void beacon_set_ble_scanrsp_data(void) {
	tBSA_DM_BLE_ADV_CONFIG ble_config_adv;
	int len;
	unsigned char mac[6]={0};
	memset(MANU_DATA + 2, 0, 6);
	if (GetDeviceMacAddress(mac) == WIFI_SUCCESS) {
		printf("===>>%s:get mac:%x,%x,%x,%x,%x,%x\n",__func__,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
		memcpy(MANU_DATA+2, mac, sizeof(mac));
	}

	sprintf(RSP_DEV_NAME, "Hi-VIATON-12DDZ%02hhX", MANU_DATA[sizeof(MANU_DATA) - 1]);
	printf("%s-RSP_DEV_NAME:%s\n", __FUNCTION__, RSP_DEV_NAME);
	printf("MANU_DATA:%02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx\n", MANU_DATA[0], MANU_DATA[1],
				 MANU_DATA[2], MANU_DATA[3], MANU_DATA[4], MANU_DATA[5], MANU_DATA[6], MANU_DATA[7]);
	SetDeviceName(RSP_DEV_NAME, strlen(RSP_DEV_NAME)); //写入指定ble名称

	memset(&ble_config_adv, 0, sizeof(tBSA_DM_BLE_ADV_CONFIG));
	ble_config_adv.is_scan_rsp = TRUE;
	ble_config_adv.adv_data_mask = BSA_DM_BLE_AD_BIT_MANU | BSA_DM_BLE_AD_BIT_DEV_NAME;
	memcpy(ble_config_adv.p_val, MANU_DATA, sizeof(MANU_DATA));
	ble_config_adv.len = sizeof(MANU_DATA);

	BleConfigAdvData data;
	data.advData = (char*)&ble_config_adv;
	data.advLength = sizeof(ble_config_adv);
	BleSetAdvData(0, &data);
}
void beacon_ble_adv_enable(REG_ST st) {
	tBSA_DM_BLE_ADV_CONFIG ble_config_adv;
	int sn_len, reginfo_len;
	if (product_sn[0] == 0) {
		get_product_sn(product_sn);
		if (product_sn[0] != 0) {
			reginfo_len = sizeof(unreg_data);
			sn_len = strlen(product_sn);
			memcpy(unreg_data + reginfo_len - 2, product_sn + sn_len - 2, 2);
			reginfo_len = sizeof(reg_data);
			memcpy(reg_data + reginfo_len - 2, product_sn + sn_len - 2, 2);
		}
	}
	memset(&ble_config_adv, 0, sizeof(tBSA_DM_BLE_ADV_CONFIG));
	ble_config_adv.flag = BTM_BLE_GEN_DISC_FLAG | BTM_BLE_BREDR_NOT_SPT; /*flag 0x6*/
	ble_config_adv.is_scan_rsp = FALSE;
	//ble_config_adv.appearance_data=0x1234;
	ble_config_adv.adv_data_mask = BSA_DM_BLE_AD_BIT_FLAGS | /*BSA_DM_BLE_AD_BIT_DEV_NAME |*/ BTM_BLE_AD_BIT_SERVICE_DATA; //| BTM_BLE_AD_BIT_APPEARANCE;
	int state;
	if (st == DEV_UNREG) {
		int len = sizeof(unreg_data);
		ble_config_adv.service_data_len = len;
		ble_config_adv.service_data_uuid.len = LEN_UUID_16;
		ble_config_adv.service_data_uuid.uu.uuid16 = BLE_BEACON_HUAWEI_UUID;
		memcpy(ble_config_adv.service_data_val, unreg_data, len);
	} else {
		int len = sizeof(reg_data);
		ble_config_adv.service_data_len = len;
		ble_config_adv.service_data_uuid.len = LEN_UUID_16;
		ble_config_adv.service_data_uuid.uu.uuid16 = BLE_BEACON_HUAWEI_UUID;
		memcpy(ble_config_adv.service_data_val, reg_data, len);
	}
	BleConfigAdvData data;
	data.advData = (char*)&ble_config_adv;
	data.advLength = sizeof(ble_config_adv);
	BleSetAdvData(0, &data);
	BleStartAdv(0, NULL);
}

/*
0:停止 hilink beacon广播
1:开启广播
*/
static int bk_ble_beacon_state=-1;
static int ble_beacon_on_off = 0;
static int last_state = -1;
void setBTBleBeaconOnOff(int state){
    last_state = ble_beacon_on_off = state;
}

int getBTBleBeaconOnOff(void){
    return ble_beacon_on_off;
}
void reset_beacon_timer(void);

int setBTBleBeacon(int state) {
    if(state == 4) {
        if( ble_beacon_on_off ) {
            reset_beacon_timer();
        }
        return 0;
    }
    if(last_state == state)
        return -1;
    last_state = state;
    switch (state) {
    case 0:
    case 1:
        if (ble_beacon_on_off != state) {
            ble_beacon_on_off = state;
            ALOGD("\n setBTBleBeacon:%s\n", (state == 0) ? "off" : "on");
        }
        break;
    case 2: //unreg
    case 3: //reg
        if (bk_ble_beacon_state != state) {
            bk_ble_beacon_state = state;
            ALOGD("\n setBTBleBeacon:%s\n", (state == 2) ? "unreg" : "reg");
            hilink_set_state((state == 2) ? 0 : 1);
        }
        break;
    }
		BleStopAdv(0);
    if (ble_beacon_on_off == 1) {
        if (state) {
            //hilink_set_state(state);
            beacon_set_ble_scanrsp_data();
            //beacon_ble_adv_enable(state);
            switch (state) {
                case 1://on
                    beacon_ble_adv_enable(hilink_get_state());
                break;
                case 2://unreg
                    //hilink_set_state(0);//unreg
                    beacon_ble_adv_enable(hilink_get_state());
                break;
                case 3:
                    //hilink_set_state(1);//reg
                    beacon_ble_adv_enable(hilink_get_state());
                break;
                default:
                break;
            }
        }
    }
    return 0;
}


void beacon_set_adv_param(UINT16 para) {
    tBSA_DM_BLE_ADV_PARAM ble_adv_para;
    if (getBTBleBeaconOnOff()) {
        memset(&ble_adv_para, 0, sizeof(tBSA_DM_BLE_ADV_PARAM));
#if 0
        ble_adv_para.adv_int_min = para;
        ble_adv_para.adv_int_max = para;
        get_bt_addr(ble_adv_para.dir_bda.bd_addr);
        app_dm_set_ble_adv_param(&ble_adv_para);
        //app_dm_get_ble_adv_param();
#endif
    }
}
unsigned int beacon_intv_settimes = 0;
//保持3分钟高频率20ms广播间隔,之后关掉
#define TIME_BEACON_KEEP_20MS_S 180
void reset_beacon_timer(void) {
    if (getBTBleBeaconOnOff()) {
        beacon_intv_settimes = 0;
				BleStartAdv(0, NULL); /*adv enable*/
    }
}
const char wifi_onoff_fname[] = "/tmp/wifi_use_state";
int get_wifi_on_off(void) {
    char buf[2];
    int len;
    int fd = open(wifi_onoff_fname, O_RDONLY);
    if (fd < 0) {
        printf("cannot read wifi onoff state");
        return -1;
    }
    len = read(fd, buf, 1);
    close(fd);
    if (len == 1) {
        if (buf[0] == '1')
            return 1;
        else
            return 0;
    }
    return -1;
}

unsigned long get_file_size(const char *filename)
{
    struct stat buf;
    if(stat(filename, &buf)<0)
    {
        return 0;
    }
    return (unsigned long)buf.st_size;
}
static int isNeedBeacon(void){
    //未注册时允许广播
    if(get_file_size("/tmp/hilink_reg_state") == 0)
        return 1;
    //已注册并且允许广播才广播
    if(get_file_size("/tmp/hilink_need_beacon_reg") > 0)
        return 1;
    return 0;
}
//static void app_avk_rc_command_thread(void);
void app_set_adv_param_thread(void) {
    int wifi_onoff_state,wifi_conn_state;
    int waittime=0,w2time=0;
    while (1) {
        sleep(1);
#if 0
        wifi_onoff_state = get_wifi_on_off();
        if (getBTBleBeaconOnOff()) {
            waittime = 0;
            if ((wifi_onoff_state == 0) || (!isNeedBeacon())) {// wifi已关闭,或者联网成功, 需要关闭蓝牙广播
                if(wifi_onoff_state == 0)
                    printf("wifi off, need stop ble beacon,on_off:%d",wifi_onoff_state);
                else
                    APP_INFO0("isNeedBeacon false, need stop ble beacon.");
                setBTBleBeaconOnOff(0);
                beacon_intv_settimes = TIME_BEACON_KEEP_20MS_S+1;
                w2time = 0;
            } else if (0 < get_file_size("/tmp/wifi_conn_state")) {
                ++w2time;
                if(++w2time > 1) {
                    printf("wifi connected, need stop ble beacon,conn_state:%d,wait:%d",get_file_size("/tmp/wifi_conn_state"), w2time);
                    w2time = 0;
                    setBTBleBeaconOnOff(0);
                    beacon_intv_settimes = TIME_BEACON_KEEP_20MS_S+1;
                }
            } else
                w2time = 0;
            if (beacon_intv_settimes <= TIME_BEACON_KEEP_20MS_S) {
                app_dm_ensure_ble_visibility();
                beacon_set_adv_param(0x20);
                ++beacon_intv_settimes;
                if ((beacon_intv_settimes - 1) % 10 == 0)
                    printf("beacon_intv_settimes:%u", beacon_intv_settimes);
            } else if (beacon_intv_settimes <= TIME_BEACON_KEEP_20MS_S + 1) {
                app_dm_set_ble_visibility(FALSE, FALSE);
                ++beacon_intv_settimes;
                printf("stop ble beacon, beacon_intv_settimes:%u", beacon_intv_settimes);
                //setBTBleBeacon(0);
            }
        }else {//在wifi打开,并没有联网成功情况下,需要打开广播
            w2time = 0;
            if ((wifi_onoff_state == 1) && isNeedBeacon()) {
                if(0 == get_file_size("/tmp/wifi_conn_state")){
                    ++waittime;
                    if (waittime > 4) { //连续4s 检测到未开广播,重开广播
                        APP_INFO0("!!!wrong!!!,no wifi , bt beacon shuld be on");
                        waittime = 0;
                        setBTBleBeaconOnOff(1);
                        beacon_set_ble_scanrsp_data();
                        beacon_ble_adv_enable(hilink_get_state());
                    }
                }else
                    waittime = 0;
            }else
                waittime = 0;
        }
#endif
    }
    printf("Exit");
    pthread_exit(NULL);
}

void bt_register_server_callback(int status, int serverId, BtUuid *appUuid)
{
	printf("===>>rzhao:%s\n",__func__);
}

void bt_connect_server_callback(int connId, int serverId, const BdAddr *bdAddr)
{
	printf("===>>rzhao:%s\n",__func__);
}

void bt_disconnect_server_callback(int connId, int serverId, const BdAddr *bdAddr)
{
	printf("===>>rzhao:%s\n",__func__);
	BleStopAdv(0);
	beacon_set_ble_scanrsp_data();
	beacon_ble_adv_enable(hilink_get_state());
}

void bt_service_add_callback(int status, int serverId, BtUuid *uuid, int srvcHandle)
{
	int i, char_sum;
	int server_num = srvcHandle;
	if (server_num == 0)
		char_sum = sizeof(hilink_characteristic_of_data) / sizeof(UINT8 *);
	else
		char_sum = sizeof(hilink_characteristic_of_reg) / sizeof(UINT8 *);
	ALOGD("app_ble_server_add_char server_num:%d,sum:%d", server_num, char_sum);
	for (i = 0; i < char_sum; i++) {
		if ((server_num >= 0) && (server_num < HILINK_SERVICE_SUM)) {
			UINT8 *char_name;
			int characteristic_property = 0;
			if (server_num == 0) {
				char_name = hilink_characteristic_of_data[i];
				characteristic_property = hilink_characteristic_property_of_data[i];
			} else {
				char_name = hilink_characteristic_of_reg[i];
				characteristic_property = hilink_characteristic_property_of_reg[i];
			}
			BtUuid charac_uuid;
			charac_uuid.uuid = (char*)char_name;
			charac_uuid.uuidLen = strlen(charac_uuid.uuid);
			BleGattsAddCharacteristic(serverId, server_num, charac_uuid, characteristic_property, (1<<0 | 1<<4));
		}
	}
		printf("===>>rzhao:%s:serverId=%d\n",__func__,serverId);
}

void bt_include_service_add_callback(int status, int serverId, int srvcHandle, int includeSrvcHandle)
{
	printf("===>>rzhao:%s\n",__func__);
}

void bt_characteristic_add_callback(int status, int serverId, BtUuid *uuid,
                                          int srvcHandle, int characteristicHandle)
{
	printf("===>>rzhao:%s\n",__func__);
	BleGattsStartService(serverId, srvcHandle);
}

void bt_descriptor_add_callback(int status, int serverId, BtUuid *uuid,
                                      int srvcHandle, int descriptorHandle)
{
	printf("===>>rzhao:%s\n",__func__);
}

void bt_service_start_callback(int status, int serverId, int srvcHandle)
{
	printf("===>>rzhao:%s\n",__func__);
	BleStopAdv(0);
	beacon_set_ble_scanrsp_data();
	beacon_ble_adv_enable(hilink_get_state());
}

void bt_service_stop_callback(int status, int serverId, int srvcHandle)
{
	printf("===>>rzhao:%s\n",__func__);
}

void bt_service_delete_callback(int status, int serverId, int srvcHandle)
{
	printf("===>>rzhao:%s\n",__func__);
}

void bt_request_read_callback(BtReqReadCbPara readCbPara)
{
	printf("===>>rzhao:%s\n",__func__);
}

void bt_request_write_callback(BtReqWriteCbPara writeCbPara)
{
	printf("===>>rzhao:%s\n",__func__);
}

void bt_response_confirmation_callback(int status, int handle)
{
	printf("===>>rzhao:%s\n",__func__);
}

void bt_indication_sent_callback(int connId, int status)
{
	printf("===>>rzhao:%s\n",__func__);
}

void bt_mtu_change_callback(int connId, int mtu)
{
	printf("===>>rzhao:%s\n",__func__);
}

BtGattServerCallbacks ble_cback = {
	.registerServerCb = bt_register_server_callback,
	.connectServerCb = bt_connect_server_callback,
	.disconnectServerCb = bt_disconnect_server_callback,
	.serviceAddCb = bt_service_add_callback,
	.includeServiceAddCb = bt_include_service_add_callback,
	.characteristicAddCb = bt_characteristic_add_callback,
	.descriptorAddCb = bt_descriptor_add_callback,
	.serviceStartCb = bt_service_start_callback,
	.serviceStopCb = bt_service_stop_callback,
	.serviceDeleteCb = bt_service_delete_callback,
	.requestReadCb = bt_request_read_callback,
	.requestWriteCb = bt_request_write_callback,
	.responseConfirmationCb = bt_response_confirmation_callback,
	.indicationSentCb = bt_indication_sent_callback,
	.mtuChangeCb = bt_mtu_change_callback,
};

int creatBSAPath(){
    DIR * dirp = opendir("/usr/data/bsa");
    if (NULL == dirp) {
	int ret = mkdir("/usr/data/bsa", 0777);
	if (ret != 0) {
	    printf("Error:can't create directory: %s\n", "/usr/data/bsa");
	    return -1;
	}
    } else {
	closedir(dirp);
    }
    return 0;
}
/*******************************************************************************
 **
 ** Function         main
 **
 *******************************************************************************/
static string bt_name = "";

int BtMain()
{
	ALOGV("--BtMain in\n");
	/* create /data/bsa/ */
	if(creatBSAPath() < 0) {
		ALOGE("\n\nwrong creatBSAPath wrong\n");
		return -1;
	}

	if(InitBtStack() != 0) {
		ALOGE("\n\nwrong managerInit wrong\n");
		return -1;
	}

	string enable = "";
	android::SettingsDB::getString(DB_KEY_BLUETOOTH_ENABLE, enable);
	if (enable.empty()) {
		android::SettingsDB::getString(DB_KEY_BLUETOOTH_OPEN_ENABLE, enable);
	}
	android::SettingsDB::putString(DB_KEY_BLUETOOTH_ENABLE, enable);

	ALOGV("enable:%s\n",enable.c_str());
    if (1||(enable == "true")) {
		EnableBtStack();
		BleGattsRegisterCallbacks(&ble_cback);

		BtUuid appUuid;
		for (int idx = 0; idx < HILINK_SERVICE_SUM; idx++) {
			unsigned short uuid = hilink_ble_uuid[idx];
			char uuidChar[10];
			sprintf(uuidChar,"%u",uuid);
			appUuid.uuid = strdup(uuidChar);
			appUuid.uuidLen=strlen(appUuid.uuid);
			printf("===>>%s:%d,appUuid.uuid is %s\n",__func__,__LINE__,appUuid.uuid);
			BleGattsRegister(appUuid);
			free(appUuid.uuid);
		}
		for (int idx = 0; idx < HILINK_SERVICE_SUM; idx++) {
			appUuid.uuid = (char *)hilink_services[idx];
			appUuid.uuidLen=strlen(appUuid.uuid);
#if APP_BLE_SERVICE_IS_PRIMARY   //is primary
			BleGattsAddService(idx, appUuid, true, APP_BLE_SERVICE_NUM_HANDLE);
#else
			BleGattsAddService(idx, appUuid, false, APP_BLE_SERVICE_NUM_HANDLE);
#endif
		}
		BleStartAdv(1, NULL);
		}

    ALOGD("BTMain start here");
    return 0;
}
