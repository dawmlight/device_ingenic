#ifndef __BLUETOOTH_UTILS_H__
#define __BLUETOOTH_UTILS_H__
/* Discovery callback */
#include <stdio.h>
#define BD_ADDR_SIZE 6
#define BD_NAME_SIZE 100
typedef unsigned char BD_ADDR[BD_ADDR_SIZE];         /* Device address */

typedef enum {
    DISC_NEW_EVT, /* a New Device has been discovered */
    DISC_CMPL_EVT, /* End of Discovery */
} DISC_EVT;

typedef enum {// just for linking & linked device type
	BD_TYPE_UNKNOWN = 0,
    BD_TYPE_BLE = 1,
    BD_TYPE_SPP = 2,
    BD_TYPE_A2DP = 3,
} BD_TYPE_ENUM;

typedef struct{
    BD_ADDR bd_addr; /* Bluetooth address peer device. */
    char name[BD_NAME_SIZE];   /* Name of peer device. */
    unsigned char bd_type;
} REMOTE_DEVICE;

#endif /* __BLUETOOTH_UTILS_H__ */
