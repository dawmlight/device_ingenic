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
#include <strings.h>

#include "gki.h"
#include "uipc.h"

#include "bsa_dm_api.h"
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

#include <stdbool.h>
#include "ohos_bt_def.h"
#include "ohos_bt_gatt_server.h"
#include "ohos_bt_gatt.h"

#define ALOGE printf
#define BT_POWER_STATE "/sys/class/rfkill/rfkill0/state"
#ifdef BLUETOOTH_HILINK_SUPPORT
extern tAPP_MGR_CB app_mgr_cb;
#endif
extern void BluetoothServer_instance();

int managerInit(const char* name);
static BtGattServerCallbacks *bt_callback = NULL;
static char bt_name[128] = "ohos-bt";
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

int startProfiles(){
	int ret = 0;
	int status = app_ble_init();
	if (status < 0){
		APP_ERROR0("Couldn't Initialize BLE !!");
		ret = -1;
	}
	/* Start BLE application */
	status = app_ble_start();
	if (status < 0){
		APP_ERROR0("Couldn't Start BLE !!");
		ret = -1;
	}
	return ret;
}

void stopProfiles(){
    app_ble_exit();
    app_mgt_close();
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
    if (app_mgr_config(name)){
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

/*******************************************************************************
**
** Function         app_ble_server_profile_cback
**
** Description      BLE Server Profile callback.
**
** Returns          void
**
*******************************************************************************/
void event_cback(tBSA_BLE_EVT event, tBSA_BLE_MSG *p_data) {
	int num;
	BtReqReadCbPara readCbPara;
	BtReqWriteCbPara writeCbPara;

	if(bt_callback == NULL){
		printf("%s: bt_callback is null, nothing todo!",__func__);
		return;
	}

	printf("event_cback event = %d ", event);
	switch (event) {
	case BSA_BLE_SE_CREATE_EVT:
		if (p_data->ser_create.status == BSA_SUCCESS) {
			num = app_ble_server_find_index_by_interface(p_data->ser_create.server_if);

			/* search interface number */
			if (num < 0) {
				APP_ERROR0("no interface!!");
				break;
			}

			if(bt_callback->serviceAddCb != NULL)
				bt_callback->serviceAddCb(p_data->ser_create.status, p_data->ser_create.service_id, NULL, num);
		}
		break;

	case BSA_BLE_SE_ADDCHAR_EVT:
		num = app_ble_server_find_index_by_interface(p_data->ser_addchar.server_if);

		/* search interface number */
		if (num < 0) {
			APP_ERROR0("no interface!!");
			break;
		}

		if(bt_callback->characteristicAddCb != NULL)
			bt_callback->characteristicAddCb(p_data->ser_addchar.status, p_data->ser_addchar.service_id, NULL, num, 0);
		break;

	case BSA_BLE_SE_START_EVT:
		if (p_data->ser_start.status == BSA_SUCCESS) {
			num = app_ble_server_find_index_by_interface(p_data->ser_start.server_if);

			/* search interface number */
			if (num < 0) {
				APP_ERROR0("no interface!!");
				break;
			}
			if(bt_callback->serviceStartCb != NULL){
				bt_callback->serviceStartCb(p_data->ser_start.status, p_data->ser_start.service_id, num);
			}
		}
		break;

	case BSA_BLE_SE_READ_EVT:
		memset(&readCbPara, 0, sizeof(readCbPara));
		readCbPara.connId = p_data->ser_read.conn_id;
		readCbPara.transId = p_data->ser_read.trans_id;
		readCbPara.bdAddr = p_data->ser_read.remote_bda;
		readCbPara.attrHandle = p_data->ser_read.handle;
		readCbPara.offset = p_data->ser_read.offset;
		readCbPara.isLong = false;
		if(bt_callback->requestReadCb != NULL)
			bt_callback->requestReadCb(readCbPara);
		break;

	case BSA_BLE_SE_WRITE_EVT:
		memset(&writeCbPara, 0, sizeof(writeCbPara));
		writeCbPara.connId = p_data->ser_write.conn_id;
		writeCbPara.transId = p_data->ser_write.trans_id;
		writeCbPara.bdAddr = p_data->ser_write.remote_bda;
		writeCbPara.attrHandle = p_data->ser_write.handle;
		writeCbPara.offset = p_data->ser_write.offset;
		writeCbPara.needRsp = false;
		writeCbPara.isPrep = false;
		int length = (int)(p_data->ser_write.len);
		length = length > BLE_DATA_MAX_SIZE * 2 ? BLE_DATA_MAX_SIZE * 2 : length;
		char request_value[BLE_DATA_MAX_SIZE * 2];
		memset(request_value, 0, BLE_DATA_MAX_SIZE * 2);
		memcpy(request_value, p_data->ser_write.value, length);
		writeCbPara.length = length;
		writeCbPara.value = request_value;
		if(bt_callback->requestWriteCb != NULL)
			bt_callback->requestWriteCb(writeCbPara);
		break;

	case BSA_BLE_SE_OPEN_EVT:
		num = app_ble_server_find_index_by_interface(p_data->ser_open.server_if);
		/* search interface number */
		if (num < 0) {
			APP_ERROR0("no interface!!");
			break;
		}
		if(bt_callback->connectServerCb != NULL){
			BdAddr bdAddr;
			memset(bdAddr.addr, 0, sizeof(bdAddr.addr));
			memcpy(bdAddr.addr, p_data->ser_open.remote_bda, BD_ADDR_LEN);
			bt_callback->connectServerCb(p_data->ser_open.conn_id, num, &bdAddr);
		}
		break;

	case BSA_BLE_SE_CLOSE_EVT:
		num = app_ble_server_find_index_by_interface(p_data->ser_close.server_if);
		/* search interface number */
		if (num < 0) {
			APP_ERROR0("no interface!!");
			break;
		}
		if(bt_callback->disconnectServerCb != NULL)
			bt_callback->disconnectServerCb(p_data->ser_close.conn_id, num, p_data->ser_close.remote_bda);
		break;

	case BSA_BLE_SE_CONF_EVT:
		APP_INFO1("BSA_BLE_SE_CONF_EVT status:%d,conn_id:0x%x", p_data->ser_conf.status, p_data->ser_conf.conn_id);
		break;

	default:
		break;
	}
}

/**
 * @brief Initializes the Bluetooth protocol stack.
 *
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the Bluetooth protocol stack is initialized;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int InitBtStack(void)
{
	if(managerInit(bt_name) < 0) {
		ALOGE("\n\nwrong managerInit wrong\n");
		return OHOS_BT_STATUS_FAIL;
	}
	app_mgr_set_bt_config(true);
	setBtPowerEnable(true);
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Enables the Bluetooth protocol stack.
 *
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the Bluetooth protocol stack is enabled;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int EnableBtStack(void)
{
	startProfiles();
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Disables the Bluetooth protocol stack.
 *
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the Bluetooth protocol stack is disabled;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int DisableBtStack(void)
{
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Sets the Bluetooth device name.
 *
 * @param name Indicates the pointer to the name to set.
 * @param len Indicates the length of the name to set.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the Bluetooth device name is set;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int SetDeviceName(const char *name, unsigned int len)
{
	int ret = OHOS_BT_STATUS_SUCCESS;
	if(app_mgr_set_ble_name(name, len)) {
		ret = OHOS_BT_STATUS_FAIL;
	}
	return ret;
}

/**
 * @brief Sets advertising data.
 *
 * @param advId Indicates the advertisement ID, which is allocated by the upper layer of the advertiser.
 * @param data Indicates the pointer to the advertising data. For details, see {@link BleConfigAdvData}.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if advertising data is set;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleSetAdvData(int advId, const BleConfigAdvData *data)
{
	/*need modify later*/
	tBSA_DM_BLE_ADV_CONFIG *p_data = (tBSA_DM_BLE_ADV_CONFIG*)data->advData;
	app_dm_set_ble_adv_data(p_data);
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Starts advertising.
 *
 * @param advId Indicates the advertisement ID, which is allocated by the upper layer of the advertiser.
 * @param param Indicates the pointer to the advertising parameters. For details, see {@link BleAdvParams}.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if advertising is started;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleStartAdv(int advId, const BleAdvParams *param)
{
	/*need modify later,advId currently use for start server*/
	app_dm_set_ble_visibility(true, true);
	if(advId){
		SppProtocol_instance();
		BleProtocol_instance();
		BluetoothServer_instance();
	}

	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Stops advertising.
 *
 * @param advId Indicates the advertisement ID, which is allocated by the upper layer of the advertiser.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if advertising is stopped;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleStopAdv(int advId)
{
	app_dm_set_ble_visibility(false, false);
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Updates advertising parameters.
 *
 * @param advId Indicates the advertisement ID, which is allocated by the upper layer of the advertiser.
 * @param param Indicates the pointer to the advertising parameters. For details, see {@link BleAdvParams}.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if advertising parameters are updated;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleUpdateAdv(int advId, const BleAdvParams *param)
{
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Sets the secure I/O capability mode.
 *
 * @param mode Indicates the capability mode to set. For details, see {@link BleIoCapMode}.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the capability mode is set;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleSetSecurityIoCap(BleIoCapMode mode)
{
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Sets the authentication mode for secure connection requests.
 *
 * @param mode Indicates the authentication mode to set. For details, see {@link BleAuthReqMode}.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the authentication mode is set;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleSetSecurityAuthReq(BleAuthReqMode mode)
{
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Responds to a secure connection request.
 *
 * @param bdAddr Indicates the address of the device that sends the request.
 * @param accept Specifies whether to accept the request. The value <b>true</b> means to accept the request,
 * and <b>false</b> means to reject the request.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the request is responded to;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleGattSecurityRsp(BdAddr bdAddr, bool accept)
{
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Obtains the device MAC address.
 *
 * @param mac Indicates the pointer to the device MAC address.
 * @param len Indicates the length of the device MAC address.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the device MAC address is obtained;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int ReadBtMacAddr(unsigned char *mac, unsigned int len)
{
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Sets scan parameters.
 *
 * @param clientId Indicates the client ID, which is obtained during client registration.
 * @param param Indicates the pointer to the scan parameters. For details, see {@link BleScanParams}.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the scan parameters are set;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleSetScanParameters(int clientId, BleScanParams *param)
{
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Starts a scan.
 *
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the scan is started;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleStartScan(void)
{
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Stops a scan.
 *
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the scan is stopped;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleStopScan(void)
{
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Registers GATT callbacks.
 *
 * @param func Indicates the pointer to the callbacks to register. For details, see {@link BtGattCallbacks}.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the GATT callbacks are registered;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleGattRegisterCallbacks(BtGattCallbacks *func)
{
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Sets advertising data and parameters and starts advertising.
 *
 * This function is available for system applications only. \n
 *
 * @param advId Indicates the pointer to the advertisement ID.
 * @param rawData Indicates the advertising data. For details, see {@link StartAdvRawData}.
 * @param advParam Indicates the advertising parameters. For details, see {@link BleAdvParams}.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the operation is successful;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleStartAdvEx(int *advId, const StartAdvRawData rawData, BleAdvParams advParam)
{
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Registers a GATT server with a specified application UUID.
 *
 * The <b>RegisterServerCallback</b> is invoked to return the GATT server ID.
 *
 * @param appUuid Indicates the UUID of the application for which the GATT server is to be registered.
 * The UUID is defined by the application.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the GATT server is registered;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleGattsRegister(BtUuid appUuid)
{
	unsigned short uuid = atoi(appUuid.uuid);
	printf("===>>%s:uuid=%u,%x\n",__func__,uuid,uuid);
	app_ble_server_register(uuid, event_cback);
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Unregisters a GATT server with a specified ID.
 *
 * @param serverId Indicates the ID of the GATT server.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the GATT server is unregistered;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleGattsUnRegister(int serverId)
{
	bt_callback = NULL;
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Disconnects the GATT server from the client.
 *
 * @param serverId Indicates the ID of the GATT server.
 * @param bdAddr Indicates the address of the client.
 * @param connId Indicates the connection ID, which is returned during the server registration.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the GATT server is disconnected from the client;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleGattsDisconnect(int serverId, BdAddr bdAddr, int connId)
{
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Adds a service.
 *
 * This function adds the service, its characteristics, and descriptors separately in sequence.\n
 * A service is a collection of data and related behavior that enable a specific capability or feature.\n
 * It consists of a service declaration and one or more included services and characteristics.
 *
 * @param serverId Indicates the ID of the GATT server.
 * @param srvcUuid Indicates the UUID of the service.
 * @param isPrimary Specifies whether the service is primary or secondary.
 * Value <b>true</b> indicates that the service is primary, and <b>false</b> indicates that the service is secondary.
 * @param number Indicates the number of attribute handles.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the service is added;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleGattsAddService(int serverId, BtUuid srvcUuid, bool isPrimary, int number)
{
	app_ble_server_create_service(serverId, srvcUuid.uuid, isPrimary, number);
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Adds an included service to a specified service.
 *
 * An included service is referenced to define another service on the GATT server.
 *
 * @param serverId Indicates the ID of the GATT server.
 * @param srvcHandle Indicates the handle ID of the service.
 * @param includedHandle Indicates the attribute handle ID of the included service.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the included service is added to the service;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleGattsAddIncludedService(int serverId, int srvcHandle, int includedHandle)
{
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Adds a characteristic to a specified service.
 *
 * A characteristic consists of data, the data access method, data format, and how the data manifests itself.
 *
 * @param serverId Indicates the ID of the GATT server.
 * @param srvcHandle Indicates the handle ID of the service.
 * @param characUuid Indicates the UUID of the characteristic to add.
 * @param properties Indicates the access methods supported by the characteristic,
 * as enumerated in {@link GattCharacteristicProperty}.
 * @param permissions Indicates the access permissions supported by the characteristic,
 * as enumerated in {@link GattAttributePermission}.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the characteristic is added to the service;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleGattsAddCharacteristic(int serverId, int srvcHandle, BtUuid characUuid,
                              int properties, int permissions)
{
	app_ble_server_add_char(serverId, srvcHandle, characUuid.uuid, properties, permissions);
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Adds a descriptor to a specified characteristic.
 *
 * A descriptor contains the description, configuration, and format of a characteristic.
 *
 * @param serverId Indicates the ID of the GATT server.
 * @param srvcHandle Indicates the handle ID of the service to which the characteristic belongs.
 * @param descUuid Indicates the UUID of the descriptor to add.
 * @param permissions Indicates the access permissions supported by the descriptor,
 * as enumerated in {@link GattAttributePermission}.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the descriptor is added to the characteristic;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleGattsAddDescriptor(int serverId, int srvcHandle, BtUuid descUuid, int permissions)
{
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Starts a service.
 *
 * @param serverId Indicates the ID of the GATT server.
 * @param srvcHandle Indicates the handle ID of the service.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the service is started;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleGattsStartService(int serverId, int srvcHandle)
{
	app_ble_server_start_service(serverId, srvcHandle);
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Stops a service.
 *
 * @param serverId Indicates the ID of the GATT server.
 * @param srvcHandle Indicates the handle ID of the service.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the service is stopped;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleGattsStopService(int serverId, int srvcHandle)
{
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Deletes a service.
 *
 * @param serverId Indicates the ID of the GATT server.
 * @param srvcHandle Indicates the handle ID of the service.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the service is deleted;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleGattsDeleteService(int serverId, int srvcHandle)
{
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Clears all services.
 *
 * @param serverId Indicates the ID of the GATT server.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the services are cleared;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleGattsClearServices(int serverId)
{
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Sends a response to the client from which a read or write request has been received.
 *
 * @param serverId Indicates the ID of the GATT server.
 * @param param Indicates the pointer to the response parameters. For details, see {@link GattsSendRspParam}.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the response is sent;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleGattsSendResponse(int serverId, GattsSendRspParam *param)
{
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Sends an indication or notification to the client.
 *
 * The <b>confirm</b> field in <b>param</b> determines whether to send an indication or a notification.
 *
 * @param serverId Indicates the ID of the GATT server.
 * @param param Indicates the pointer to the sending parameters. For details, see {@link GattsSendIndParam}.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the indication or notification is sent;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleGattsSendIndication(int serverId, GattsSendIndParam *param)
{
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Sets the encryption type for the GATT connection.
 *
 * @param bdAddr Indicates the address of the client.
 * @param secAct Indicates the encryption type, as enumerated in {@link BleSecAct}.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the encryption type is set;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleGattsSetEncryption(BdAddr bdAddr, BleSecAct secAct)
{
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Registers GATT server callbacks.
 *
 * @param func Indicates the pointer to the callbacks to register, as enumerated in {@link BtGattServerCallbacks}.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the callbacks are registered;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleGattsRegisterCallbacks(BtGattServerCallbacks *func)
{
	bt_callback = func;
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Adds a service, its characteristics, and descriptors and starts the service.
 *
 * This function is available for system applications only.
 *
 * @param srvcHandle Indicates the pointer to the handle ID of the service,
 * which is returned by whoever implements this function.
 * @param srvcInfo Indicates the pointer to the service information.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the operation is successful;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleGattsStartServiceEx(int *srvcHandle, BleGattService *srvcInfo)
{
	return OHOS_BT_STATUS_SUCCESS;
}

/**
 * @brief Stops a service.
 *
 * This function is available for system applications only.
 *
 * @param srvcHandle Indicates the handle ID of the service.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the operation is successful;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int BleGattsStopServiceEx(int srvcHandle)
{
	return OHOS_BT_STATUS_SUCCESS;
}
