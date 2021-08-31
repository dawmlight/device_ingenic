/*****************************************************************************
**
**  Name:           app_ble_server.c
**
**  Description:    Bluetooth BLE Server general application
**
**  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include "btm_api.h"
#include "app_ble.h"
#include "app_disc.h"
#include "app_dm.h"
#include "app_mgt.h"
#include "app_utils.h"
#include "app_xml_utils.h"

#include "app_ble_server.h"

#define APP_BLE_SERVICE_NUM 1

#ifndef BLUETOOTH_HILINK_SUPPORT
#define APP_BLE_ADV_VALUE_LEN 6
#define APP_BLE_ADV_CONF_UUID_VAL 0x1800 //eg:0x1800
#define APP_BLE_ADV_CONF_IS_SCAN_RSP 0   //0:scan don't response
#endif

#define APP_BLE_SERVICE_IS_PRIMARY 1   //is primary
#define APP_BLE_SENDIND_NEED_CONFIRM 1 //0: don't need confirm

#define APP_BLE_UNBOND_END "unbondEnd" //phone unbond end

#ifdef BLUETOOTH_HILINK_SUPPORT
static const UINT8 HILINK_SERVICE_SUM = 2;
static int attr_id_num[] = {0, 0, 0};
#else
static int attr_id_num[] = {0};
#define APP_BLE_SERVICE_NUM_HANDLE 11

//GLASS_BLE service uuid: "CE9ECF03-D64B-4586-A405-50D0F4FABC86";
static UINT8 SERVICE_UUID128[MAX_UUID_SIZE] = {0x86, 0xBC, 0xFA, 0xF4, 0xD0, 0x50, 0x05, 0xA4, 0x86, 0x45, 0x4B, 0xD6, 0x03, 0xCF, 0x9E, 0xCE};
//GLASS_BLE characteristic uuid: "1FABD91D-5521-4B6D-B0C8-7B3C0B18577D";
static UINT8 CHAR_UUID128[MAX_UUID_SIZE] = {0x7D, 0x57, 0x18, 0x0B, 0x3C, 0x7B, 0xC8, 0xB0, 0x6D, 0x4B, 0x21, 0x55, 0x1D, 0xD9, 0xAB, 0x1F};
#endif
#define APP_BLE_REGISTER_UUID 0xabcd
/*
 * Global Variables
 */
static void (*mWriteRequestCback)(char *addr, int conn_id, char *data, int size) = NULL;
static void (*mConnectionStateChange)(char *add, int state) = NULL;
#ifdef BLUETOOTH_HILINK_SUPPORT
static void (*mHilinkConnectionStateChange)(char *addr, int link_state, int con_id, int server_num) = NULL;
static void (*mHilinkBLEWriteRequestCback)(int server_no, char *addr, int conn_id, char *data, int size) = NULL;
#endif
tBSA_BLE_CBACK *ohos_event_cback = NULL;
/*
 * Local functions
 */
extern int app_mgr_get_bleName(char *name);
int setBTVisibility(_Bool discoverable, _Bool connectable) {
    app_dm_set_ble_visibility(discoverable, connectable);
}
/*
 * BLE common functions
 */

/*******************************************************************************
 **
 ** Function         app_ble_server_find_free_server
 **
 ** Description      find free server for BLE server application
 **
 ** Parameters
 **
 ** Returns          positive number(include 0) if successful, error code otherwise
 **
 *******************************************************************************/
int app_ble_server_find_free_server(void) {
    int index;

    for (index = 0; index < BSA_BLE_SERVER_MAX; index++) {
        if (!app_ble_cb.ble_server[index].enabled) {
            return index;
        }
    }
    return -1;
}

/*******************************************************************************
 **
 ** Function         app_ble_server_find_free_attr
 **
 ** Description      find free attr for BLE server application
 **
 ** Parameters
 **
 ** Returns          positive number(include 0) if successful, error code otherwise
 **
 *******************************************************************************/
int app_ble_server_find_free_attr(UINT16 server) {
    int index;

    for (index = 0; index < BSA_BLE_ATTRIBUTE_MAX; index++) {
        if (!app_ble_cb.ble_server[server].attr[index].attr_UUID.uu.uuid16) {
            return index;
        }
    }
    return -1;
}

/*******************************************************************************
 **
 ** Function         app_ble_server_display
 **
 ** Description      display BLE server
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ble_server_display(void) {
    int index, attr_num;

    APP_INFO0("*************** BLE SERVER LIST *****************");
    for (index = 0; index < BSA_BLE_SERVER_MAX; index++) {
        if (app_ble_cb.ble_server[index].enabled) {
            APP_INFO1("%d:BLE Server server_if:%d", index,
                      app_ble_cb.ble_server[index].server_if);
            for (attr_num = 0; attr_num < BSA_BLE_ATTRIBUTE_MAX; attr_num++) {
                if (app_ble_cb.ble_server[index].attr[attr_num].attr_UUID.uu.uuid16) {
                    if ((app_ble_cb.ble_server[index].attr[attr_num].attr_type == BSA_GATTC_ATTR_TYPE_SRVC) ||
                        (app_ble_cb.ble_server[index].attr[attr_num].attr_type == BSA_GATTC_ATTR_TYPE_INCL_SRVC)) {
                        APP_INFO1("\t attr_num:%d:uuid:0x%04x, is_pri:%d, service_id:%d attr_id:%d",
                                  attr_num,
                                  app_ble_cb.ble_server[index].attr[attr_num].attr_UUID.uu.uuid16,
                                  app_ble_cb.ble_server[index].attr[attr_num].is_pri,
                                  app_ble_cb.ble_server[index].attr[attr_num].service_id,
                                  app_ble_cb.ble_server[index].attr[attr_num].attr_id);
                    } else {
                        APP_INFO1("\t\t attr_num:%d:uuid:0x%04x, is_pri:%d, service_id:%d attr_id:%d",
                                  attr_num,
                                  app_ble_cb.ble_server[index].attr[attr_num].attr_UUID.uu.uuid16,
                                  app_ble_cb.ble_server[index].attr[attr_num].is_pri,
                                  app_ble_cb.ble_server[index].attr[attr_num].service_id,
                                  app_ble_cb.ble_server[index].attr[attr_num].attr_id);
                    }
                }
            }
        }
    }
    APP_INFO0("*************** BLE SERVER LIST END *************");
}

/*******************************************************************************
 **
 ** Function         app_ble_server_find_reg_pending_index
 **
 ** Description      find registration pending index
 **
 ** Parameters
 **
 ** Returns          positive number(include 0) if successful, error code otherwise
 **
 *******************************************************************************/
int app_ble_server_find_reg_pending_index(void) {
    int index;

    for (index = 0; index < BSA_BLE_SERVER_MAX; index++) {
        if ((app_ble_cb.ble_server[index].enabled) &&
            (app_ble_cb.ble_server[index].server_if == BSA_BLE_INVALID_IF)) {
            return index;
        }
    }
    return -1;
}

/*******************************************************************************
 **
 ** Function         app_ble_server_find_index_by_interface
 **
 ** Description      find BLE server index by interface
 **
 ** Parameters    if_num: interface number
 **
 ** Returns          positive number(include 0) if successful, error code otherwise
 **
 *******************************************************************************/
int app_ble_server_find_index_by_interface(tBSA_BLE_IF if_num) {
    int index;

    for (index = 0; index < BSA_BLE_SERVER_MAX; index++) {
        if (app_ble_cb.ble_server[index].server_if == if_num) {
            return index;
        }
    }
    return -1;
}

/*******************************************************************************
 **
 ** Function         app_ble_server_find_index_by_conn_id
 **
 ** Description      find BLE server index by connection ID
 **
 ** Parameters       conn_id: Connection ID
 **
 ** Returns          positive number(include 0) if successful, error code otherwise
 **
 *******************************************************************************/
int app_ble_server_find_index_by_conn_id(UINT16 conn_id) {
    int index;

    for (index = 0; index < BSA_BLE_CLIENT_MAX; index++) {
        if (app_ble_cb.ble_server[index].conn_id == conn_id) {
            return index;
        }
    }
    return -1;
}

void printDumpInfo(const char *title, const char *buf, int size) {
    int j, i, k;
    char prtinfo[512], pr2[256];

    APP_INFO1(" -%s-size=%d--- -start-------", title, size);
    for (k = i = 0; i < size; k++) {
        prtinfo[0] = 0;
        pr2[0] = 0;
        for (j = 0; (i < size && j < 16); j++, i++) {
            sprintf(pr2 + j, "%c", isgraph(buf[i]) ? buf[i] : ' ');
            sprintf(prtinfo + strlen(prtinfo), "%02X, ", ((unsigned int)buf[i]) & 0xff);
        }
        APP_INFO1("%3x| %s -- %s", k, prtinfo, pr2);
    }
    APP_INFO1(" -%s-size=%d--end----------", title, size);
}

int app_ble_server_register(UINT16 uuid, tBSA_BLE_CBACK *p_cback) {
    tBSA_STATUS status;
    tBSA_BLE_SE_REGISTER ble_register_param;
    int server_num;
    int idx;
    server_num = app_ble_server_find_free_server();
    if (server_num < 0) {
        APP_ERROR0("No more spaces!!");
        return -1;
    }
    APP_INFO1("Register app UUID(eg. 0x%04x)", uuid);
    if (!uuid) {
        uuid = APP_BLE_REGISTER_UUID;
    }

    status = BSA_BleSeAppRegisterInit(&ble_register_param);
    if (status != BSA_SUCCESS) {
        APP_ERROR1("BSA_BleSeAppRegisterInit failed status = %d", status);
        return -1;
    }

    ble_register_param.uuid.len = 2;
    ble_register_param.uuid.uu.uuid16 = uuid;
		if(p_cback != NULL)
			ohos_event_cback = p_cback;
		ble_register_param.p_cback = app_ble_server_profile_cback;

    //BTM_BLE_SEC_NONE: No authentication and no encryption
    //BTM_BLE_SEC_ENCRYPT: encrypt the link with current key
    //BTM_BLE_SEC_ENCRYPT_NO_MITM: unauthenticated encryption
    //BTM_BLE_SEC_ENCRYPT_MITM: authenticated encryption
    ble_register_param.sec_act = BTM_BLE_SEC_NONE;
    status = BSA_BleSeAppRegister(&ble_register_param);
    if (status != BSA_SUCCESS) {
        APP_ERROR1("BSA_BleSeAppRegister failed status = %d", status);
        return -1;
    }
    app_ble_cb.ble_server[server_num].enabled = TRUE;
    app_ble_cb.ble_server[server_num].server_if = ble_register_param.server_if;
    APP_INFO1("idx:%d,server_num:%d,enabled:%d, server_if:%d,conn_id:%d", idx, server_num, app_ble_cb.ble_server[server_num].enabled,
              app_ble_cb.ble_server[server_num].server_if,
              app_ble_cb.ble_server[server_num].conn_id);
    return 0;
}
/*******************************************************************************
 **
 ** Function        app_ble_server_deregister
 **
 ** Description     Deregister server app
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_server_deregister(void) {
    tBSA_STATUS status;
    tBSA_BLE_SE_DEREGISTER ble_deregister_param;
    int num;

    APP_INFO0("Bluetooth BLE deregister menu:");
    APP_INFO0("Select Server:");
    app_ble_server_display();
    num = app_get_choice("Select");
    if ((num < 0) || (num >= BSA_BLE_SERVER_MAX)) {
        APP_ERROR1("Wrong server number! = %d", num);
        return -1;
    }
    if (app_ble_cb.ble_server[num].enabled != TRUE) {
        APP_ERROR1("Server was not registered! = %d", num);
        return -1;
    }

    status = BSA_BleSeAppDeregisterInit(&ble_deregister_param);
    if (status != BSA_SUCCESS) {
        APP_ERROR1("BSA_BleSeAppDeregisterInit failed status = %d", status);
        return -1;
    }

    ble_deregister_param.server_if = app_ble_cb.ble_server[num].server_if;

    status = BSA_BleSeAppDeregister(&ble_deregister_param);
    if (status != BSA_SUCCESS) {
        APP_ERROR1("BSA_BleSeAppDeregister failed status = %d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_server_create_service
 **
 ** Description     create service
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_server_create_service(int server_num, UINT8* service_uuid, bool is_primary, int number) {
    tBSA_STATUS status;
    tBSA_BLE_SE_CREATE ble_create_param;
    int attr_num;

    if ((server_num < 0) || (server_num >= BSA_BLE_SERVER_MAX)) {
        APP_ERROR1("Wrong server number! = %d", server_num);
        return -1;
    }
    if (app_ble_cb.ble_server[server_num].enabled != TRUE) {
        APP_ERROR1("Server was not enabled! = %d", server_num);
        return -1;
    }
		if (!number)
		{
			APP_ERROR1("wrong value = %d", number);
			return -1;
		}
		if (!(is_primary == 0) && !(is_primary == 1))
		{
			APP_ERROR1("wrong value = %d", is_primary);
			return -1;
		}
    /* APP_INFO0("Enter num of handle(x2) to create."); */
    /* APP_INFO0("\tFor Example, if you will add 5 characteristics in this service"); */
    /* APP_INFO0("\tUse 12 (1 service x 2 + 5 characteristics x 2)"); */
    /* if (!num_handle) */
    /* { */
    /*     APP_ERROR1("wrong num_handle! = %d", num_handle); */
    /*     return -1; */
    /* } */
    attr_num = app_ble_server_find_free_attr(server_num);
    if (attr_num < 0) {
        APP_ERROR1("Wrong attr number! = %d", attr_num);
        return -1;
    }
    status = BSA_BleSeCreateServiceInit(&ble_create_param);
    if (status != BSA_SUCCESS) {
        APP_ERROR1("BSA_BleSeCreateServiceInit failed status = %d", status);
        return -1;
    }

    int index;
    for (index = 0; index < MAX_UUID_SIZE; index++) {
        ble_create_param.service_uuid.uu.uuid128[index] = service_uuid[index];
    }
    ble_create_param.service_uuid.len = MAX_UUID_SIZE;
    ble_create_param.server_if = app_ble_cb.ble_server[server_num].server_if;
    ble_create_param.num_handle = number;

		if(is_primary != 0) {
			ble_create_param.is_primary = TRUE;
		}else {
			ble_create_param.is_primary = FALSE;
		}

    app_ble_cb.ble_server[server_num].attr[attr_num].wait_flag = TRUE;

    status = BSA_BleSeCreateService(&ble_create_param);
    if (status != BSA_SUCCESS) {
        APP_ERROR1("BSA_BleSeCreateService failed status = %d", status);
        app_ble_cb.ble_server[server_num].attr[attr_num].wait_flag = FALSE;
        return -1;
    }

    /* store information on control block */
    app_ble_cb.ble_server[server_num].attr[attr_num].attr_UUID.len = MAX_UUID_SIZE;
    for (index = 0; index < MAX_UUID_SIZE; index++) {
        app_ble_cb.ble_server[server_num].attr[attr_num].attr_UUID.uu.uuid128[index] = service_uuid[index];
    }
    app_ble_cb.ble_server[server_num].attr[attr_num].is_pri = is_primary;
    app_ble_cb.ble_server[server_num].attr[attr_num].attr_type = BSA_GATTC_ATTR_TYPE_SRVC;
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_server_add_char
 **
 ** Description     Add character to service
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_server_add_char(int server_id, int server_num, UINT8* charac_uuid, int properties, int permissions)
{
	tBSA_STATUS status;
	tBSA_BLE_SE_ADDCHAR ble_addchar_param;
	int srvc_attr_num, char_attr_num;
	int characteristic_property = 0;
	int index;

	printf("%s-%d,app_ble_cb.ble_server[%d].enabled:%d\n", __FUNCTION__, __LINE__,
				 server_num, app_ble_cb.ble_server[server_num].enabled);
	if ((server_num < 0) || (server_num >= BSA_BLE_SERVER_MAX)) {
		APP_ERROR1("Wrong server number! = %d", server_num);
		return -1;
	}
	if (app_ble_cb.ble_server[server_num].enabled != TRUE) {
		APP_ERROR1("Server was not enabled! = %d", server_num);
		return -1;
	}

	srvc_attr_num = 0;
	if (srvc_attr_num < 0) {
		APP_ERROR0("app_ble_server_add_char : Invalid srvc_attr_num entered");
		return -1;
	}

	char_attr_num = app_ble_server_find_free_attr(server_num);
	status = BSA_BleSeAddCharInit(&ble_addchar_param);
	if (status != BSA_SUCCESS) {
		APP_ERROR1("BSA_BleSeAddCharInit failed status = %d", status);
		return -1;
	}
	ble_addchar_param.service_id = app_ble_cb.ble_server[server_num].attr[srvc_attr_num].service_id;
	ble_addchar_param.char_uuid.len = MAX_UUID_SIZE;
	for (index = 0; index < MAX_UUID_SIZE; index++) {
		ble_addchar_param.char_uuid.uu.uuid128[index] = charac_uuid[index]; //CHAR_UUID128[index];
	}

	ble_addchar_param.is_descr = FALSE;
	ALOGD("app_ble_server_add_char set BTA_GATT_PERM_READ | BTA_GATT_PERM_WRITE");
	ble_addchar_param.perm = permissions;

	ble_addchar_param.property = properties;
	status = BSA_BleSeAddChar(&ble_addchar_param);
	if (status != BSA_SUCCESS) {
		APP_ERROR1("BSA_BleSeAddChar failed status = %d", status);
		return -1;
	}
#if 0
#ifdef BLUETOOTH_HILINK_SUPPORT
	if (server_num == 0)
		attr_id_num[i] = char_attr_num;
	else
		attr_id_num[i + 2] = char_attr_num;
#endif
	ALOGD("app_ble_server_add_char server_num:%d,sum:%d, char_idx:%d,char_attr_num:%d", server_num, char_sum, i, char_attr_num);
#endif
	/* save all information */
	app_ble_cb.ble_server[server_num].attr[char_attr_num].attr_UUID.len = MAX_UUID_SIZE;
	for (index = 0; index < MAX_UUID_SIZE; index++) {
		app_ble_cb.ble_server[server_num].attr[char_attr_num].attr_UUID.uu.uuid128[index] = charac_uuid[index]; //CHAR_UUID128[index];
	}
	app_ble_cb.ble_server[server_num].attr[char_attr_num].prop = properties;
	app_ble_cb.ble_server[server_num].attr[char_attr_num].attr_type = BSA_GATTC_ATTR_TYPE_CHAR;
	app_ble_cb.ble_server[server_num].attr[char_attr_num].wait_flag = TRUE;

	return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_server_start_service
 **
 ** Description     Start Service
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/

int app_ble_server_start_service(int server_id, int num) {
    tBSA_STATUS status;
    tBSA_BLE_SE_START ble_start_param;
    int attr_num;

    if ((num < 0) || (num >= BSA_BLE_SERVER_MAX)) {
        APP_ERROR1("Wrong server number! = %d", num);
        return -1;
    }
    if (app_ble_cb.ble_server[num].enabled != TRUE) {
        APP_ERROR1("Server was not enabled! = %d", num);
        return -1;
    }
    attr_num = 0;
    if (attr_num < 0) {
        APP_ERROR0("app_ble_server_start_service : Invalid attr_num entered");
        return -1;
    }

    status = BSA_BleSeStartServiceInit(&ble_start_param);
    if (status != BSA_SUCCESS) {
        APP_ERROR1("BSA_BleSeStartServiceInit failed status = %d", status);
        return -1;
    }

    ble_start_param.service_id = app_ble_cb.ble_server[num].attr[attr_num].service_id;
    ble_start_param.sup_transport = BSA_BLE_GATT_TRANSPORT_LE_BR_EDR;

    printf("service_id:%d, num:%d\n", ble_start_param.service_id, num);
    APP_INFO1("service_id:%d, num:%d", ble_start_param.service_id, num);

    status = BSA_BleSeStartService(&ble_start_param);
    if (status != BSA_SUCCESS) {
        APP_ERROR1("BSA_BleSeStartService failed status = %d", status);
        return -1;
    }
    return 0;
}
/*******************************************************************************
 **
 ** Function        app_ble_server_send_indication
 **
 ** Description     Send indication to client
 **
 ** Parameters      con_id ble connection id,the indication value that send to client
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
static UINT32 bt_send_times = 0, bt_send_totals = 0;
int app_ble_server_send_indication(int conn_id, char *indication, int size) {
    ++bt_send_times;
    bt_send_totals += size;
    APP_INFO1("start app_ble_server_send_indication..size = %d, times:%u, totals:%u", size, bt_send_times, bt_send_totals);
    //APP_DUMP("send_indication", indication, size);
    tBSA_STATUS status;
    tBSA_BLE_SE_SENDIND ble_sendind_param;
    int num, length_of_data, index, attr_num;

    //num = 0; //pzhang
    num = app_ble_server_find_index_by_conn_id(conn_id);
    if ((num < 0) || (num >= BSA_BLE_SERVER_MAX)) {
        APP_ERROR1("Wrong server number! = %d", num);
        return -1;
    }
    if (app_ble_cb.ble_server[num].enabled != TRUE) {
        APP_ERROR1("Server was not enabled! = %d", num);
        return -1;
    }
    if (app_ble_cb.ble_server[num].congested) {
        APP_ERROR1("fail : congested(%d)!", app_ble_cb.ble_server[num].congested);
        return -1;
    }
#ifdef BLUETOOTH_HILINK_SUPPORT
    attr_num = 1;//clivia 0;
    //attr_num = attr_id_num[0]; //clivia
#else
    attr_num = attr_id_num[0]; //clivia
#endif
    //APP_INFO1("BSA_BleSeSendIndInit attr_num = %d", attr_num);

    status = BSA_BleSeSendIndInit(&ble_sendind_param);
    if (status != BSA_SUCCESS) {
        APP_ERROR1("BSA_BleSeSendIndInit failed status = %d", status);
        return -1;
    }
    APP_INFO1("start server_num:%d, conn_id:%d, attr_num:%d,attr_id:%d", num, conn_id, attr_num, app_ble_cb.ble_server[num].attr[attr_num].attr_id);
    ble_sendind_param.conn_id = conn_id;
#ifdef BLUETOOTH_HILINK_SUPPORT
    ble_sendind_param.attr_id = app_ble_cb.ble_server[num].attr[attr_num].attr_id;
#else
    ble_sendind_param.attr_id = app_ble_cb.ble_server[num].attr[attr_num].attr_id;
#endif

    ble_sendind_param.data_len = size;

    memcpy(ble_sendind_param.value, indication, size);
    //APP_INFO1("BSA_BleSeSendInd ble_sendind_param.value = %s", ble_sendind_param.value);

#if APP_BLE_SENDIND_NEED_CONFIRM
    ble_sendind_param.need_confirm = TRUE;
#else
    ble_sendind_param.need_confirm = FALSE;
#endif
    status = BSA_BleSeSendInd(&ble_sendind_param);
    if (status != BSA_SUCCESS) {
        APP_ERROR1("BSA_BleSeSendInd failed status = %d", status);
        return -1;
    }
    APP_INFO0("end app_ble_server_send_indication..");
    return 0;
}
static UINT32 bt_write_times = 0, bt_write_totals = 0;
/*******************************************************************************
**
** Function         app_ble_server_profile_cback
**
** Description      BLE Server Profile callback.
**
** Returns          void
**
*******************************************************************************/
void app_ble_server_profile_cback(tBSA_BLE_EVT event, tBSA_BLE_MSG *p_data) {
    int num, attr_index;
    int status;
    tBSA_BLE_SE_SENDRSP send_server_resp;
    UINT8 attribute_value[BSA_BLE_MAX_ATTR_LEN] = {0x11, 0x22, 0x33, 0x44};

    APP_DEBUG1("app_ble_server_profile_cback event = %d ", event);

    switch (event) {
    case BSA_BLE_SE_DEREGISTER_EVT:
        APP_INFO1("BSA_BLE_SE_DEREGISTER_EVT server_if:%d status:%d",
                  p_data->ser_deregister.server_if, p_data->ser_deregister.status);
        num = app_ble_server_find_index_by_interface(p_data->ser_deregister.server_if);
        if (num < 0) {
            APP_ERROR0("no deregister pending!!");
            break;
        }

        app_ble_cb.ble_server[num].server_if = BSA_BLE_INVALID_IF;
        app_ble_cb.ble_server[num].enabled = FALSE;
        for (attr_index = 0; attr_index < BSA_BLE_ATTRIBUTE_MAX; attr_index++) {
            memset(&app_ble_cb.ble_server[num].attr[attr_index], 0, sizeof(tAPP_BLE_ATTRIBUTE));
        }

        break;

    case BSA_BLE_SE_CREATE_EVT:
        APP_INFO1("BSA_BLE_SE_CREATE_EVT server_if:%d status:%d service_id:%d",
                  p_data->ser_create.server_if, p_data->ser_create.status, p_data->ser_create.service_id);

        num = app_ble_server_find_index_by_interface(p_data->ser_create.server_if);

        /* search interface number */
        if (num < 0) {
            APP_ERROR0("no interface!!");
            break;
        }

        /* search attribute number */
        for (attr_index = 0; attr_index < BSA_BLE_ATTRIBUTE_MAX; attr_index++) {
            if (app_ble_cb.ble_server[num].attr[attr_index].wait_flag == TRUE) {
                APP_INFO1("BSA_BLE_SE_CREATE_EVT if_num:%d, attr_num:%d", num, attr_index);
                if (p_data->ser_create.status == BSA_SUCCESS) {
                    app_ble_cb.ble_server[num].attr[attr_index].service_id = p_data->ser_create.service_id;
                    app_ble_cb.ble_server[num].attr[attr_index].wait_flag = FALSE;
                    break;
                } else /* if CREATE fail */
                {
                    memset(&app_ble_cb.ble_server[num].attr[attr_index], 0, sizeof(tAPP_BLE_ATTRIBUTE));
                    break;
                }
            }
        }
        if (attr_index >= BSA_BLE_ATTRIBUTE_MAX) {
            APP_ERROR0("BSA_BLE_SE_CREATE_EVT no waiting!!");
            break;
        }
        break;

    case BSA_BLE_SE_ADDCHAR_EVT:
        APP_INFO1("BSA_BLE_SE_ADDCHAR_EVT status:%d", p_data->ser_addchar.status);
        APP_INFO1("attr_id:0x%x", p_data->ser_addchar.attr_id);
        num = app_ble_server_find_index_by_interface(p_data->ser_addchar.server_if);

        /* search interface number */
        if (num < 0) {
            APP_ERROR0("no interface!!");
            break;
        }

        for (attr_index = 0; attr_index < BSA_BLE_ATTRIBUTE_MAX; attr_index++) {
            if (app_ble_cb.ble_server[num].attr[attr_index].wait_flag == TRUE) {
                //APP_INFO1("if_num:%d, attr_num:%d", num, attr_index);
                if (p_data->ser_addchar.status == BSA_SUCCESS) {
                    APP_INFO1("server_num:%d, attr_num:%d,server_id:%d,attr_id:%d", num, attr_index, p_data->ser_addchar.service_id, p_data->ser_addchar.attr_id);
                    app_ble_cb.ble_server[num].attr[attr_index].service_id = p_data->ser_addchar.service_id;
                    app_ble_cb.ble_server[num].attr[attr_index].attr_id = p_data->ser_addchar.attr_id;
                    app_ble_cb.ble_server[num].attr[attr_index].wait_flag = FALSE;
                    break;
                } else /* if CREATE fail */
                {
                    memset(&app_ble_cb.ble_server[num].attr[attr_index], 0, sizeof(tAPP_BLE_ATTRIBUTE));
                    break;
                }
            }
        }
        if (attr_index >= BSA_BLE_ATTRIBUTE_MAX) {
            APP_ERROR0("BSA_BLE_SE_ADDCHAR_EVT no waiting!!");
            break;
        }
        break;

    case BSA_BLE_SE_START_EVT:
        APP_INFO1("BSA_BLE_SE_START_EVT status:%d", p_data->ser_start.status);
#ifndef BLUETOOTH_HILINK_SUPPORT
        if (p_data->ser_start.status == BSA_SUCCESS) {
            //config the adv data
            app_ble_config_adv_data();
        }
#endif
        break;

    case BSA_BLE_SE_READ_EVT:
        APP_INFO1("BSA_BLE_SE_READ_EVT status:%d", p_data->ser_read.status);
        BSA_BleSeSendRspInit(&send_server_resp);
        send_server_resp.conn_id = p_data->ser_read.conn_id;
        send_server_resp.trans_id = p_data->ser_read.trans_id;
        send_server_resp.status = p_data->ser_read.status;
        send_server_resp.handle = p_data->ser_read.handle;
        send_server_resp.offset = p_data->ser_read.offset;
        send_server_resp.len = 0;
        send_server_resp.auth_req = GATT_AUTH_REQ_NONE;

        /*send_server_resp.len = 4;
        memcpy(send_server_resp.value, attribute_value, BSA_BLE_MAX_ATTR_LEN); */
        APP_INFO1("BSA_BLE_SE_READ_EVT: send_server_resp.conn_id:%d, send_server_resp.trans_id:%d", send_server_resp.conn_id, send_server_resp.trans_id, send_server_resp.status);
        APP_INFO1("BSA_BLE_SE_READ_EVT: send_server_resp.status:%d,send_server_resp.auth_req:%d", send_server_resp.status, send_server_resp.auth_req);
        APP_INFO1("BSA_BLE_SE_READ_EVT: send_server_resp.handle:%d, send_server_resp.offset:%d, send_server_resp.len:%d", send_server_resp.handle, send_server_resp.offset, send_server_resp.len);
        BSA_BleSeSendRsp(&send_server_resp);

        break;

    case BSA_BLE_SE_WRITE_EVT:
        num = app_ble_server_find_index_by_conn_id(p_data->ser_write.conn_id);
        APP_INFO1("BSA_BLE_SE_WRITE_EVT: ser_write.conn_id:%d, server_num:%d", p_data->ser_write.conn_id, num);
        ++bt_write_times;
        bt_write_totals += p_data->ser_write.len;
        APP_INFO1("BSA_BLE_SE_WRITE_EVT status:%d,size:%d, times:%u, totals:%u", p_data->ser_write.status,
                  p_data->ser_write.len, bt_write_times, bt_write_totals);
        //APP_DUMP("Write value", p_data->ser_write.value, p_data->ser_write.len);
        APP_INFO1("BSA_BLE_SE_WRITE_EVT trans_id:%d, conn_id:%d, handle:%d",
                  p_data->ser_write.trans_id, p_data->ser_write.conn_id, p_data->ser_write.handle);

        if (p_data->ser_write.need_rsp) {
            BSA_BleSeSendRspInit(&send_server_resp);
            send_server_resp.conn_id = p_data->ser_write.conn_id;
            send_server_resp.trans_id = p_data->ser_write.trans_id;
            send_server_resp.status = p_data->ser_write.status;
            send_server_resp.handle = p_data->ser_write.handle;
            send_server_resp.len = 0;
            APP_INFO1("BSA_BLE_SE_WRITE_EVT: send_server_resp.conn_id:%d, send_server_resp.trans_id:%d", send_server_resp.conn_id, send_server_resp.trans_id, send_server_resp.status);
            APP_INFO1("BSA_BLE_SE_WRITE_EVT: send_server_resp.status:%d,send_server_resp.auth_req:%d", send_server_resp.status, send_server_resp.auth_req);
            APP_INFO1("BSA_BLE_SE_WRITE_EVT: send_server_resp.handle:%d, send_server_resp.offset:%d, send_server_resp.len:%d", send_server_resp.handle, send_server_resp.offset, send_server_resp.len);
            BSA_BleSeSendRsp(&send_server_resp);
        }
        //pzhang
        app_ble_cb.ble_server[num].conn_id = p_data->ser_write.conn_id;
        char write_addr[20];
        sprintf(write_addr, "%02x:%02x:%02x:%02x:%02x:%02x", p_data->ser_write.remote_bda[0], p_data->ser_write.remote_bda[1], p_data->ser_write.remote_bda[2], p_data->ser_write.remote_bda[3], p_data->ser_write.remote_bda[4], p_data->ser_write.remote_bda[5]);
        int length = (int)(p_data->ser_write.len);
        length = length > BLE_DATA_MAX_SIZE * 2 ? BLE_DATA_MAX_SIZE * 2 : length;
        char request_value[BLE_DATA_MAX_SIZE * 2];
        memset(request_value, 0, BLE_DATA_MAX_SIZE * 2);
        memcpy(request_value, p_data->ser_write.value, length);
        //APP_INFO1("request_value = %s\n", request_value);

#ifdef BLUETOOTH_HILINK_SUPPORT
        //TODO: 判断是hilink的信息，直接发送，不做其余处理
        if ((num >= 0) && (num < HILINK_SERVICE_SUM)) { //hilink 3 service
            if (mHilinkBLEWriteRequestCback != NULL)
                mHilinkBLEWriteRequestCback(num, write_addr, p_data->ser_write.conn_id, request_value, length); //p_data->ser_write.len);
        } else
#endif
            if (mWriteRequestCback != NULL)
            mWriteRequestCback(write_addr, p_data->ser_write.conn_id, request_value, p_data->ser_write.len);
        break;

    case BSA_BLE_SE_EXEC_WRITE_EVT:
        APP_INFO1("BSA_BLE_SE_EXEC_WRITE_EVT status:%d", p_data->ser_exec_write.status);
        APP_INFO1("BSA_BLE_SE_WRITE_EVT trans_id:%d, conn_id:%d, exec_write:%d",
                  p_data->ser_exec_write.trans_id, p_data->ser_exec_write.conn_id, p_data->ser_exec_write.exec_write);

        BSA_BleSeSendRspInit(&send_server_resp);
        send_server_resp.conn_id = p_data->ser_exec_write.conn_id;
        send_server_resp.trans_id = p_data->ser_exec_write.trans_id;
        send_server_resp.status = p_data->ser_exec_write.status;
        send_server_resp.handle = 0;
        send_server_resp.len = 0;
        APP_INFO1("BSA_BLE_SE_WRITE_EVT: send_server_resp.conn_id:%d, send_server_resp.trans_id:%d", send_server_resp.conn_id, send_server_resp.trans_id, send_server_resp.status);
        APP_INFO1("BSA_BLE_SE_WRITE_EVT: send_server_resp.status:%d", send_server_resp.status);
        BSA_BleSeSendRsp(&send_server_resp);

        break;

    case BSA_BLE_SE_OPEN_EVT:
        APP_INFO1("BSA_BLE_SE_OPEN_EVT status:%d", p_data->ser_open.reason);
        if (p_data->ser_open.reason == BSA_SUCCESS) {
            num = app_ble_server_find_index_by_interface(p_data->ser_open.server_if);
            /* search interface number */
            if (num < 0) {
                APP_ERROR0("no interface!!");
                break;
            }
            APP_INFO1("BSA_BLE_SE_OPEN_EVT conn_id:%d,server_if:%d,num:%d", p_data->ser_open.conn_id, p_data->ser_open.server_if, num);
            APP_INFO1("attr_num0:%d,attr_num1:%d", attr_id_num[0], attr_id_num[1]);
            app_ble_cb.ble_server[num].conn_id = p_data->ser_open.conn_id;
            char open_addr[20];
            sprintf(open_addr, "%02x:%02x:%02x:%02x:%02x:%02x", p_data->ser_open.remote_bda[0], p_data->ser_open.remote_bda[1], p_data->ser_open.remote_bda[2], p_data->ser_open.remote_bda[3], p_data->ser_open.remote_bda[4], p_data->ser_open.remote_bda[5]);
#ifdef BLUETOOTH_HILINK_SUPPORT
            if ((num >= 0) && (num < HILINK_SERVICE_SUM)) {
                if (mHilinkConnectionStateChange != NULL)
                    mHilinkConnectionStateChange(open_addr, 1, p_data->ser_open.conn_id, num);
            } else
#endif
                if (mConnectionStateChange != NULL)
                mConnectionStateChange(open_addr, 1);
            /* XML Database update */
            app_read_xml_remote_devices();
            /* Add BLE service for this devices in XML database */
            app_xml_add_trusted_services_db(app_xml_remote_devices_db,
                                            APP_NUM_ELEMENTS(app_xml_remote_devices_db), p_data->ser_open.remote_bda,
                                            BSA_BLE_SERVICE_MASK);

            status = app_write_xml_remote_devices();
            if (status < 0) {
                APP_ERROR1("app_ble_write_remote_devices failed: %d", status);
                return;
            }
        }
        break;

    case BSA_BLE_SE_CONGEST_EVT:
        APP_INFO1("BSA_BLE_SE_CONGEST_EVT  :conn_id:0x%x, congested:%d",
                  p_data->ser_congest.conn_id, p_data->ser_congest.congested);
        num = app_ble_server_find_index_by_conn_id(p_data->ser_congest.conn_id);
        if (num >= 0) {
            app_ble_cb.ble_server[num].congested = p_data->ser_congest.congested;
        }
        break;

    case BSA_BLE_SE_CLOSE_EVT:
        APP_INFO1("BSA_BLE_SE_CLOSE_EVT status:%d", p_data->ser_close.reason);
        APP_INFO1("conn_id:0x%x", p_data->ser_close.conn_id);
        num = app_ble_server_find_index_by_interface(p_data->ser_close.server_if);
        /* search interface number */
        if (num < 0) {
            APP_ERROR0("no interface!!");
            break;
        }
        char close_addr[20];
        sprintf(close_addr, "%02x:%02x:%02x:%02x:%02x:%02x", p_data->ser_close.remote_bda[0], p_data->ser_close.remote_bda[1], p_data->ser_close.remote_bda[2], p_data->ser_close.remote_bda[3], p_data->ser_close.remote_bda[4], p_data->ser_close.remote_bda[5]);
#ifdef BLUETOOTH_HILINK_SUPPORT
        if ((num >= 0) && (num < HILINK_SERVICE_SUM)) {
            if (mHilinkConnectionStateChange != NULL)
                mHilinkConnectionStateChange(close_addr, 0, p_data->ser_close.conn_id, num);
        } else
#endif
            if (mConnectionStateChange != NULL)
            mConnectionStateChange(close_addr, 0);
        app_ble_cb.ble_server[num].conn_id = BSA_BLE_INVALID_CONN;
        //config the adv data
#ifndef BLUETOOTH_HILINK_SUPPORT
        app_ble_config_adv_data();
#endif
        break;

    case BSA_BLE_SE_CONF_EVT:
        APP_INFO1("BSA_BLE_SE_CONF_EVT status:%d,conn_id:0x%x", p_data->ser_conf.status, p_data->ser_conf.conn_id);
        break;

    default:
        break;
    }
		if(ohos_event_cback != NULL)
			ohos_event_cback(event, p_data);
}

/*******************************************************************************
 **
 ** Function        app_ble_server_open
 **
 ** Description     This is the ble open connection to ble client
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_server_open(void) {
    tBSA_STATUS status;
    tBSA_BLE_SE_OPEN ble_open_param;
    int device_index;
    BD_ADDR bd_addr;
    int server_num;
    int direct;

    APP_INFO0("Bluetooth BLE connect menu:");
    APP_INFO0("    0 Device from XML database (already paired)");
    APP_INFO0("    1 Device found in last discovery");
    device_index = app_get_choice("Select source");
    /* Devices from XML database */
    if (device_index == 0) {
        /* Read the XML file which contains all the bonded devices */
        app_read_xml_remote_devices();

        app_xml_display_devices(app_xml_remote_devices_db,
                                APP_NUM_ELEMENTS(app_xml_remote_devices_db));
        device_index = app_get_choice("Select device");
        if ((device_index >= 0) &&
            (device_index < APP_NUM_ELEMENTS(app_xml_remote_devices_db)) &&
            (app_xml_remote_devices_db[device_index].in_use != FALSE)) {
            bdcpy(bd_addr, app_xml_remote_devices_db[device_index].bd_addr);
        } else {
            APP_ERROR1("Bad Device Index:%d\n", device_index);
            return -1;
        }

    }
    /* Devices from Discovery */
    else if (device_index == 1) {
        app_disc_display_devices();
        device_index = app_get_choice("Select device");
        if ((device_index >= 0) &&
            (device_index < APP_DISC_NB_DEVICES) &&
            (app_discovery_cb.devs[device_index].in_use != FALSE)) {
            bdcpy(bd_addr, app_discovery_cb.devs[device_index].device.bd_addr);
        }
    } else {
        APP_ERROR0("Bad choice [XML(0) or Disc(1) only]");
        return -1;
    }

    APP_INFO0("Select Server:");
    app_ble_server_display();
    server_num = app_get_choice("Select");

    if ((server_num < 0) ||
        (server_num >= BSA_BLE_SERVER_MAX) ||
        (app_ble_cb.ble_server[server_num].enabled == FALSE)) {
        APP_ERROR1("Wrong server number! = %d", server_num);
        return -1;
    }

    if (app_ble_cb.ble_server[server_num].conn_id != BSA_BLE_INVALID_CONN) {
        APP_ERROR1("Connection already exist, conn_id = %d",
                   app_ble_cb.ble_server[server_num].conn_id);
        return -1;
    }

    status = BSA_BleSeConnectInit(&ble_open_param);
    if (status != BSA_SUCCESS) {
        APP_ERROR1("BSA_BleSeConnectInit failed status = %d", status);
        return -1;
    }

    ble_open_param.server_if = app_ble_cb.ble_server[server_num].server_if;
    direct = app_get_choice("Direct connection:1, Background connection:0");
    if (direct == 1) {
        ble_open_param.is_direct = TRUE;
    } else if (direct == 0) {
        ble_open_param.is_direct = FALSE;
    } else {
        APP_ERROR1("Wrong selection! = %d", direct);
        return -1;
    }
    bdcpy(ble_open_param.bd_addr, bd_addr);

    status = BSA_BleSeConnect(&ble_open_param);
    if (status != BSA_SUCCESS) {
        APP_ERROR1("BSA_BleSeConnect failed status = %d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_server_close
 **
 ** Description     This is the ble close connection
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_server_close(void) {
    tBSA_STATUS status;
    tBSA_BLE_SE_CLOSE ble_close_param;
    int server_num = 0;

    /* APP_INFO0("Select Server:"); */
    /* app_ble_server_display(); */
    /* server_num = app_get_choice("Select"); */
    for (server_num = 0; server_num < BSA_BLE_SERVER_MAX; server_num++) {
        if ((server_num < 0) ||
            (server_num >= BSA_BLE_SERVER_MAX) ||
            (app_ble_cb.ble_server[server_num].enabled == FALSE)) {
            APP_ERROR1("Wrong server number! = %d", server_num);
            return -1;
        }
        status = BSA_BleSeCloseInit(&ble_close_param);
        if (status != BSA_SUCCESS) {
            APP_ERROR1("BSA_BleSeCloseInit failed status = %d", status);
            return -1;
        }
        ble_close_param.conn_id = app_ble_cb.ble_server[server_num].conn_id;
        status = BSA_BleSeClose(&ble_close_param);
        if (status != BSA_SUCCESS) {
            APP_ERROR1("BSA_BleSeClose failed status = %d", status);
            return -1;
        }
    }
    return 0;
}

//pzhang
/* 广播包数据长度最长为31。超过此长度会将多余的丢掉。 */
/* 长度计算规则:所有键值对长度总和。 */
/* 键值对长度:flag 占1，长度占1，然后就是内容占的长度。 */

/* 该长度为(1+1+1)+(1+1+23) = 28 */
/* advertisementData: { */
/*     kCBAdvDataIsConnectable = 1;    */
/*     kCBAdvDataLocalName = "MPENLSCE:F9:49:5D:97:1C"; */
/* }  */
/* 该长度为(1+1+1)+(1+1+17)+(1+1+6) = 30 */
/* advertisementData: { */
/*     kCBAdvDataIsConnectable = 1; */
/*     kCBAdvDataLocalName = "MYPAI_BSA_0000-nc"; */
/*     kCBAdvDataManufacturerData = <2b1aaabb ccdd>; */
/* } */
#ifndef BLUETOOTH_HILINK_SUPPORT
int app_ble_config_adv_data(void) {
    APP_INFO0("start config the ble adv data..");
    char name[50];
    memset(name, 0, 50);
    app_mgr_get_bleName(name);
    int i;
    tBSA_DM_BLE_ADV_CONFIG adv_conf;
    UINT8 app_ble_adv_value[APP_BLE_ADV_VALUE_LEN] = {0x2b, 0x1a, 0xaa, 0xbb, 0xcc, 0xdd}; /*First 2 byte is Company Identifier Eg: 0x1a2b refers to Bluetooth ORG, followed by 4bytes of data*/
    memset(&adv_conf, 0, sizeof(tBSA_DM_BLE_ADV_CONFIG));
    adv_conf.len = APP_BLE_ADV_VALUE_LEN;
    adv_conf.flag = BSA_DM_BLE_ADV_FLAG_MASK;
    /* start advertising */ //31 - 8 - 3 - 2=18
    if (strlen((char *)name) <= 18) {
        memcpy(adv_conf.p_val, app_ble_adv_value, APP_BLE_ADV_VALUE_LEN);
        /* All the masks/fields that are set will be advertised*/
        adv_conf.adv_data_mask = BSA_DM_BLE_AD_BIT_FLAGS | BSA_DM_BLE_AD_BIT_DEV_NAME | BSA_DM_BLE_AD_BIT_MANU;
    } else {
        adv_conf.adv_data_mask = BSA_DM_BLE_AD_BIT_FLAGS | BSA_DM_BLE_AD_BIT_DEV_NAME;
    }
    adv_conf.num_service = APP_BLE_SERVICE_NUM;
    adv_conf.uuid_val[0] = APP_BLE_ADV_CONF_UUID_VAL;
    adv_conf.service_data_len = 4 + 16 + 0; /* length = AD type + service data uuid + data) */
    int index;
    for (index = 0; index < MAX_UUID_SIZE; index++) {
        adv_conf.services_128b.uuid128[index] = SERVICE_UUID128[index];
        adv_conf.service_data_uuid.uu.uuid128[index] = SERVICE_UUID128[index];
    }
    adv_conf.is_part_service = TRUE;
    adv_conf.is_scan_rsp = APP_BLE_ADV_CONF_IS_SCAN_RSP; //0:scan don't response

    app_dm_set_ble_adv_data(&adv_conf);
    APP_INFO0("end config the ble adv data..");
    return 0;
}
#endif

int app_mgr_get_ble_name(char *ble_name, int len) {
    int status;
    tBSA_DM_GET_CONFIG bt_config;

    /*
	 * Get bluetooth configuration
	 */
    status = BSA_DmGetConfigInit(&bt_config);
    status = BSA_DmGetConfig(&bt_config);
    if (status != BSA_SUCCESS) {
        APP_ERROR1("BSA_DmGetConfig failed:%d", status);
        return (-1);
    }

    int str_len = strlen(bt_config.ble_name);
    if (str_len < len)
        strncpy(ble_name, bt_config.ble_name, str_len);
    else
        strncpy(ble_name, bt_config.ble_name, len);
    return (str_len < len) ? str_len : len;
}

int app_mgr_set_ble_name(char *ble_name, int len) {
    int status;
    tBSA_DM_SET_CONFIG bt_config;

    /* Init config parameter */
    status = BSA_DmSetConfigInit(&bt_config);
    bt_config.enable = TRUE;
    bt_config.config_mask = BSA_DM_CONFIG_NAME_MASK;
    strncpy(bt_config.name, ble_name, sizeof(BD_NAME));     //len);
    strncpy(bt_config.ble_name, ble_name, sizeof(BD_NAME)); //len);
    status = BSA_DmSetConfig(&bt_config);
    if (status != BSA_SUCCESS) {
        APP_ERROR1("BSA_DmSetConfig failed:%d", status);
        return (-1);
    }
    return 0;
}

int hilink_ble_server_register_write_cback(void (*receiveWriteRequestCback)(int server_no, char *addr, int conn_id, char *data, int size)) {
    mHilinkBLEWriteRequestCback = receiveWriteRequestCback;
    return 0;
}
void hilink_app_ble_server_register_connectstate_cback(void (*hilinkOnConnectionStateChange)(char *addr, int link_state, int con_id, int server_num)) {
    mHilinkConnectionStateChange = hilinkOnConnectionStateChange;
}

//pyzhang
int app_ble_server_register_write_cback(void (*receiveWriteRequestCback)(char *addr, int conn_id, char *data, int size)) {
    mWriteRequestCback = receiveWriteRequestCback;
    return 0;
}
//pyzhang
void app_ble_server_register_connectstate_cback(void (*onConnectionStateChange)(char *add, int state)) {
    mConnectionStateChange = onConnectionStateChange;
}
