/*****************************************************************************
 **
 **  Name:           app_manager.c
 **
 **  Description:    Bluetooth Manager application
 **
 **  Copyright (c) 2010-2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#include "gki.h"
#include "uipc.h"

#include "bsa_api.h"

#include "app_xml_utils.h"

#include "app_disc.h"
#include "app_utils.h"
#include "app_dm.h"
//#include "app_av.h"

#include "app_services.h"
#include "app_mgt.h"

#include "app_manager.h"
#include "app_link.h"
#include "app_disc_cback.h"

/* Default BdAddr */
#define APP_DEFAULT_BD_ADDR             {0xBE, 0xEF, 0xBE, 0xEF, 0x00, 0x01}

/* Default local Name */
/* #define APP_DEFAULT_BT_NAME             "GLASS_BSA_1111" */

/* #define APP_DEFAULT_BLE_NAME             "GLASS_BLE_1111" */
/* Default COD SetTopBox (Major Service = none) (MajorDevclass = Audio/Video) (Minor=STB) */
#define APP_DEFAULT_CLASS_OF_DEVICE     {0x7a,0x07, 0x04}//{0x00, 0x04, 0x24}

#define APP_DEFAULT_ROOT_PATH           "/usr/data/bsa/pictures"

#define APP_DEFAULT_PIN_CODE            "0000"
#define APP_DEFAULT_PIN_LEN             4

#define APP_XML_CONFIG_FILE_PATH            "/usr/data/bsa/bt_config.xml"
#define APP_XML_REM_DEVICES_FILE_PATH       "/usr/data/bsa/bt_devices.xml"

/*
 * Simple Pairing Input/Output Capabilities:
 * BSA_SEC_IO_CAP_OUT  =>  DisplayOnly
 * BSA_SEC_IO_CAP_IO   =>  DisplayYesNo
 * BSA_SEC_IO_CAP_IN   =>  KeyboardOnly
 * BSA_SEC_IO_CAP_NONE =>  NoInputNoOutput
 * * #if BLE_INCLUDED == TRUE && SMP_INCLUDED == TRUE
 * BSA_SEC_IO_CAP_KBDISP =>  KeyboardDisplay
 * #endif
 */
#ifndef APP_SEC_IO_CAPABILITIES
#define APP_SEC_IO_CAPABILITIES BSA_SEC_IO_CAP_NONE
#endif

/* Automatically accept Simple Pairing Requests */
#ifndef APP_SEC_SP_AUTO_ACCEPT
#define APP_SEC_SP_AUTO_ACCEPT TRUE
#endif
/*
 * Global variables
 */
tAPP_XML_CONFIG         app_xml_config;
BD_ADDR                 app_sec_db_addr;    /* BdAddr of peer device requesting SP */

tAPP_MGR_CB app_mgr_cb;

tBSA_SEC_SET_REMOTE_OOB bsa_sec_set_remote_oob;

tBSA_SEC_PASSKEY_REPLY g_passkey_reply;

tLINKING_DEVICE linking_dev[APP_MAX_NB_REMOTE_STORED_DEVICES];
/*
 * Local functions
 */
#ifdef __cplusplus
extern "C"
{
#endif
#ifdef BLUETOOTH_HILINK_SUPPORT
int setBleVisibility(int discoverable, int connectable);
#endif
#ifdef __cplusplus
}
#endif
int app_mgr_config(char* bt_name);
char *app_mgr_get_dual_stack_mode_desc(void);

void (*mAppDiscCback)(int event, tAPP_DISC_MSG* p_data) = NULL;

/*******************************************************************************
 **
 ** Function         app_mgr_get_bleName
 **
 ** Description      This function is used to read the XML bluetooth configuration file
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_mgr_get_bleName(char * name)
{
	int status;
	status = app_xml_read_cfg(APP_XML_CONFIG_FILE_PATH, &app_xml_config);
	if (status < 0)
	{
		APP_ERROR1("app_xml_read_cfg failed:%d", status);
		return -1;
	}
	else
	{
		strncpy(name, (char *)app_xml_config.ble_name, strlen(app_xml_config.ble_name));
	}
	return 0;
}
/*******************************************************************************
 **
 ** Function         app_mgr_get_addr
 **
 ** Description      This function is used to read the XML bluetooth configuration file
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_mgr_get_addr(char * addr)
{
	int status;

	status = app_xml_read_cfg(APP_XML_CONFIG_FILE_PATH, &app_xml_config);
	if (status < 0)
	{
		APP_ERROR1("app_xml_read_cfg failed:%d", status);
		return -1;
	}
	else
	{
	    sprintf(addr,"%02x:%02x:%02x:%02x:%02x:%02x",
				app_xml_config.bd_addr[0], app_xml_config.bd_addr[1],
				app_xml_config.bd_addr[2], app_xml_config.bd_addr[3],
				app_xml_config.bd_addr[4], app_xml_config.bd_addr[5]);
	}
	return 0;
}

/*******************************************************************************
 **
 ** Function         app_mgr_read_config
 **
 ** Description      This function is used to read the XML bluetooth configuration file
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_mgr_read_config(void)
{
	int status;

	status = app_xml_read_cfg(APP_XML_CONFIG_FILE_PATH, &app_xml_config);
	if (status < 0)
	{
		APP_ERROR1("app_xml_read_cfg failed:%d", status);
		return -1;
	}
	else
	{
		APP_DEBUG1("Enable:%d", app_xml_config.enable);
		APP_DEBUG1("Discoverable:%d", app_xml_config.discoverable);
		APP_DEBUG1("Connectable:%d", app_xml_config.connectable);
		APP_DEBUG1("Name:%s", app_xml_config.name);
		APP_DEBUG1("BLE Name:%s", app_xml_config.ble_name);
		APP_DEBUG1("Bdaddr %02x:%02x:%02x:%02x:%02x:%02x",
				app_xml_config.bd_addr[0], app_xml_config.bd_addr[1],
				app_xml_config.bd_addr[2], app_xml_config.bd_addr[3],
				app_xml_config.bd_addr[4], app_xml_config.bd_addr[5]);
		APP_DEBUG1("ClassOfDevice:%02x:%02x:%02x => %s", app_xml_config.class_of_device[0],
				app_xml_config.class_of_device[1], app_xml_config.class_of_device[2],
				app_get_cod_string(app_xml_config.class_of_device));
		APP_DEBUG1("RootPath:%s", app_xml_config.root_path);

		APP_DEBUG1("Default PIN (%d):%s", app_xml_config.pin_len, app_xml_config.pin_code);
	}
	return 0;
}



/*******************************************************************************
 **
 ** Function         app_mgr_write_config
 **
 ** Description      This function is used to write the XML bluetooth configuration file
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_mgr_write_config(void)
{
	int status;

	status = app_xml_write_cfg(APP_XML_CONFIG_FILE_PATH, &app_xml_config);
	if (status < 0)
	{
		APP_ERROR1("app_xml_write_cfg failed:%d", status);
		return -1;
	}
	else
	{
		APP_DEBUG1("Enable:%d", app_xml_config.enable);
		APP_DEBUG1("Discoverable:%d", app_xml_config.discoverable);
		APP_DEBUG1("Connectable:%d", app_xml_config.connectable);
		APP_DEBUG1("Name:%s", app_xml_config.name);
		APP_DEBUG1("BLE Name:%s", app_xml_config.ble_name);
		APP_DEBUG1("Bdaddr %02x:%02x:%02x:%02x:%02x:%02x",
				app_xml_config.bd_addr[0], app_xml_config.bd_addr[1],
				app_xml_config.bd_addr[2], app_xml_config.bd_addr[3],
				app_xml_config.bd_addr[4], app_xml_config.bd_addr[5]);
		APP_DEBUG1("ClassOfDevice:%02x:%02x:%02x", app_xml_config.class_of_device[0],
				app_xml_config.class_of_device[1], app_xml_config.class_of_device[2]);
		APP_DEBUG1("RootPath:%s", app_xml_config.root_path);
	}
	return 0;
}

/*******************************************************************************
 **
 ** Function         app_mgr_read_remote_devices
 **
 ** Description      This function is used to read the XML bluetooth remote device file
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_mgr_read_remote_devices(void)
{
	int status;
	int index;

	for (index = 0 ; index < APP_NUM_ELEMENTS(app_xml_remote_devices_db) ; index++)
	{
		app_xml_remote_devices_db[index].in_use = FALSE;
	}

	status = app_xml_read_db(APP_XML_REM_DEVICES_FILE_PATH, app_xml_remote_devices_db,
			APP_NUM_ELEMENTS(app_xml_remote_devices_db));

	if (status < 0)
	{
		APP_ERROR1("app_xml_read_db failed:%d", status);
		return -1;
	}
	return 0;
}

int app_mgr_get_dev_av_bt_addr(BD_ADDR bd_addr)
{
    int index;

    APP_INFO1("%s",__func__);
    for (index = 0; index < APP_NUM_ELEMENTS(app_xml_remote_devices_db); index++)
    {
        if((app_xml_remote_devices_db[index].in_use != FALSE) &&
            (app_xml_remote_devices_db[index].trusted_services & BSA_A2DP_SERVICE_MASK))
        {
            bdcpy(bd_addr,app_xml_remote_devices_db[index].bd_addr);
            APP_INFO1("get a av dev name:%s,addr:%02x:%02x:%02x:%02x:%02x:%02x",app_xml_remote_devices_db[index].name,
                (unsigned char)bd_addr[0],(unsigned char)bd_addr[1],
                (unsigned char)bd_addr[2],(unsigned char)bd_addr[3],
                (unsigned char)bd_addr[4],(unsigned char)bd_addr[5]);
            return 0;
        }
    }
    return -1;

}

int app_mgr_get_bt_services(BD_ADDR bd_addr, unsigned char *bd_type)
{
    int index;
    for (index = 0; index < APP_NUM_ELEMENTS(app_xml_remote_devices_db); index++)
    {
        if((app_xml_remote_devices_db[index].in_use != FALSE) &&
            (bdcmp(bd_addr,app_xml_remote_devices_db[index].bd_addr)==0))
        {
            printf("service:%x,class:%02hhx\n",app_xml_remote_devices_db[index].trusted_services,
                app_xml_remote_devices_db[index].class_of_device[1]);
            if(app_xml_remote_devices_db[index].trusted_services & BSA_BLE_SERVICE_MASK)
                *bd_type = 1;
            else if(app_xml_remote_devices_db[index].trusted_services & BSA_SPP_SERVICE_MASK)
                *bd_type = 2;
            else if(app_xml_remote_devices_db[index].trusted_services & BSA_A2DP_SERVICE_MASK)
                *bd_type = 3;
            else if((app_xml_remote_devices_db[index].class_of_device[1] & 0x1F) == BTM_COD_MAJOR_AUDIO)
                *bd_type = 3;
            else
                *bd_type = 0;
            return 0;
        }
    }
    *bd_type = 0;
    return -1;
}

int app_mgr_get_bt_name(BD_ADDR bd_addr, BD_NAME *bd_name)
{
    int index;
    for (index = 0; index < APP_NUM_ELEMENTS(app_xml_remote_devices_db); index++)
    {
        if((app_xml_remote_devices_db[index].in_use != FALSE) &&
            (bdcmp(bd_addr,app_xml_remote_devices_db[index].bd_addr)==0))
        {
            strcpy(bd_name,app_xml_remote_devices_db[index].name);
            return 0;
        }
    }
    return -1;
}

/*******************************************************************************
 **
 ** Function         app_mgr_write_remote_devices
 **
 ** Description      This function is used to write the XML bluetooth remote device file
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_mgr_write_remote_devices(void)
{
	int status;

	status = app_xml_write_db(APP_XML_REM_DEVICES_FILE_PATH, app_xml_remote_devices_db,
			APP_NUM_ELEMENTS(app_xml_remote_devices_db));

	if (status < 0)
	{
		APP_ERROR1("app_xml_write_db failed:%d", status);
		return -1;
	}
	else
	{
		APP_DEBUG0("app_xml_write_db ok");
	}
	return 0;
}


/*******************************************************************************
 **
 ** Function         app_mgr_set_bt_config
 **
 ** Description      This function is used to get the bluetooth configuration
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_mgr_set_bt_config(BOOLEAN enable)
{
	int status;
	tBSA_DM_SET_CONFIG bt_config;

	/* Init config parameter */
	status = BSA_DmSetConfigInit(&bt_config);

	/*
	 * Set bluetooth configuration (from XML file)
	 */
	bt_config.enable = enable;
	bt_config.discoverable = app_xml_config.discoverable;
	bt_config.connectable = app_xml_config.connectable;
	bdcpy(bt_config.bd_addr, app_xml_config.bd_addr);
	strncpy((char *)bt_config.name, (char *)app_xml_config.name, sizeof(bt_config.name));
	strncpy((char *)bt_config.ble_name, (char *)app_xml_config.ble_name, sizeof(bt_config.ble_name));
	bt_config.name[sizeof(bt_config.name) - 1] = '\0';
	memcpy(bt_config.class_of_device, app_xml_config.class_of_device, sizeof(DEV_CLASS));
	bt_config.config_mask |= BSA_DM_CONFIG_NAME_MASK;
#ifdef USE_CHIP_BT_ADDR
	bt_config.config_mask &= ~(BSA_DM_CONFIG_BDADDR_MASK);
#endif
	APP_DEBUG1("Enable:%d", bt_config.enable);
	APP_DEBUG1("Discoverable:%d", bt_config.discoverable);
	APP_DEBUG1("Connectable:%d,mask:%08x", bt_config.connectable,bt_config.config_mask);
	APP_DEBUG1("Name:%s", bt_config.name);
	APP_DEBUG1("BLE Name:%s", bt_config.ble_name);
	APP_DEBUG1("Bdaddr %02x:%02x:%02x:%02x:%02x:%02x",
			bt_config.bd_addr[0], bt_config.bd_addr[1],
			bt_config.bd_addr[2], bt_config.bd_addr[3],
			bt_config.bd_addr[4], bt_config.bd_addr[5]);
	APP_DEBUG1("ClassOfDevice:%02x:%02x:%02x", bt_config.class_of_device[0],
			bt_config.class_of_device[1], bt_config.class_of_device[2]);
	APP_DEBUG1("First host disabled channel:%d", bt_config.first_disabled_channel);
	APP_DEBUG1("Last host disabled channel:%d", bt_config.last_disabled_channel);

	/* Apply BT config */
	status = BSA_DmSetConfig(&bt_config);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("BSA_DmSetConfig failed:%d", status);
		return(-1);
	}
#ifdef BLUETOOTH_HILINK_SUPPORT
        //setBleVisibility(0, 0);
#endif
	return 0;
}


/*******************************************************************************
 **
 ** Function         app_mgr_get_bt_config
 **
 ** Description      This function is used to get the bluetooth configuration
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_mgr_get_bt_config(void)
{
	int status;
	tBSA_DM_GET_CONFIG bt_config;

	/*
	 * Get bluetooth configuration
	 */
	status = BSA_DmGetConfigInit(&bt_config);
	status = BSA_DmGetConfig(&bt_config);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("BSA_DmGetConfig failed:%d", status);
		return(-1);
	}
	APP_DEBUG1("Enable:%d", bt_config.enable);
	APP_DEBUG1("Discoverable:%d", bt_config.discoverable);
	APP_DEBUG1("Connectable:%d", bt_config.connectable);
	APP_DEBUG1("Name:%s", bt_config.name);
	APP_DEBUG1("Name:%s", bt_config.ble_name);
	APP_DEBUG1("Bdaddr %02x:%02x:%02x:%02x:%02x:%02x",
			bt_config.bd_addr[0], bt_config.bd_addr[1],
			bt_config.bd_addr[2], bt_config.bd_addr[3],
			bt_config.bd_addr[4], bt_config.bd_addr[5]);
	APP_DEBUG1("ClassOfDevice:%02x:%02x:%02x", bt_config.class_of_device[0],
			bt_config.class_of_device[1], bt_config.class_of_device[2]);
	APP_DEBUG1("First host disabled channel:%d", bt_config.first_disabled_channel);
	APP_DEBUG1("Last host disabled channel:%d", bt_config.last_disabled_channel);

	return 0;

}

/*******************************************************************************
 **
 ** Function         app_mgr_read_oob
 **
 ** Description      This function is used to read local OOB data from local controller
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_mgr_read_oob_data()
{
	tBSA_SEC_READ_OOB bsa_sec_read_oob;
	if (BSA_SecReadOOBInit(&bsa_sec_read_oob) != BSA_SUCCESS)
	{
		APP_ERROR0("BSA_SecReadOOBInit failed");
		return;
	}

	if (BSA_SecReadOOB(&bsa_sec_read_oob) != BSA_SUCCESS)
	{
		APP_ERROR0("BSA_SecReadOOB failed");
		return;
	}
}

/*******************************************************************************
 **
 ** Function         app_mgr_local_oob_evt_data
 **
 ** Description      This function prints out local OOB data returned by the
 **                  local controller
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/

void app_mgr_local_oob_evt_data(tBSA_SEC_LOCAL_OOB_MSG* p_data)
{
	char szt[128];
	BT_OCTET16 p_c;
	BT_OCTET16 p_r;
	memcpy(p_c, p_data->c, 16);
	memcpy(p_r, p_data->r, 16);

	memset(szt,0, sizeof(szt));
	sprintf(szt,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			p_c[0],p_c[1],p_c[2],p_c[3],p_c[4],p_c[5],p_c[6],p_c[7],
			p_c[8],p_c[9],p_c[10],p_c[11],p_c[12],p_c[13],p_c[14],p_c[15]);
	APP_DEBUG1("app_mgr_local_oob_evt_data: C Hash: %s", szt);

	memset(szt,0, sizeof(szt));
	sprintf(szt,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			p_r[0],p_r[1],p_r[2],p_r[3],p_r[4],p_r[5],p_r[6],p_r[7],
			p_r[8],p_r[9],p_r[10],p_r[11],p_r[12],p_r[13],p_r[14],p_r[15]);
	APP_DEBUG1("app_mgr_local_oob_evt_data: R Randomizer: %s", szt);

	APP_DEBUG1("app_mgr_local_oob_evt_data: OOB Valid: %d", p_data->valid);
}


/*******************************************************************************
 **
 ** Function         app_mgr_set_remote_oob
 **
 ** Description      This function is used to set OOB data for peer device
 **                  During pairing stack uses this information to pair
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_mgr_set_remote_oob()
{
	int uarr[16];
	char szInput[64];
	int  i;

	if (BSA_SecSetRemoteOOBInit(&bsa_sec_set_remote_oob) != BSA_SUCCESS)
	{
		APP_ERROR0("app_mgr_set_remote_oob failed");
		return;
	}

	APP_INFO0("Enter BDA of Peer device: AABBCCDDEEFF");
	memset(szInput, 0, sizeof(szInput));
	app_get_string("Enter BDA: ", szInput, sizeof(szInput));
	sscanf (szInput, "%02x%02x%02x%02x%02x%02x",
			&uarr[0], &uarr[1], &uarr[2], &uarr[3], &uarr[4], &uarr[5]);

	for(i=0; i < 6; i++)
		bsa_sec_set_remote_oob.bd_addr[i] = (UINT8) uarr[i];

	bsa_sec_set_remote_oob.result = TRUE;   /* TRUE to accept; FALSE to reject */

	APP_INFO0("Enter 16 byte C Hash in this format: FA87C0D0AFAC11DE8A390800200C9A66");
	memset(szInput, 0, sizeof(szInput));
	app_get_string("C Hash: ", szInput, sizeof(szInput));
	sscanf (szInput, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			&uarr[0], &uarr[1], &uarr[2], &uarr[3], &uarr[4], &uarr[5], &uarr[6], &uarr[7], &uarr[8],
			&uarr[9], &uarr[10], &uarr[11], &uarr[12], &uarr[13], &uarr[14], &uarr[15]);

	for(i=0; i < 16; i++)
		bsa_sec_set_remote_oob.p_c[i] = (UINT8) uarr[i];

	APP_INFO0("Enter 16 byte R Randomizer in this format: FA87C0D0AFAC11DE8A390800200C9A66");
	memset(szInput, 0, sizeof(szInput));
	app_get_string("R Randomizer: ", szInput, sizeof(szInput));
	sscanf (szInput, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			&uarr[0], &uarr[1], &uarr[2], &uarr[3], &uarr[4], &uarr[5], &uarr[6], &uarr[7], &uarr[8],
			&uarr[9], &uarr[10], &uarr[11], &uarr[12], &uarr[13], &uarr[14], &uarr[15]);

	for(i=0; i < 16; i++)
		bsa_sec_set_remote_oob.p_r[i] = (UINT8) uarr[i];

	if (BSA_SecSetRemoteOOB(&bsa_sec_set_remote_oob) != BSA_SUCCESS)
	{
		APP_ERROR0("BSA_SecSetRemoteOOB failed");
		return;
	}
}

static tBSA_SEC_PIN_CODE_REPLY g_pin_code_reply;
#include "BluetoothUtils.h"
/*******************************************************************************
 **
 ** Function         app_mgr_security_callback
 **
 ** Description      Security callback
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_SEC_LINK_UP bklinkup_dev;
void app_mgr_security_callback(tBSA_SEC_EVT event, tBSA_SEC_MSG *p_data)
{
	int status;
	int indexDisc;
	tBSA_SEC_PIN_CODE_REPLY pin_code_reply;
	tBSA_SEC_AUTH_REPLY autorize_reply;

	APP_DEBUG1("event:%d", event);

	switch(event)
	{
		case BSA_SEC_LINK_UP_EVT:       /* A device is physically connected (for info) */
            memcpy(&bklinkup_dev, &p_data->link_up, sizeof(tBSA_SEC_LINK_UP));
			APP_DEBUG1("BSA_SEC_LINK_UP_EVT bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
					p_data->link_up.bd_addr[0], p_data->link_up.bd_addr[1],
					p_data->link_up.bd_addr[2], p_data->link_up.bd_addr[3],
					p_data->link_up.bd_addr[4], p_data->link_up.bd_addr[5]);
			APP_DEBUG1("ClassOfDevice:%02x:%02x:%02x => %s",
					p_data->link_up.class_of_device[0],
					p_data->link_up.class_of_device[1],
					p_data->link_up.class_of_device[2],
					app_get_cod_string(p_data->link_up.class_of_device));
#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
			APP_DEBUG1("LinkType:%s", app_link_transport_type_desc(p_data->link_up.link_type));
			APP_DEBUG1("BLE_addr_type:%d", p_data->link_up.ble_addr_type);
#endif
            for (indexDisc = 0; indexDisc < APP_MAX_NB_REMOTE_STORED_DEVICES; indexDisc++)
            {
                if(linking_dev[indexDisc].in_use == FALSE)
                {
                    memset(&linking_dev[indexDisc], 0, sizeof(tLINKING_DEVICE));
                    linking_dev[indexDisc].in_use = TRUE;
                    memcpy(linking_dev[indexDisc].bd_addr, p_data->link_up.bd_addr, sizeof(BD_ADDR));
                    if((p_data->link_up.class_of_device[1] & 0x1f) == BTM_COD_MAJOR_AUDIO) {
                        linking_dev[indexDisc].bd_type = BD_TYPE_A2DP;
                    }/*else if(p_data->link_up.BLE_addr_type) {
                        linking_dev[indexDisc].bd_type =BD_TYPE_BLE;
                    }*/else
                        app_mgr_get_bt_services(linking_dev[indexDisc].bd_addr,&linking_dev[indexDisc].bd_type);
			        APP_DEBUG1("get type:%d", linking_dev[indexDisc].bd_type);
                                //BTM_COD_MAJOR_CLASS(linking_dev[indexDisc].bd_type, p_data->link_up.class_of_device);
                    app_mgr_get_bt_name(linking_dev[indexDisc].bd_addr,linking_dev[indexDisc].bd_name);
                    break;
                }
            }
#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
#ifdef BLUETOOTH_HILINK_SUPPORT //def BLE_BOND_CONN  //cljiang add
            if (!app_xml_update_ble_addr_type_db(app_xml_remote_devices_db,
                APP_NUM_ELEMENTS(app_xml_remote_devices_db), p_data->link_up.bd_addr, p_data->link_up.ble_addr_type))
                app_write_xml_remote_devices();
#if 0
            if(p_data->link_up.ble_addr_type) {
                if((p_data->link_up.class_of_device[1] & 0x1f) != BTM_COD_MAJOR_AUDIO) {
                    APP_DEBUG0("send secBondInit");
                    tBSA_SEC_BOND sec_bond;
                    BSA_SecBondInit(&sec_bond);
                    bdcpy(sec_bond.bd_addr, p_data->link_up.bd_addr);
                    status = BSA_SecBond(&sec_bond);
                    if (status != BSA_SUCCESS)
                    {
                        APP_ERROR1("BSA_SecBond failed:%d", status);
                    }
                }
            }
#endif
#endif//cljiang add
#endif
        break;
		case BSA_SEC_LINK_DOWN_EVT:     /* A device is physically disconnected (for info)*/
			APP_DEBUG1("BSA_SEC_LINK_DOWN_EVT bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
					p_data->link_down.bd_addr[0], p_data->link_down.bd_addr[1],
					p_data->link_down.bd_addr[2], p_data->link_down.bd_addr[3],
					p_data->link_down.bd_addr[4], p_data->link_down.bd_addr[5]);
			APP_DEBUG1("Reason: 0x%x (%d)", p_data->link_down.status, p_data->link_down.status);
#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
			APP_DEBUG1("LinkType:%s,%d:%d", app_link_transport_type_desc(p_data->link_down.link_type), p_data->link_down.link_type);
            if (p_data->sig_strength.link_type == BT_TRANSPORT_LE){
                //call cb here
            }
#endif
#if 1
            for (indexDisc = 0; indexDisc < APP_MAX_NB_REMOTE_STORED_DEVICES; indexDisc++)
            {
                if(linking_dev[indexDisc].in_use == TRUE )
                {
                    if(memcmp(p_data->link_down.bd_addr, linking_dev[indexDisc].bd_addr, sizeof(BD_ADDR)) == 0)
                    {
                        linking_dev[indexDisc].in_use = FALSE;
                        if(linking_dev[indexDisc].bd_type = 3){//A2DP
                            app_av_linkdn_notify();
                        }
                        break;
                    }
                }
            }
#endif
			break;

		case BSA_SEC_PIN_REQ_EVT:
			APP_DEBUG1("BSA_SEC_PIN_REQ_EVT (Pin Code Request) received from: %02x:%02x:%02x:%02x:%02x:%02x",
					p_data->pin_req.bd_addr[0], p_data->pin_req.bd_addr[1],
					p_data->pin_req.bd_addr[2], p_data->pin_req.bd_addr[3],
					p_data->pin_req.bd_addr[4],p_data->pin_req.bd_addr[5]);

			APP_DEBUG1("call BSA_SecPinCodeReply pin:%s len:%d",
					app_xml_config.pin_code, app_xml_config.pin_len);

			BSA_SecPinCodeReplyInit(&pin_code_reply);
			bdcpy(pin_code_reply.bd_addr, p_data->pin_req.bd_addr);
			pin_code_reply.pin_len = app_xml_config.pin_len;
			strncpy((char *)pin_code_reply.pin_code, (char *)app_xml_config.pin_code,
					app_xml_config.pin_len);
			/* note that this code will not work if pin_len = 16 */
			pin_code_reply.pin_code[PIN_CODE_LEN-1] = '\0';
			status = BSA_SecPinCodeReply(&pin_code_reply);
			break;

		case BSA_SEC_AUTH_CMPL_EVT:
			APP_DEBUG1("BSA_SEC_AUTH_CMPL_EVT (name=%s,success=%d)",
					p_data->auth_cmpl.bd_name, p_data->auth_cmpl.success);
			if (!p_data->auth_cmpl.success)
			{
				APP_DEBUG1("    fail_reason=%d", p_data->auth_cmpl.fail_reason);
			}
			APP_DEBUG1("    bd_addr:%02x:%02x:%02x:%02x:%02x:%02x",
					p_data->auth_cmpl.bd_addr[0], p_data->auth_cmpl.bd_addr[1], p_data->auth_cmpl.bd_addr[2],
					p_data->auth_cmpl.bd_addr[3], p_data->auth_cmpl.bd_addr[4], p_data->auth_cmpl.bd_addr[5]);
			if (p_data->auth_cmpl.key_present != FALSE)
			{
				APP_DEBUG1("    LinkKey:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
						p_data->auth_cmpl.key[0], p_data->auth_cmpl.key[1], p_data->auth_cmpl.key[2], p_data->auth_cmpl.key[3],
						p_data->auth_cmpl.key[4], p_data->auth_cmpl.key[5], p_data->auth_cmpl.key[6], p_data->auth_cmpl.key[7],
						p_data->auth_cmpl.key[8], p_data->auth_cmpl.key[9], p_data->auth_cmpl.key[10], p_data->auth_cmpl.key[11],
						p_data->auth_cmpl.key[12], p_data->auth_cmpl.key[13], p_data->auth_cmpl.key[14], p_data->auth_cmpl.key[15]);
			}

			/* If success */
			if (p_data->auth_cmpl.success != 0)
			{
				/* Read the Remote device xml file to have a fresh view */
				app_mgr_read_remote_devices();

				app_xml_update_name_db(app_xml_remote_devices_db,
						APP_NUM_ELEMENTS(app_xml_remote_devices_db),
						p_data->auth_cmpl.bd_addr,
						p_data->auth_cmpl.bd_name);

				if (p_data->auth_cmpl.key_present != FALSE)
				{
					app_xml_update_key_db(app_xml_remote_devices_db,
							APP_NUM_ELEMENTS(app_xml_remote_devices_db),
							p_data->auth_cmpl.bd_addr,
							p_data->auth_cmpl.key,
							p_data->auth_cmpl.key_type);
				}

				/* Unfortunately, the BSA_SEC_AUTH_CMPL_EVT does not contain COD, let's look in the Discovery database */
				for (indexDisc = 0 ; indexDisc < APP_DISC_NB_DEVICES ; indexDisc++)
				{
					if ((app_discovery_cb.devs[indexDisc].in_use != FALSE) &&
							(bdcmp(app_discovery_cb.devs[indexDisc].device.bd_addr, p_data->auth_cmpl.bd_addr) == 0))

					{
						app_xml_update_cod_db(app_xml_remote_devices_db,
								APP_NUM_ELEMENTS(app_xml_remote_devices_db),
								p_data->auth_cmpl.bd_addr,
								app_discovery_cb.devs[indexDisc].device.class_of_device);
					}
				}
                //--add by clivia
                app_xml_update_cod_db(app_xml_remote_devices_db,
								APP_NUM_ELEMENTS(app_xml_remote_devices_db),
                                bklinkup_dev.bd_addr,
                                bklinkup_dev.class_of_device);
				status = app_mgr_write_remote_devices();
				if (status < 0)
				{
					APP_ERROR1("app_mgr_write_remote_devices failed:%d", status);
				}

#ifdef BSA_PEER_IOS
				/* Start device info discovery */
				app_disc_start_dev_info(p_data->auth_cmpl.bd_addr, NULL);
#endif
			}
			break;

		case BSA_SEC_BOND_CANCEL_CMPL_EVT:
			APP_DEBUG1("BSA_SEC_BOND_CANCEL_CMPL_EVT status=%d",
					p_data->bond_cancel.status);
			break;

		case BSA_SEC_AUTHORIZE_EVT:  /* Authorization request */
			APP_DEBUG0("BSA_SEC_AUTHORIZE_EVT");
			APP_DEBUG1("\tRemote device:%s", p_data->authorize.bd_name);
			APP_DEBUG1("\tbd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
					p_data->authorize.bd_addr[0], p_data->authorize.bd_addr[1],
					p_data->authorize.bd_addr[2], p_data->authorize.bd_addr[3],
					p_data->authorize.bd_addr[4],p_data->authorize.bd_addr[5]);
			APP_DEBUG1("\tRequest access to service:%x (%s)",
					(int)p_data->authorize.service,
					app_service_id_to_string(p_data->authorize.service));
			APP_DEBUG0("\tAccess Granted (permanently)");
			status = BSA_SecAuthorizeReplyInit(&autorize_reply);
			bdcpy(autorize_reply.bd_addr, p_data->authorize.bd_addr);
			autorize_reply.trusted_service = p_data->authorize.service;
			autorize_reply.auth = BSA_SEC_AUTH_PERM;
			status = BSA_SecAuthorizeReply(&autorize_reply);
			/*
			 * Update XML database
			 */
			/* Read the Remote device xml file to have a fresh view */
			app_mgr_read_remote_devices();
			/* Add AV service for this devices in XML database */
			app_xml_add_trusted_services_db(app_xml_remote_devices_db,
					APP_NUM_ELEMENTS(app_xml_remote_devices_db), p_data->authorize.bd_addr,
					1 << p_data->authorize.service);

			if (strlen((char *)p_data->authorize.bd_name) > 0)
				app_xml_update_name_db(app_xml_remote_devices_db,
						APP_NUM_ELEMENTS(app_xml_remote_devices_db),
						p_data->authorize.bd_addr, p_data->authorize.bd_name);

			/* Update database => write on disk */
			status = app_mgr_write_remote_devices();
			if (status < 0)
			{
				APP_ERROR1("app_mgr_write_remote_devices failed:%d", status);
			}
			break;

		case BSA_SEC_SP_CFM_REQ_EVT: /* Simple Pairing confirm request */
			APP_DEBUG0("BSA_SEC_SP_CFM_REQ_EVT");
			APP_DEBUG1("\tRemote device:%s", p_data->cfm_req.bd_name);
			APP_DEBUG1("\tbd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
					p_data->cfm_req.bd_addr[0], p_data->cfm_req.bd_addr[1],
					p_data->cfm_req.bd_addr[2], p_data->cfm_req.bd_addr[3],
					p_data->cfm_req.bd_addr[4], p_data->cfm_req.bd_addr[5]);
			APP_DEBUG1("\tClassOfDevice:%02x:%02x:%02x => %s",
					p_data->cfm_req.class_of_device[0], p_data->cfm_req.class_of_device[1], p_data->cfm_req.class_of_device[2],
					app_get_cod_string(p_data->cfm_req.class_of_device));
			APP_DEBUG1("\tJust Work:%s", p_data->cfm_req.just_works == TRUE ? "TRUE" : "FALSE");
			APP_DEBUG1("\tNumeric Value:%d", p_data->cfm_req.num_val);
			bdcpy(app_sec_db_addr, p_data->cfm_req.bd_addr);
#if defined(APP_SEC_SP_AUTO_ACCEPT) && ( APP_SEC_SP_AUTO_ACCEPT == TRUE)
			APP_DEBUG0("\tSimple Pairing automatically Accepted");
			app_mgr_sp_cfm_reply(TRUE, p_data->cfm_req.bd_addr);
#else
			APP_DEBUG0("\tYou must accept or refuse using menu!!");
#endif
			break;

		case BSA_SEC_SP_KEY_NOTIF_EVT: /* Simple Pairing Passkey Notification */
			APP_DEBUG0("BSA_SEC_SP_KEY_NOTIF_EVT");
			APP_DEBUG1("\tbd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
					p_data->key_notif.bd_addr[0], p_data->key_notif.bd_addr[1],
					p_data->key_notif.bd_addr[2], p_data->key_notif.bd_addr[3],
					p_data->key_notif.bd_addr[4], p_data->key_notif.bd_addr[5]);
			APP_DEBUG1("\tClassOfDevice:%02x:%02x:%02x => %s",
					p_data->key_notif.class_of_device[0],
					p_data->key_notif.class_of_device[1],
					p_data->key_notif.class_of_device[2],
					app_get_cod_string(p_data->key_notif.class_of_device));
			APP_DEBUG1("\tNumeric Value:%d", p_data->key_notif.passkey);
#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
			APP_DEBUG1("\tLinkType:%s", app_link_transport_type_desc(p_data->key_notif.link_type));
#endif
			APP_DEBUG0("\tYou must enter this value on peer device's keyboard");
			break;

		case BSA_SEC_SP_KEY_REQ_EVT: /* Simple Pairing Passkey request Notification */
			APP_DEBUG0("BSA_SEC_SP_KEY_REQ_EVT");
			APP_DEBUG1("\tbd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
					p_data->key_notif.bd_addr[0], p_data->key_notif.bd_addr[1],
					p_data->key_notif.bd_addr[2], p_data->key_notif.bd_addr[3],
					p_data->key_notif.bd_addr[4], p_data->key_notif.bd_addr[5]);
			APP_DEBUG1("\tClassOfDevice:%02x:%02x:%02x => %s",
					p_data->key_notif.class_of_device[0], p_data->key_notif.class_of_device[1],
					p_data->key_notif.class_of_device[2],
					app_get_cod_string(p_data->key_notif.class_of_device));
			APP_DEBUG1("\tNumeric Value:%d", p_data->key_notif.passkey);
#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
			APP_DEBUG1("\tLinkType: %d", p_data->key_notif.link_type);
#endif
			APP_DEBUG0("\tYou must enter this value on peer device's keyboard");

			memset(&g_passkey_reply, 0, sizeof(g_passkey_reply));
			bdcpy(g_passkey_reply.bd_addr, p_data->pin_req.bd_addr);
			break;

		case BSA_SEC_SP_KEYPRESS_EVT: /* Simple Pairing Key press notification event. */
			APP_DEBUG1("BSA_SEC_SP_KEYPRESS_EVT (type:%d)", p_data->key_press.notif_type);
			APP_DEBUG1("\tbd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
					p_data->key_press.bd_addr[0], p_data->key_press.bd_addr[1],
					p_data->key_press.bd_addr[2], p_data->key_press.bd_addr[3],
					p_data->key_press.bd_addr[4], p_data->key_press.bd_addr[5]);
			break;

		case BSA_SEC_SP_RMT_OOB_EVT: /* Simple Pairing Remote OOB Data request. */
			APP_DEBUG0("BSA_SEC_SP_RMT_OOB_EVT received - Handled internally if Peer OOB already set");
			break;

		case BSA_SEC_LOCAL_OOB_DATA_EVT: /* Local OOB Data response */
			APP_DEBUG0("BSA_SEC_LOCAL_OOB_DATA_EVT received");
			app_mgr_local_oob_evt_data(&p_data->local_oob);
			break;

		case BSA_SEC_SUSPENDED_EVT: /* Connection Suspended */
			APP_INFO1("BSA_SEC_SUSPENDED_EVT bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
					p_data->suspended.bd_addr[0], p_data->suspended.bd_addr[1],
					p_data->suspended.bd_addr[2], p_data->suspended.bd_addr[3],
					p_data->suspended.bd_addr[4], p_data->suspended.bd_addr[5]);
			break;

		case BSA_SEC_RESUMED_EVT: /* Connection Resumed */
			APP_INFO1("BSA_SEC_RESUMED_EVT bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
					p_data->resumed.bd_addr[0], p_data->resumed.bd_addr[1],
					p_data->resumed.bd_addr[2], p_data->resumed.bd_addr[3],
					p_data->resumed.bd_addr[4], p_data->resumed.bd_addr[5]);
			break;

#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
		case BSA_SEC_BLE_KEY_EVT: /* BLE KEY event */
			APP_INFO0("BSA_SEC_BLE_KEY_EVT");
			switch (p_data->ble_key.key_type)
			{
				case BSA_LE_KEY_PENC:
					APP_DEBUG1("\t key_type: BTM_LE_KEY_PENC(%d)", p_data->ble_key.key_type);
					APP_DUMP("LTK", p_data->ble_key.key_value.penc_key.ltk, 16);
					APP_DUMP("RAND", p_data->ble_key.key_value.penc_key.rand, 8);
					APP_DEBUG1("ediv: 0x%x:", p_data->ble_key.key_value.penc_key.ediv);
					APP_DEBUG1("sec_level: 0x%x:", p_data->ble_key.key_value.penc_key.sec_level);
					APP_DEBUG1("key_size: %d:", p_data->ble_key.key_value.penc_key.key_size);
					break;
				case BSA_LE_KEY_PID:
					APP_DEBUG1("\t key_type: BTM_LE_KEY_PID(%d)", p_data->ble_key.key_type);
					APP_DUMP("IRK", p_data->ble_key.key_value.pid_key.irk, 16);
					APP_DEBUG1("addr_type: 0x%x:", p_data->ble_key.key_value.pid_key.addr_type);
					APP_DEBUG1("static_addr: %02X-%02X-%02X-%02X-%02X-%02X",
							p_data->ble_key.key_value.pid_key.static_addr[0],
							p_data->ble_key.key_value.pid_key.static_addr[1],
							p_data->ble_key.key_value.pid_key.static_addr[2],
							p_data->ble_key.key_value.pid_key.static_addr[3],
							p_data->ble_key.key_value.pid_key.static_addr[4],
							p_data->ble_key.key_value.pid_key.static_addr[5]);
					break;
				case BSA_LE_KEY_PCSRK:
					APP_DEBUG1("\t key_type: BTM_LE_KEY_PCSRK(%d)", p_data->ble_key.key_type);
					APP_DEBUG1("counter: 0x%x:", p_data->ble_key.key_value.pcsrk_key.counter);
					APP_DUMP("CSRK", p_data->ble_key.key_value.pcsrk_key.csrk, 16);
					APP_DEBUG1("sec_level: %d:", p_data->ble_key.key_value.pcsrk_key.sec_level);
					break;
				case BSA_LE_KEY_LCSRK:
					APP_DEBUG1("\t key_type: BTM_LE_KEY_LCSRK(%d)", p_data->ble_key.key_type);
					APP_DEBUG1("counter: 0x%x:", p_data->ble_key.key_value.lcsrk_key.counter);
					APP_DEBUG1("div: %d:", p_data->ble_key.key_value.lcsrk_key.div);
					APP_DEBUG1("sec_level: 0x%x:", p_data->ble_key.key_value.lcsrk_key.sec_level);
					break;
				case BSA_LE_KEY_LENC:
					APP_DEBUG1("\t key_type: BTM_LE_KEY_LENC(%d)", p_data->ble_key.key_type);
					APP_DEBUG1("div: 0x%x:", p_data->ble_key.key_value.lenc_key.div);
					APP_DEBUG1("key_size: %d:", p_data->ble_key.key_value.lenc_key.key_size);
					APP_DEBUG1("sec_level: 0x%x:", p_data->ble_key.key_value.lenc_key.sec_level);
					break;
				case BSA_LE_KEY_LID:
					APP_DEBUG1("\t key_type: BTM_LE_KEY_LID(%d)", p_data->ble_key.key_type);
					break;
				default:
					APP_DEBUG1("\t key_type: Unknown key(%d)", p_data->ble_key.key_type);
					break;
			}

			/* Read the Remote device xml file to have a fresh view */
			app_mgr_read_remote_devices();

			app_xml_update_device_type_db(app_xml_remote_devices_db,
					APP_NUM_ELEMENTS(app_xml_remote_devices_db), p_data->ble_key.bd_addr,
					BT_DEVICE_TYPE_BLE);

			app_xml_update_ble_key_db(app_xml_remote_devices_db,
					APP_NUM_ELEMENTS(app_xml_remote_devices_db),
					p_data->ble_key.bd_addr,
					p_data->ble_key.key_value,
					p_data->ble_key.key_type);

			status = app_mgr_write_remote_devices();
			if (status < 0)
			{
				APP_ERROR1("app_mgr_write_remote_devices failed:%d", status);
			}
			break;

		case BSA_SEC_BLE_PASSKEY_REQ_EVT:
			APP_INFO0("BSA_SEC_BLE_PASSKEY_REQ_EVT (Passkey Request) received from:");
			APP_INFO1("\tbd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
					p_data->ble_passkey_req.bd_addr[0], p_data->ble_passkey_req.bd_addr[1],
					p_data->ble_passkey_req.bd_addr[2], p_data->ble_passkey_req.bd_addr[3],
					p_data->ble_passkey_req.bd_addr[4], p_data->ble_passkey_req.bd_addr[5]);

			memset(&g_pin_code_reply, 0, sizeof(g_pin_code_reply));
			bdcpy(g_pin_code_reply.bd_addr, p_data->pin_req.bd_addr);
			break;
#endif

		case BSA_SEC_RSSI_EVT:
			APP_INFO1("BSA_SEC_RSSI_EVT received for BdAddr: %02x:%02x:%02x:%02x:%02x:%02x",
					p_data->sig_strength.bd_addr[0], p_data->sig_strength.bd_addr[1],
					p_data->sig_strength.bd_addr[2], p_data->sig_strength.bd_addr[3],
					p_data->sig_strength.bd_addr[4], p_data->sig_strength.bd_addr[5]);
			if (p_data->sig_strength.mask & BSA_SEC_SIG_STRENGTH_RSSI)
			{
				if (p_data->sig_strength.link_type == BSA_TRANSPORT_BR_EDR)
				{
					/* For BasicRate & EnhancedDataRate links, the RSSI is an attenuation (dB) */
					APP_INFO1("\tRSSI: %d (dB)", p_data->sig_strength.rssi_value);
				}
				else
				{
					/* For Low Energy links, the RSSI is a strength (dBm) */
					APP_INFO1("\tRSSI: %d (dBm)", p_data->sig_strength.rssi_value);
				}
			}

			if (p_data->sig_strength.mask & BSA_SEC_SIG_STRENGTH_RAW_RSSI)
			{
				/* Raw RSSI is always a strength (dBm) */
				APP_INFO1("\tRaw RSSI: %d (dBm)", p_data->sig_strength.raw_rssi_value);
			}

			if (p_data->sig_strength.mask & BSA_SEC_SIG_STRENGTH_LINK_QUALITY)
			{
				APP_INFO1("\tLink Quality: %d", p_data->sig_strength.link_quality_value);
			}
			break;

		default:
			APP_ERROR1("unknown event:%d", event);
			break;
	}
}

/*******************************************************************************
 **
 ** Function         app_mgr_send_pincode
 **
 ** Description      Sends simple pairing passkey to server
 **
 ** Parameters       passkey
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_mgr_send_pincode(tBSA_SEC_PIN_CODE_REPLY pin_code_reply)
{
    int status;
    bdcpy(pin_code_reply.bd_addr, g_pin_code_reply.bd_addr);
    status = BSA_SecPinCodeReply(&pin_code_reply);
    if (status)
    {
        APP_ERROR1("BSA_SecPinCodeReply ERROR:%d", status);
    }else
        APP_ERROR1("BSA_SecPinCodeReply bd:%02x:%02x:%02x:%02x:%02x:%02x,pin_code:%x\n",
        (unsigned char)g_pin_code_reply.bd_addr[0],(unsigned char)g_pin_code_reply.bd_addr[1],
        (unsigned char)g_pin_code_reply.bd_addr[2],(unsigned char)g_pin_code_reply.bd_addr[3],
        (unsigned char)g_pin_code_reply.bd_addr[4],(unsigned char)g_pin_code_reply.bd_addr[5],
        g_pin_code_reply.pin_code);
}


/*******************************************************************************
 **
 ** Function         app_mgr_send_passkey
 **
 ** Description      Sends simple pairing passkey to server
 **
 ** Parameters       passkey
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_mgr_send_passkey(UINT32 passkey)
{
	g_passkey_reply.passkey = passkey;
	BSA_SecSpPasskeyReply(&g_passkey_reply);
}

/*******************************************************************************
 **
 ** Function         app_mgr_sec_set_sp_cap
 **
 ** Description      Set simple pairing capabilities
 **
 ** Parameters       sp_cap: simple pairing capabilities
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_mgr_sec_set_sp_cap(tBSA_SEC_IO_CAP sp_cap)
{
	int status;
	tBSA_SEC_SET_SECURITY set_security;

	BSA_SecSetSecurityInit(&set_security);
	set_security.simple_pairing_io_cap = sp_cap;
	set_security.sec_cback = app_mgr_security_callback;
	status = BSA_SecSetSecurity(&set_security);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("BSA_SecSetSecurity failed:%d", status);
		return -1;
	}
	return 0;
}

/*******************************************************************************
 **
 ** Function         app_mgr_sec_set_security
 **
 ** Description      Security configuration
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_mgr_sec_set_security(void)
{
	int status;
	tBSA_SEC_SET_SECURITY set_security;

	APP_DEBUG0("");

	BSA_SecSetSecurityInit(&set_security);
	set_security.simple_pairing_io_cap = app_xml_config.io_cap;
	set_security.sec_cback = app_mgr_security_callback;
	status = BSA_SecSetSecurity(&set_security);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("BSA_SecSetSecurity failed:%d", status);
		return -1;
	}
	return 0;
}


/*******************************************************************************
 **
 ** Function         app_mgr_sp_cfm_reply
 **
 ** Description      Function used to accept/refuse Simple Pairing
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_mgr_sp_cfm_reply(BOOLEAN accept, BD_ADDR bd_addr)
{
	int status;
	tBSA_SEC_SP_CFM_REPLY app_sec_sp_cfm_reply;

	APP_DEBUG1("accept:%d bd_addr: %02x:%02x:%02x:%02x:%02x:%02x", accept,
			bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);

	BSA_SecSpCfmReplyInit(&app_sec_sp_cfm_reply);
	app_sec_sp_cfm_reply.accept = accept;
	bdcpy(app_sec_sp_cfm_reply.bd_addr, bd_addr);
	status = BSA_SecSpCfmReply(&app_sec_sp_cfm_reply);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("BSA_SecSpCfmReply failed:%d", status);
		return -1;
	}
	return 0;
}



/*******************************************************************************
 **
 ** Function         app_mgr_sec_bond_cancel
 **
 ** Description      Cancel a bonding procedure
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_mgr_sec_bond_cancel(void)
{
	int status;
	int device_index;
	tBSA_SEC_BOND_CANCEL sec_bond_cancel;

	APP_INFO0("Bluetooth Bond Cancel menu:");
	app_disc_display_devices();
	device_index = app_get_choice("Select device");

	if ((device_index >= 0) &&
			(device_index < APP_DISC_NB_DEVICES) &&
			(app_discovery_cb.devs[device_index].in_use != FALSE))
	{
		BSA_SecBondCancelInit(&sec_bond_cancel);
		bdcpy(sec_bond_cancel.bd_addr, app_discovery_cb.devs[device_index].device.bd_addr);
		status = BSA_SecBondCancel(&sec_bond_cancel);
		if (status != BSA_SUCCESS)
		{
			APP_ERROR1("BSA_SecBondCancel failed:%d", status);
			return -1;
		}
	}
	else
	{
		APP_ERROR1("Bad device number:%d", device_index);
		return -1;
	}

	return 0;
}



/*******************************************************************************
 **
 ** Function         app_mgr_set_discoverable
 **
 ** Description      Set the device discoverable for a specific time
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_mgr_set_discoverable(void)
{
	tBSA_DM_SET_CONFIG bsa_dm_set_config;
	if (BSA_DmSetConfigInit(&bsa_dm_set_config) != BSA_SUCCESS)
	{
		APP_ERROR0("BSA_DmSetConfigInit failed");
		return;
	}
	bsa_dm_set_config.discoverable = 1;
	bsa_dm_set_config.config_mask = BSA_DM_CONFIG_VISIBILITY_MASK;
	if (BSA_DmSetConfig(&bsa_dm_set_config) != BSA_SUCCESS)
	{
		APP_ERROR0("BSA_DmSetConfig failed");
		return;
	}
}

/*******************************************************************************
 **
 ** Function         app_mgr_set_non_discoverable
 **
 ** Description      Set the device non discoverable
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_mgr_set_non_discoverable(void)
{
	tBSA_DM_SET_CONFIG bsa_dm_set_config;
	if (BSA_DmSetConfigInit(&bsa_dm_set_config) != BSA_SUCCESS)
	{
		APP_ERROR0("BSA_DmSetConfigInit failed");
		return;
	}
	bsa_dm_set_config.discoverable = 0;
	bsa_dm_set_config.config_mask = BSA_DM_CONFIG_VISIBILITY_MASK;
	if (BSA_DmSetConfig(&bsa_dm_set_config) != BSA_SUCCESS)
	{
		APP_ERROR0("BSA_DmSetConfig failed");
		return;
	}
}

/*******************************************************************************
 **
 ** Function         app_mgr_set_connectable
 **
 ** Description      Set the device connectable for a specific time
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_mgr_set_connectable(void)
{
	tBSA_DM_SET_CONFIG bsa_dm_set_config;
	if (BSA_DmSetConfigInit(&bsa_dm_set_config) != BSA_SUCCESS)
	{
		APP_ERROR0("BSA_DmSetConfigInit failed");
		return;
	}
	bsa_dm_set_config.connectable = 1;
	bsa_dm_set_config.config_mask = BSA_DM_CONFIG_VISIBILITY_MASK;
	if (BSA_DmSetConfig(&bsa_dm_set_config) != BSA_SUCCESS)
	{
		APP_ERROR0("BSA_DmSetConfig failed");
		return;
	}
}

/*******************************************************************************
 **
 ** Function         app_mgr_set_non_connectable
 **
 ** Description      Set the device non connectable
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_mgr_set_non_connectable(void)
{
	tBSA_DM_SET_CONFIG bsa_dm_set_config;
	if (BSA_DmSetConfigInit(&bsa_dm_set_config) != BSA_SUCCESS)
	{
		APP_ERROR0("BSA_DmSetConfigInit failed");
		return;
	}
	bsa_dm_set_config.connectable = 0;
	bsa_dm_set_config.config_mask = BSA_DM_CONFIG_VISIBILITY_MASK;
	if (BSA_DmSetConfig(&bsa_dm_set_config) != BSA_SUCCESS)
	{
		APP_ERROR0("BSA_DmSetConfig failed");
		return;
	}
}

/*******************************************************************************
 **
 ** Function         app_mgr_di_discovery
 **
 ** Description      Perform a device Id discovery
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_mgr_di_discovery(void)
{
	int index, status;
	BD_ADDR bd_addr;

	do
	{
		APP_INFO0("Bluetooth Manager DI Discovery menu:");
		APP_INFO0("    0 Device from XML database (previously paired)");
		APP_INFO0("    1 Device found in last discovery");
		index = app_get_choice("Select source");
		if (index == 0)
		{
			/* Read the Remote device xml file to have a fresh view */
			app_mgr_read_remote_devices();

			app_xml_display_devices(app_xml_remote_devices_db, APP_NUM_ELEMENTS(app_xml_remote_devices_db));
			index = app_get_choice("Select device");
			if ((index >= 0) &&
					(index < APP_NUM_ELEMENTS(app_xml_remote_devices_db)) &&
					(app_xml_remote_devices_db[index].in_use != FALSE))
			{
				bdcpy(bd_addr, app_xml_remote_devices_db[index].bd_addr);
			}
			else
			{
				break;
			}
		}
		/* Devices from Discovery */
		else if (index == 1)
		{
			app_disc_display_devices();
			index = app_get_choice("Select device");
			if ((index >= 0) &&
					(index < APP_DISC_NB_DEVICES) &&
					(app_discovery_cb.devs[index].in_use != FALSE))
			{
				bdcpy(bd_addr, app_discovery_cb.devs[index].device.bd_addr);
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}

		/* Start Discovery Device Info */
		status = app_disc_start_dev_info(bd_addr, NULL);
		return status;
	} while (0);

	APP_ERROR1("Bad Device index:%d", index);
	return -1;
}

#define ID_FILE "/sys/class/net/wlan0/address"
//#define ID_FILE "/sys/class/net/wlan0/baddr"

static INT32 get_mac(char *fname, UINT8 * id_buf)
{
    UINT32 fd;
    INT32 ret=6;
	UINT8 buf[64];
    UINT8 tbuf[20];
    UINT8 buf2[64];

    fd = open(fname, O_RDONLY);
    if (fd < 0) {
        APP_ERROR1(" Fail to open device %s\n", ID_FILE);
        return 0;
    }
	buf[0] = 0;
	read(fd, buf, 63);
		printf("fname:%s:get_mac:buf:%s\n",fname,buf);
	close(fd);
	if(buf[0]) {
        UINT8 *ptr;
        ptr = strchr(buf,'\n');
        if(ptr){
            *ptr = 0;
        }
		sscanf(buf,"%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",tbuf,tbuf+1,tbuf+2,tbuf+3,tbuf+4,tbuf+5);
        bdcpy(id_buf,tbuf);
		//sscanf(buf,"%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",id_buf,id_buf+1,id_buf+2,id_buf+3,id_buf+4,id_buf+5);
		printf("id_buf->%02x:%02x:%02x:%02x:%02x:%02x\n",id_buf[0],id_buf[1],\
			id_buf[2],id_buf[3],id_buf[4],id_buf[5]);
        /*fd = open("/usr/data/bsa/bt_addr", O_RDONLY);
        if(fd>=0){
            read(fd,buf2,128);
            close(fd);
            if(strcmp(buf,buf2)){
                system("rm -rf /usr/data/bsa");
            }
        }*/
	}else{
		printf("read baddr fail.fd: %d,buf[0]:%02hh\n",fd,buf[0]);
        ret = -1;
    }
    return ret;
}
int get_wifi_addr(BD_ADDR wifi_mac){
    UINT32 ret;
	UINT8 macbuf[6]={0};
    ret = get_mac("/sys/class/net/wlan0/address", macbuf);
    if(ret == 6)
    {
        //APP_ERROR0("blue MAC:->");
        //printf("------------------start copy\n");
		bdcpy(wifi_mac,macbuf);
        //printf("\n===================blue MAC:->%02x-%02x-%02x-%02x-%02x-%02x-\n",bt_mac[0],bt_mac[1],bt_mac[2],bt_mac[3],bt_mac[4],bt_mac[5]);
    }
    return ret;
}
int get_bt_addr(BD_ADDR bt_mac)
{
    UINT32 i;
    UINT32 ret;
	UINT8 macbuf[6]={0};

    //jz_efuse_read(0, 16, buf);
    ret = get_mac(ID_FILE, macbuf);
    if(ret == 6)
    {
        //APP_ERROR0("blue MAC:->");
		bdcpy(bt_mac,macbuf);
        printf("\n===================blue MAC:->%02x-%02x-%02x-%02x-%02x-%02x-\n",bt_mac[0],bt_mac[1],bt_mac[2],bt_mac[3],bt_mac[4],bt_mac[5]);
    }else
	{
		printf("\n===================can't get blue MAC.\n");
	}
    return ret;
}
/*******************************************************************************
 **
 ** Function         app_mgr_config
 **
 ** Description      Configure the BSA server
 **
 ** Parameters       None
 **
 ** Returns          Status of the operation
 **
 *******************************************************************************/
int app_mgr_config(char* bt_name)
{
	int                 status;
	int                 index;
	BD_ADDR             local_bd_addr = APP_DEFAULT_BD_ADDR;
    UINT8 baddr_buf[20];
	DEV_CLASS           local_class_of_device = APP_DEFAULT_CLASS_OF_DEVICE;
	tBSA_SEC_ADD_DEV    bsa_add_dev_param;
	tBSA_SEC_ADD_SI_DEV bsa_add_si_dev_param;
	struct timeval      tv;
	unsigned int        rand_seed;
	tBSA_DM_GET_CONFIG  bt_config;
	/*
	 * The rest of the initialization function must be done
	 * for application boot and when Bluetooth is restarted
	 */
    BD_ADDR btmac,curset;

    memset(btmac, 0, sizeof(BD_ADDR));
    get_bt_addr(btmac);
	/* Example of function to read the XML file which contains the
	 * Local Bluetooth configuration
	 * */
	status = app_mgr_read_config();
    if(status >=0){
        if(memcmp(app_xml_config.bd_addr, btmac, sizeof(BD_ADDR)) !=0){//unsame蓝牙地址不一致，重写config文件
            status = -1;
        }
    }
	if (status < 0)
	{
		APP_ERROR0("Creating default XML config file");
		char ble_name[30];
		stpcpy(ble_name, bt_name);
		  //strncat(ble_name, "-nc", 3);
		app_xml_config.enable = TRUE;
		app_xml_config.discoverable = TRUE;
		app_xml_config.connectable = TRUE;
		strncpy((char *)app_xml_config.name, bt_name, sizeof(app_xml_config.name));
		app_xml_config.name[sizeof(app_xml_config.name) - 1] = '\0';
		strncpy((char *)app_xml_config.ble_name, ble_name, sizeof(app_xml_config.ble_name));
		app_xml_config.ble_name[sizeof(app_xml_config.ble_name) - 1] = '\0';
#ifdef USE_CHIP_BT_ADDR
		status = BSA_DmGetConfigInit(&bt_config);
		status = BSA_DmGetConfig(&bt_config);
		if (status != BSA_SUCCESS){
			APP_ERROR1("BSA_DmGetConfig failed:%d", status);
			return(-1);
		}
		bdcpy(app_xml_config.bd_addr, bt_config.bd_addr);
#else
		if(6 == get_bt_addr(baddr_buf)){
            printf("\n\n ------------set baddr\n");
            bdcpy(app_xml_config.bd_addr, baddr_buf);
        }else {
            printf("\n\n ------------set default addr\n");
		    bdcpy(app_xml_config.bd_addr, local_bd_addr);
            #if 1
                gettimeofday(&tv, NULL);
                rand_seed = tv.tv_sec * tv.tv_usec * getpid();
                app_xml_config.bd_addr[4] = rand_r(&rand_seed);
                app_xml_config.bd_addr[5] = rand_r(&rand_seed);
            #endif
        }
        printf("=================will write bd_addr:->%02x-%02x-%02x-%02x-%02x-%02x-\n",
			app_xml_config.bd_addr[0],app_xml_config.bd_addr[1],
			app_xml_config.bd_addr[2],app_xml_config.bd_addr[3],
			app_xml_config.bd_addr[4],app_xml_config.bd_addr[5]);
		/* let's use a random number for the last two bytes of the BdAddr */
#endif
		memcpy(app_xml_config.class_of_device, local_class_of_device, sizeof(DEV_CLASS));
		strncpy(app_xml_config.root_path, APP_DEFAULT_ROOT_PATH, sizeof(app_xml_config.root_path));
		app_xml_config.root_path[sizeof(app_xml_config.root_path) - 1] = '\0';
		strncpy((char *)app_xml_config.pin_code, APP_DEFAULT_PIN_CODE, APP_DEFAULT_PIN_LEN);
		/* The following code will not work if APP_DEFAULT_PIN_LEN is 16 bytes */
		app_xml_config.pin_code[APP_DEFAULT_PIN_LEN] = '\0';
		app_xml_config.pin_len = APP_DEFAULT_PIN_LEN;
		app_xml_config.io_cap = APP_SEC_IO_CAPABILITIES;

        printf("=================write bd_addr:->%02x-%02x-%02x-%02x-%02x-%02x-\n",
			app_xml_config.bd_addr[0],app_xml_config.bd_addr[1],
			app_xml_config.bd_addr[2],app_xml_config.bd_addr[3],
			app_xml_config.bd_addr[4],app_xml_config.bd_addr[5]);
		status = app_mgr_write_config();
		if (status < 0)
		{
			APP_ERROR0("Unable to Create default XML config file");
			app_mgt_close();
			return status;
		}
	}

	/* Example of function to read the database of remote devices */
	status = app_mgr_read_remote_devices();
	if (status < 0)
	{
		APP_ERROR0("No remote device database found");
	}
	else
	{
		app_xml_display_devices(app_xml_remote_devices_db,
				APP_NUM_ELEMENTS(app_xml_remote_devices_db));
	}

	/* Example of function to get the Local Bluetooth configuration */
	app_mgr_get_bt_config();

	/* Example of function to set the Bluetooth Security */
	status = app_mgr_sec_set_security();
	if (status < 0)
	{
		APP_ERROR1("app_mgr_sec_set_security failed:%d", status);
		app_mgt_close();
		return status;
	}

	/* Add every devices found in remote device database */
	/* They will be able to connect to our device */
	APP_INFO0("Add all devices found in database");
	for (index = 0 ; index < APP_NUM_ELEMENTS(app_xml_remote_devices_db) ; index++)
	{
		if (app_xml_remote_devices_db[index].in_use != FALSE)
		{
			APP_INFO1("Adding:%s", app_xml_remote_devices_db[index].name);
			BSA_SecAddDeviceInit(&bsa_add_dev_param);
			bdcpy(bsa_add_dev_param.bd_addr,
					app_xml_remote_devices_db[index].bd_addr);
			memcpy(bsa_add_dev_param.class_of_device,
					app_xml_remote_devices_db[index].class_of_device,
					sizeof(DEV_CLASS));
			memcpy(bsa_add_dev_param.link_key,
					app_xml_remote_devices_db[index].link_key,
					sizeof(LINK_KEY));
			bsa_add_dev_param.link_key_present = app_xml_remote_devices_db[index].link_key_present;
			bsa_add_dev_param.trusted_services = app_xml_remote_devices_db[index].trusted_services;
			bsa_add_dev_param.is_trusted = TRUE;
			bsa_add_dev_param.key_type = app_xml_remote_devices_db[index].key_type;
			bsa_add_dev_param.io_cap = app_xml_remote_devices_db[index].io_cap;
#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
			bsa_add_dev_param.ble_addr_type = app_xml_remote_devices_db[index].ble_addr_type;
			bsa_add_dev_param.device_type = app_xml_remote_devices_db[index].device_type;
			bsa_add_dev_param.inq_result_type = app_xml_remote_devices_db[index].inq_result_type;
			if(app_xml_remote_devices_db[index].ble_link_key_present)
			{
				bsa_add_dev_param.ble_link_key_present = TRUE;
				/* Fill PENC Key */
				memcpy(bsa_add_dev_param.le_penc_key.ltk,
						app_xml_remote_devices_db[index].penc_ltk,
						sizeof(app_xml_remote_devices_db[index].penc_ltk));
				memcpy(bsa_add_dev_param.le_penc_key.rand,
						app_xml_remote_devices_db[index].penc_rand,
						sizeof(app_xml_remote_devices_db[index].penc_rand));
				bsa_add_dev_param.le_penc_key.ediv = app_xml_remote_devices_db[index].penc_ediv;
				bsa_add_dev_param.le_penc_key.sec_level = app_xml_remote_devices_db[index].penc_sec_level;
				bsa_add_dev_param.le_penc_key.key_size = app_xml_remote_devices_db[index].penc_key_size;
				/* Fill PID Key */
				memcpy(bsa_add_dev_param.le_pid_key.irk,
						app_xml_remote_devices_db[index].pid_irk,
						sizeof(app_xml_remote_devices_db[index].pid_irk));
				bsa_add_dev_param.le_pid_key.addr_type = app_xml_remote_devices_db[index].pid_addr_type;
				memcpy(bsa_add_dev_param.le_pid_key.static_addr,
						app_xml_remote_devices_db[index].pid_static_addr,
						sizeof(app_xml_remote_devices_db[index].pid_static_addr));
				/* Fill PCSRK Key */
				bsa_add_dev_param.le_pcsrk_key.counter = app_xml_remote_devices_db[index].pcsrk_counter;
				memcpy(bsa_add_dev_param.le_pcsrk_key.csrk,
						app_xml_remote_devices_db[index].pcsrk_csrk,
						sizeof(app_xml_remote_devices_db[index].pcsrk_csrk));
				bsa_add_dev_param.le_pcsrk_key.sec_level = app_xml_remote_devices_db[index].pcsrk_sec_level;
				/* Fill LCSRK Key */
				bsa_add_dev_param.le_lcsrk_key.counter = app_xml_remote_devices_db[index].lcsrk_counter;
				bsa_add_dev_param.le_lcsrk_key.div = app_xml_remote_devices_db[index].lcsrk_div;
				bsa_add_dev_param.le_lcsrk_key.sec_level = app_xml_remote_devices_db[index].lcsrk_sec_level;
				/* Fill LENC Key */
				bsa_add_dev_param.le_lenc_key.div = app_xml_remote_devices_db[index].lenc_div;
				bsa_add_dev_param.le_lenc_key.key_size = app_xml_remote_devices_db[index].lenc_key_size;
				bsa_add_dev_param.le_lenc_key.sec_level = app_xml_remote_devices_db[index].lenc_sec_level;
			}
#endif
			BSA_SecAddDevice(&bsa_add_dev_param);
		}
	}

	/* Add stored SI devices to BSA server */
	app_read_xml_si_devices();
	for (index = 0 ; index < APP_NUM_ELEMENTS(app_xml_si_devices_db) ; index++)
	{
		if (app_xml_si_devices_db[index].in_use)
		{
			BSA_SecAddSiDevInit(&bsa_add_si_dev_param);
			bdcpy(bsa_add_si_dev_param.bd_addr, app_xml_si_devices_db[index].bd_addr);
			bsa_add_si_dev_param.platform = app_xml_si_devices_db[index].platform;
			BSA_SecAddSiDev(&bsa_add_si_dev_param);
		}
	}

	return 0;
}

/*******************************************************************************
 **
 ** Function        app_mgr_change_dual_stack_mode
 **
 ** Description     Toggle Dual Stack Mode (BSA <=> MM/Kernel)
 **
 ** Parameters      None
 **
 ** Returns         Status
 **
 *******************************************************************************/
int app_mgr_change_dual_stack_mode(void)
{
	int choice;
	tBSA_DM_DUAL_STACK_MODE dual_stack_mode;

	APP_INFO1("Current DualStack mode: %s", app_mgr_get_dual_stack_mode_desc());
	APP_INFO0("Select the new DualStack mode:");
	APP_INFO0("\t0: DUAL_STACK_MODE_BSA");
	APP_INFO0("\t1: DUAL_STACK_MODE_MM/Kernel (requires specific BTUSB driver)");
	APP_INFO0("\t2: DUAL_STACK_MODE_BTC (requires specific FW)");
	choice = app_get_choice("Choice");

	switch(choice)
	{
		case 0:
			APP_INFO0("Switching Dual Stack to BSA");
			dual_stack_mode = BSA_DM_DUAL_STACK_MODE_BSA;
			break;

		case 1:
			APP_INFO0("Switching Dual Stack to MM/Kernel");
			dual_stack_mode = BSA_DM_DUAL_STACK_MODE_MM;
			break;

		case 2:
			APP_INFO0("Switching Dual Stack to BTC");
			dual_stack_mode = BSA_DM_DUAL_STACK_MODE_BTC;
			break;

		default:
			APP_ERROR1("Bad DualStack Mode=%d", choice);
			return -1;
	}

	if (app_dm_set_dual_stack_mode(dual_stack_mode) < 0)
	{
		APP_ERROR0("DualStack Switch Failed");
		return -1;
	}

	app_mgr_cb.dual_stack_mode = dual_stack_mode;

	return 0;
}

/*******************************************************************************
 **
 ** Function        app_mgr_get_dual_stack_mode_desc
 **
 ** Description     Get Dual Stack Mode description
 **
 ** Parameters      None
 **
 ** Returns         Description string
 **
 *******************************************************************************/
char *app_mgr_get_dual_stack_mode_desc(void)
{
	switch(app_mgr_cb.dual_stack_mode)
	{
		case BSA_DM_DUAL_STACK_MODE_BSA:
			return "DUAL_STACK_MODE_BSA";
		case BSA_DM_DUAL_STACK_MODE_MM:
			return "DUAL_STACK_MODE_MM/Kernel";
		case BSA_DM_DUAL_STACK_MODE_BTC:
			return "DUAL_STACK_MODE_BTC";
		default:
			return "Unknown Dual Stack Mode";
	}
}

/*******************************************************************************
 **
 ** Function         app_mgr_discovery_test
 **
 ** Description      This function performs a specific discovery
 **
 ** Parameters       None
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_mgr_discovery_test(void)
{
	int choice;

	APP_INFO0("Select the parameter to modify:");
	APP_INFO0("\t0: Update type");
	APP_INFO0("\t1: Inquiry TX power");
	choice = app_get_choice("Choice");

	switch(choice)
	{
		case 0:
			APP_INFO0("Select which update is needed:");
			APP_INFO1("\t%d: End of discovery (default)", BSA_DISC_UPDATE_END);
			APP_INFO1("\t%d: Upon any modification", BSA_DISC_UPDATE_ANY);
			APP_INFO1("\t%d: Upon Name Reception", BSA_DISC_UPDATE_NAME);
			choice = app_get_choice("");

			return app_disc_start_update((tBSA_DISC_UPDATE)choice);
			break;

		case 1:
			choice = app_get_choice("Enter new Inquiry TX power");

			return app_disc_start_power((INT8)choice);

			break;

		default:
			APP_ERROR0("Unsupported choice value");
			break;
	}
	return -1;
}

/*******************************************************************************
 **
 ** Function         app_mgr_read_version
 **
 ** Description      This function is used to read BSA and FW version
 **
 ** Parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_mgr_read_version(void)
{
	tBSA_TM_READ_VERSION bsa_read_version;
	tBSA_STATUS bsa_status;

	bsa_status = BSA_TmReadVersionInit(&bsa_read_version);
	bsa_status = BSA_TmReadVersion(&bsa_read_version);
	if (bsa_status != BSA_SUCCESS)
	{
		APP_ERROR1("BSA_TmReadVersion failed status:%d", bsa_status);
		return(-1);
	}

	APP_INFO1("Server status:%d", bsa_read_version.status);
	APP_INFO1("FW Version:%d.%d.%d.%d",
			bsa_read_version.fw_version.major,
			bsa_read_version.fw_version.minor,
			bsa_read_version.fw_version.build,
			bsa_read_version.fw_version.config);
	APP_INFO1("BSA Server Version:%s", bsa_read_version.bsa_server_version);
	return 0;
}


/*******************************************************************************
 **
 ** Function         app_mgr_set_link_policy
 **
 ** Description      Set the device link policy
 **                  This function sets/clears the link policy mask to the given
 **                  bd_addr.
 **                  If clearing the sniff or park mode mask, the link is put
 **                  in active mode.
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_mgr_set_link_policy(BD_ADDR bd_addr, tBSA_DM_LP_MASK policy_mask, BOOLEAN set)
{
	tBSA_DM_SET_CONFIG bsa_dm_set_config;
	if (BSA_DmSetConfigInit(&bsa_dm_set_config) != BSA_SUCCESS)
	{
		APP_ERROR0("BSA_DmSetConfigInit failed");
		return;
	}

	bsa_dm_set_config.config_mask = BSA_DM_CONFIG_LINK_POLICY_MASK;

	bdcpy(bsa_dm_set_config.policy_param.link_bd_addr, bd_addr);
	bsa_dm_set_config.policy_param.policy_mask = policy_mask;
	bsa_dm_set_config.policy_param.set = set;

	if (BSA_DmSetConfig(&bsa_dm_set_config) != BSA_SUCCESS)
	{
		APP_ERROR0("BSA_DmSetConfig failed");
		return;
	}
}

/*******************************************************************************
 **
 ** Function         app_mgr_sec_bond
 **
 ** Description      Bond a device
 **
 ** Parameters       Bond device name
 **
 ** Returns          0 if success / -1 if error
 **
 *******************************************************************************/
int app_mgr_sec_bond(const char* bd_name)
{
    int status;
    int index = 0;
    tBSA_SEC_BOND sec_bond;
    app_disc_display_devices();

    for(;index < APP_DISC_NB_DEVICES; index++) {
	if ((strcmp(bd_name, app_discovery_cb.devs[index].device.name) == 0)
	    && (app_discovery_cb.devs[index].in_use != FALSE)) {
	    BSA_SecBondInit(&sec_bond);
	    bdcpy(sec_bond.bd_addr, app_discovery_cb.devs[index].device.bd_addr);
	    status = BSA_SecBond(&sec_bond);
	    if (status == BSA_SUCCESS)
		{
		    return 0;
		}
	}
    }

    APP_ERROR1("BSA_SecBond failed:%d", status);
    return -1;
}
int app_mgr_remove_device(BD_ADDR bd_addr)
{
    int status;
    tBSA_SEC_REMOVE_DEV sec_remove;

    APP_INFO0("app_mgr_remove_device");

    /* Remove the device from Security database (BSA Server) */
    BSA_SecRemoveDeviceInit(&sec_remove);
    bdcpy(sec_remove.bd_addr, bd_addr);
    status = BSA_SecRemoveDevice(&sec_remove);
    if (status != BSA_SUCCESS)
    {
        APP_INFO1("BSA_SecRemoveDevice fail,ret=%d",status);
        return -1;
    }
    return 0;
}
/*******************************************************************************
 **
 ** Function         app_mgr_sec_unpair
 **
 ** Description      Unpair a device
 **
 ** Parameters       Bond device name
 **
 ** Returns          0 if success / -1 if error
 **
 *******************************************************************************/
int app_mgr_sec_unpair_addr(BD_ADDR bd_addr)
{
    int status;
    int device_index;
    tBSA_SEC_REMOVE_DEV sec_remove;

    APP_INFO0("app_mgr_sec_unpair");

    /* Remove the device from Security database (BSA Server) */
    BSA_SecRemoveDeviceInit(&sec_remove);

    /* Read the XML file which contains all the bonded devices */
    app_read_xml_remote_devices();

    for(device_index=0;device_index < APP_MAX_NB_REMOTE_STORED_DEVICES; device_index++) {
        if ((bdcmp(bd_addr, app_xml_remote_devices_db[device_index].bd_addr) == 0)
        && (app_xml_remote_devices_db[device_index].in_use != FALSE)) {

        bdcpy(sec_remove.bd_addr, app_xml_remote_devices_db[device_index].bd_addr);
        status = BSA_SecRemoveDevice(&sec_remove);
        if (status == BSA_SUCCESS)
            {
            /* delete the device from database */
            app_xml_remote_devices_db[device_index].in_use = FALSE;
            app_write_xml_remote_devices();
            return 0;
            }
        }
    }

    APP_ERROR1("BSA_SecRemoveDevice Operation Failed with status [%d]",status);
    return -1;
}
/*******************************************************************************
 **
 ** Function         app_mgr_sec_unpair
 **
 ** Description      Unpair a device
 **
 ** Parameters       Bond device name
 **
 ** Returns          0 if success / -1 if error
 **
 *******************************************************************************/
int app_mgr_sec_unpair(const char* bd_name)
{
	int status;
	int device_index;
	tBSA_SEC_REMOVE_DEV sec_remove;

	APP_INFO0("app_mgr_sec_unpair");

	/* Remove the device from Security database (BSA Server) */
	BSA_SecRemoveDeviceInit(&sec_remove);

	/* Read the XML file which contains all the bonded devices */
	app_read_xml_remote_devices();

	for(device_index=0;device_index < APP_MAX_NB_REMOTE_STORED_DEVICES; device_index++) {
	    if ((strcmp(bd_name, app_xml_remote_devices_db[device_index].name) == 0)
		&& (app_xml_remote_devices_db[device_index].in_use != FALSE)) {

		bdcpy(sec_remove.bd_addr, app_xml_remote_devices_db[device_index].bd_addr);
		status = BSA_SecRemoveDevice(&sec_remove);
		if (status == BSA_SUCCESS)
		    {
			/* delete the device from database */
			app_xml_remote_devices_db[device_index].in_use = FALSE;
			app_write_xml_remote_devices();
			return 0;
		    }
	    }
	}

	APP_ERROR1("BSA_SecRemoveDevice Operation Failed with status [%d]",status);
	return -1;
}
int app_mgr_del_linked_devices(const BD_ADDR addr)
{
    int index;
    int ret = 0;
    //app_read_xml_remote_devices();
    for (index = 0; index < APP_NUM_ELEMENTS(app_xml_remote_devices_db); index++)
    {
        if(app_xml_remote_devices_db[index].in_use != FALSE)
        {
            //if(memcmp(app_xml_remote_devices_db[index].bd_addr, addr, sizeof(BD_ADDR))==0)
            if(bdcmp(app_xml_remote_devices_db[index].bd_addr, addr) == 0)
            {
                APP_INFO1("del name:%s,type:%d,bd_addr:%02x:%02x:%02x:%02x:%02x:%02x", app_xml_remote_devices_db[index].name,
                    (unsigned char)addr[0],(unsigned char)addr[1],
                    (unsigned char)addr[2],(unsigned char)addr[3],
                    (unsigned char)addr[4],(unsigned char)addr[5]);
                app_mgr_sec_unpair_addr(addr);
                //if(ret !=0 )
                {
                    app_xml_remote_devices_db[index].in_use = FALSE;
                    ret = app_write_xml_remote_devices();
                }
                break;
            }
        }
    }
    return ret;
}
int app_mgr_get_linking_btname(tLINKING_DEVICE *linking_dev) {
    int index;
    int ret = 0;
    //app_read_xml_remote_devices();
    for (index = 0; index < APP_NUM_ELEMENTS(app_xml_remote_devices_db); index++)
    {
        if(app_xml_remote_devices_db[index].in_use != FALSE)
        {
            //if(memcmp(app_xml_remote_devices_db[index].bd_addr, addr, sizeof(BD_ADDR))==0)
            if(bdcmp(app_xml_remote_devices_db[index].bd_addr, linking_dev->bd_addr) == 0)
            {
                strcpy(linking_dev->bd_name, app_xml_remote_devices_db[index].name);
                break;
            }
        }
    }
    return 0;
}
int app_mgr_get_linking_devices(void (*appGetLinkingCback)(int event, tAPP_DISC_MSG* p_data), int duration)
{
    int index;
    if(appGetLinkingCback != NULL)
    {
        for (index = 0; index < APP_MAX_NB_REMOTE_STORED_DEVICES; index++)
        {
            if(linking_dev[index].in_use != FALSE)
            {
                tAPP_DISC_MSG msg;
                /* char addr[20]={0}; */
                /* snprintf(addr,20,"%02x:%02x:%02x:%02x:%02x:%02x", */
                /*      p_data->disc_new.bd_addr[0], */
                /*      p_data->disc_new.bd_addr[1], */
                /*      p_data->disc_new.bd_addr[2], */
                /*      p_data->disc_new.bd_addr[3], */
                /*      p_data->disc_new.bd_addr[4], */
                /*      p_data->disc_new.bd_addr[5]); */
                /* msg.disc_new.bd_addr = (const char*)addr; */
				msg.disc_new.bd_type = linking_dev[index].bd_type;
				msg.disc_new.bd_addr = (const char*)(linking_dev[index].bd_addr);
                if(strlen(linking_dev[index].bd_name) == 0){//从连接过设备里找名称
                    app_mgr_get_linking_btname(linking_dev +index);
                }
                msg.disc_new.name = (const char*)linking_dev[index].bd_name;
                APP_INFO1("linking name:%s,type:%d,bd_addr:%02x:%02x:%02x:%02x:%02x:%02x", msg.disc_new.name, msg.disc_new.bd_type,
                    (unsigned char)msg.disc_new.bd_addr[0],(unsigned char)msg.disc_new.bd_addr[1],
                    (unsigned char)msg.disc_new.bd_addr[2],(unsigned char)msg.disc_new.bd_addr[3],
                    (unsigned char)msg.disc_new.bd_addr[4],(unsigned char)msg.disc_new.bd_addr[5]);
                appGetLinkingCback(BSA_DISC_NEW_EVT, &msg);
            }
        }
        appGetLinkingCback(BSA_DISC_CMPL_EVT, NULL);
    }
}

int app_mgr_get_linked_devices(void (*appGetLinkedCback)(int event, tAPP_DISC_MSG* p_data), int duration)
{
    int index;
    if(appGetLinkedCback != NULL)
    {
        for (index = 0; index < APP_NUM_ELEMENTS(app_xml_remote_devices_db); index++)
        {
            if(app_xml_remote_devices_db[index].in_use != FALSE)
            {
                tAPP_DISC_MSG msg;
                /* char addr[20]={0}; */
                /* snprintf(addr,20,"%02x:%02x:%02x:%02x:%02x:%02x", */
                /*      p_data->disc_new.bd_addr[0], */
                /*      p_data->disc_new.bd_addr[1], */
                /*      p_data->disc_new.bd_addr[2], */
                /*      p_data->disc_new.bd_addr[3], */
                /*      p_data->disc_new.bd_addr[4], */
                /*      p_data->disc_new.bd_addr[5]); */
                /* msg.disc_new.bd_addr = (const char*)addr; */
                //BTM_COD_MAJOR_CLASS( msg.disc_new.bd_type, app_xml_remote_devices_db[index].class_of_device);
                printf("class:=%02hhx:%02hhx:%02hhx\n",
                    app_xml_remote_devices_db[index].class_of_device[0],app_xml_remote_devices_db[index].class_of_device[1],
                    app_xml_remote_devices_db[index].class_of_device[2]);
                if(app_xml_remote_devices_db[index].trusted_services & BSA_BLE_SERVICE_MASK)
                    msg.disc_new.bd_type = 1;
                else if(app_xml_remote_devices_db[index].trusted_services & BSA_SPP_SERVICE_MASK)
                    msg.disc_new.bd_type = 2;
                else if(app_xml_remote_devices_db[index].trusted_services & BSA_A2DP_SERVICE_MASK)
                    msg.disc_new.bd_type = 3;
                else if((app_xml_remote_devices_db[index].class_of_device[1] & 0x1F) == BTM_COD_MAJOR_AUDIO)
                    msg.disc_new.bd_type = 3;
                else
                    msg.disc_new.bd_type = 0;

                msg.disc_new.bd_addr = (const char*)(app_xml_remote_devices_db[index].bd_addr);
                msg.disc_new.name = (const char*)app_xml_remote_devices_db[index].name;
                APP_INFO1("linked name:%s,type:%d,bd_addr::%02x:%02x:%02x:%02x:%02x:%02x", msg.disc_new.name,msg.disc_new.bd_type,
                    (unsigned char)msg.disc_new.bd_addr[0],(unsigned char)msg.disc_new.bd_addr[1],
                    (unsigned char)msg.disc_new.bd_addr[2],(unsigned char)msg.disc_new.bd_addr[3],
                    (unsigned char)msg.disc_new.bd_addr[4],(unsigned char)msg.disc_new.bd_addr[5]);
                appGetLinkedCback(BSA_DISC_NEW_EVT, &msg);
            }
        }
        appGetLinkedCback(BSA_DISC_CMPL_EVT, NULL);
    }
}

int app_mgr_start_discovery(void (*appDiscCback)(int event, tAPP_DISC_MSG* p_data), int duration, int type){
    mAppDiscCback = appDiscCback;
    if(type == 0)//search all type device
        app_disc_start_regular(&app_disc_cback, duration);
    else//search audio device
        app_disc_start_cod(BTM_COD_SERVICE_AUDIO, BTM_COD_MAJOR_AUDIO, 0, &app_disc_cback);
}

void app_disc_cback(tBSA_DISC_EVT event, tBSA_DISC_MSG *p_data){
	switch (event)
	{
	    /* a New Device has been discovered */
	case BSA_DISC_NEW_EVT:
	    /* If User provided a callback, let's call it */
	    if (mAppDiscCback != NULL)
		{
		    tAPP_DISC_MSG msg;
		    /* char addr[20]={0}; */
		    /* snprintf(addr,20,"%02x:%02x:%02x:%02x:%02x:%02x", */
		    /* 	    p_data->disc_new.bd_addr[0], */
		    /* 	    p_data->disc_new.bd_addr[1], */
		    /* 	    p_data->disc_new.bd_addr[2], */
		    /* 	    p_data->disc_new.bd_addr[3], */
		    /* 	    p_data->disc_new.bd_addr[4], */
		    /* 	    p_data->disc_new.bd_addr[5]); */
		    /* msg.disc_new.bd_addr = (const char*)addr; */
			BTM_COD_MAJOR_CLASS( msg.disc_new.bd_type, (p_data->disc_new.class_of_device));
		    msg.disc_new.bd_addr = (const char*)(p_data->disc_new.bd_addr);
		    msg.disc_new.name = (const char*)p_data->disc_new.name;
            APP_INFO1("disc name:%s,type:%d,bd_addr::%02x:%02x:%02x:%02x:%02x:%02x", msg.disc_new.name,msg.disc_new.bd_type,
                (unsigned char)msg.disc_new.bd_addr[0],(unsigned char)msg.disc_new.bd_addr[1],
                (unsigned char)msg.disc_new.bd_addr[2],(unsigned char)msg.disc_new.bd_addr[3],
                (unsigned char)msg.disc_new.bd_addr[4],(unsigned char)msg.disc_new.bd_addr[5]);
		    mAppDiscCback(event, &msg);
		}
	    if (p_data->disc_new.eir_data[0])
		{
		    app_disc_parse_eir(p_data->disc_new.eir_data);
		}
	    break;

	case BSA_DISC_CMPL_EVT: /* Discovery complete. */
	    APP_INFO0("Discovery complete");
	    mAppDiscCback(event, NULL);
	    mAppDiscCback = NULL;
	    break;
	default:
	    ALOGE("app_disc_cback unknown event:%d", event);
	    break;
	}
}
