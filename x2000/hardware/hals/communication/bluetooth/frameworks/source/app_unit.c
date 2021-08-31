/*****************************************************************************
 **
 **  Name:           Main.cpp
 **
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <errno.h>

#include "gki.h"
#include "uipc.h"

#include "bsa_api.h"

#include "app_xml_utils.h"

#include "app_disc.h"
#include "app_utils.h"
#include "app_dm.h"
#include "app_ble.h"
#include "app_dg.h"
#include "app_ble_server.h"
#include "app_utils.h"

#include "app_services.h"
#include "app_mgt.h"
#include "app_thread.h"
#include "app_manager.h"

#define ALOGE printf
//#include <hardware/hardware.h>
//#include <hardware/audio.h>

#define APP_DG_TEST_FILE "./server/build/test_files/dg/tx_test_file.txt"
#define APP_BLE_MAIN_INVALID_UUID    0
#define BT_POWER_STATE "/sys/class/rfkill/rfkill0/state"
/*
 * Extern variables
 */
extern BD_ADDR                 app_sec_db_addr;    /* BdAddr of peer device requesting SP */
extern tAPP_MGR_CB app_mgr_cb;
extern tAPP_XML_CONFIG         app_xml_config;
char* bt_name = NULL;

int managerInit(const char* name);
int startProfiles();

/*******************************************************************************
 **
 ** Function         app_mgr_mgt_callback
 **
 *******************************************************************************/
static BOOLEAN app_mgr_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
{
    switch(event)
	{
	case BSA_MGT_STATUS_EVT:
	    APP_DEBUG0("BSA_MGT_STATUS_EVT");
	    if (p_data->status.enable)
		{
		    APP_DEBUG0("Bluetooth restarted => re-initialize the application");
		    app_mgr_config(bt_name);
		}
	    break;

	case BSA_MGT_DISCONNECT_EVT:
	    APP_DEBUG1("---BSA_MGT_DISCONNECT_EVT reason:%d", p_data->disconnect.reason);
            system("killall -2 bsa_server_mips");

	    pid_t pid;
	    pid = fork();
	    if (pid == -1){
	    	    APP_ERROR0("fork failed!");
	    	    return 0;
	    }
	    if (pid == 0) {
	    	    APP_DEBUG0("start bsa_server_mips..");
	    	    execlp("/usr/bin/bsa_server_mips", "bsa_server_mips","-lpm","-all=0","-r","14","-d","/dev/ttyS0","-u","/var/run/","-p","/firmware/BCM43455_003.001.025.0160.0303.hcd", NULL);
	    }else{
	    	    APP_DEBUG0("signal..");
	    	    signal(SIGCHLD,SIG_IGN);
	    }
            APP_DEBUG1("bt_name:%s", bt_name);
            if(bt_name != NULL) {
                    if(managerInit(bt_name) < 0) {
                            APP_ERROR0("managerInit failed!");
                            return 0;
                    }
            }
            if(startProfiles() < 0) {
                    APP_ERROR0("startProfiles failed!");
                    return 0;
            }
	    break;

	default:
	    break;
	}
    return TRUE;
}

int managerInit(const char* name){
    int mode;
    app_mgt_init();
    if (app_mgt_open("/var/run/", app_mgr_mgt_callback) < 0){
	APP_ERROR0("Unable to connect to server");
	return -1;
    }

      /* Init XML state machine */
    app_xml_init();
    bt_name = (char*)name;
    if (app_mgr_config((char*)name)){
	APP_ERROR0("Couldn't configure successfully, exiting");
	return -1;
    }

      /* Display FW versions */
    app_mgr_read_version();

      /* Get the current Stack mode */
    mode = app_dm_get_dual_stack_mode();
    if (mode < 0){
	APP_ERROR0("app_dm_get_dual_stack_mode failed");
	return -1;
    }else{
	  /* Save the current DualStack mode */
	app_mgr_cb.dual_stack_mode = mode;
	APP_INFO1("Current DualStack mode:%s", app_mgr_get_dual_stack_mode_desc());
    }

#ifdef USE_INGENIC_AUDIO_OSS_IN
/**************************load audio oss***************************/
    int ret = load_ingenic_audio_oss();
    APP_INFO1("Load ingenic audio dev:%d\n",ret);
#endif
    return 0;
}

#ifdef BLUETOOTH_HILINK_SUPPORT
tAPP_THREAD t_app_set_adv_param_thread;
extern void app_set_adv_param_thread(void);
extern void beacon_ble_adv_disable();
extern void beacon_set_ble_scanrsp_data(void);
#endif

int startProfiles(){
/**************************start DG*******************************/
    if (app_dg_init() < 0){
	APP_ERROR0("Unable to init DG");
	return -1;
    }

    if (app_dg_start() < 0){
	APP_ERROR0("Unable to start DG");
	return -1;
    }
/**************************start BLE*******************************/
    int status = app_ble_init();
    if (status < 0){
	APP_ERROR0("Couldn't Initialize BLE !!");
    }
      /* Start BLE application */
    status = app_ble_start();
    if (status < 0){
	APP_ERROR0("Couldn't Start BLE !!");
    }else {
	app_ble_server_register(APP_BLE_MAIN_INVALID_UUID, NULL);
	app_ble_server_create_service();
    }

/**************************start HeadSet***************************/
#ifdef BLUETOOTH_HS_SUPPORT
    /* Init Headset Application */
    app_hs_init();
      /* Start Headset service*/
    app_hs_start(NULL);
#endif
    app_dg_listen();
/**************************start avk***************************/
#ifdef BLUETOOTH_AVK_SUPPORT
    app_avk_init(NULL);
    app_avk_register();
#endif
#ifdef BLUETOOTH_AV_SUPPORT
    app_av_init(TRUE);
    int choice;
    for (choice = 0; choice < BTA_AV_NUM_STRS; choice++){
    	app_av_register();
    }
    BD_ADDR bd_addr;
    if(app_mgr_get_dev_av_bt_addr(bd_addr) == 0)
        app_av_open(bd_addr);

#endif
/**************************start ag***************************/
#ifdef BLUETOOTH_AG_SUPPORT
	app_ag_init();
	app_ag_start(BSA_SCO_ROUTE_PCM);

#endif
#ifdef BLUETOOTH_HILINK_SUPPORT
    system("wpa_cli scan &");
    app_create_thread(app_set_adv_param_thread, 0, 0, &t_app_set_adv_param_thread);
#endif
    return 0;
}
static void setBtPowerEnable(bool enable){
    char buffer = '0';
    int fd = open(BT_POWER_STATE, O_WRONLY);
    if (fd < 0){
        ALOGE("set_bluetooth_power : open for write failed: %s (%d)",
	      strerror(errno), errno);
        return;
    }

    if(enable){
	    buffer = '1';
    }
    int sz = write(fd, &buffer, 1);
    if (sz < 0) {
	ALOGE("set_bluetooth_power : write failed: %s (%d)",
	      strerror(errno), errno);
    }
    close(fd);
}
static void stopProfiles(){
      /* Disable DG service */
    app_dg_stop();

      /* Exit BLE mode */
    app_ble_exit();

#ifdef BLUETOOTH_HS_SUPPORT
      /* Stop Headset service*/
    app_hs_stop();
#endif
      /* stop avk profile */
#ifdef BLUETOOTH_AVK_SUPPORT
    app_avk_end();
    app_avk_deregister();
#endif

#ifdef BLUETOOTH_AV_SUPPORT
    app_av_end();
#endif
	/*stop ag*/
#ifdef BLUETOOTH_AG_SUPPORT
	app_ag_end();
#endif
      /**/
    app_mgt_close();
}
#ifdef BLUETOOTH_HILINK_SUPPORT
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
    beacon_ble_adv_disable();
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
#endif
#ifdef BLUETOOTH_HILINK_SUPPORT
int setBleVisibility(int discoverable, int connectable) {
    app_dm_set_ble_visibility(discoverable, connectable);
}
#endif
int setBTVisibility(bool discoverable, bool connectable) {
#ifdef BLUETOOTH_HILINK_SUPPORT
    app_dm_set_visibility(false, false);
    return 0;
#endif
    app_dm_set_visibility(discoverable, connectable);
    app_dm_set_ble_visibility(discoverable, connectable);
}

int setBTEnable(bool enable)
{
    app_mgr_set_bt_config(enable);
    setBtPowerEnable(enable);
    if(enable)
	startProfiles();
    else
	stopProfiles();
}

