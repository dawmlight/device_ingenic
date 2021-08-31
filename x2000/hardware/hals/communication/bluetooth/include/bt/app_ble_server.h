/*****************************************************************************
**
**  Name:           app_ble_server.h
**
**  Description:    Bluetooth BLE include file
**
**  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef APP_BLE_SERVER_H
#define APP_BLE_SERVER_H

#include <stdbool.h>
#include "bsa_api.h"
#include "app_ble.h"

/*******************************************************************************
 **
 ** Function         app_ble_server_find_free_space
 **
 ** Description      find free space for BLE server application
 **
 ** Parameters
 **
 ** Returns          positive number(include 0) if successful, error code otherwise
 **
 *******************************************************************************/
int app_ble_server_find_free_space(void);

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
void app_ble_server_display(void);

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
int app_ble_server_find_reg_pending_index(void);

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
int app_ble_server_find_index_by_interface(tBSA_BLE_IF if_num);

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
int app_ble_server_find_index_by_conn_id(UINT16 conn_id);

/*
 * BLE Server functions
 */
/*******************************************************************************
 **
 ** Function        app_ble_server_register
 **
 ** Description     Register server app
 **
 ** Parameters      uuid: uuid number of application
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_server_register(UINT16 uuid, tBSA_BLE_CBACK *p_cback);

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
int app_ble_server_deregister(void);

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
int app_ble_server_create_service(int server_num, UINT8* service_uuid, bool is_primary, int number);

/*******************************************************************************
 **
 ** Function        app_ble_server_add_char
 **
 ** Description     Add charcter to service
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
#ifdef BLUETOOTH_HILINK_SUPPORT
int app_ble_server_add_char(int server_id, int server_num, UINT8* charac_uuid, int properties, int permissions);
#else
int app_ble_server_add_char(void);
#endif

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
#ifdef BLUETOOTH_HILINK_SUPPORT
int app_ble_server_start_service(int server_id, int num);
#else
int app_ble_server_start_service(void);
#endif

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
int app_ble_server_send_indication(int conn_id, char* indication, int size);

/*******************************************************************************
**
** Function         app_ble_server_profile_cback
**
** Description      BLE Server Profile callback.
**
** Returns          void
**
*******************************************************************************/
void app_ble_server_profile_cback(tBSA_BLE_EVT event,  tBSA_BLE_MSG *p_data);

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
int app_ble_server_open(void);

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
int app_ble_server_close(void);

/*******************************************************************************
 **
 ** Function        app_ble_config_adv_data
 **
 ** Description     config the BLE adv data
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_config_adv_data(void);
/*******************************************************************************
 **
 ** Function        app_ble_server_register_write_cback
 **
 ** Description     Set call back of write characteristic request
 **
 ** Parameters      receiveWriteRequestCback call back function
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_server_register_write_cback(void (*receiveWriteRequestCback)(char *addr,int conn_id, char * data, int size));

#endif
