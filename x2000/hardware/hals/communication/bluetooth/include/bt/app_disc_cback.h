#ifndef __APP_DISC_CBACK_H__
#define __APP_DISC_CBACK_H__
/* Discovery callback */
#include <stdio.h>
typedef enum
{
    APP_DISC_NEW_EVT, /* a New Device has been discovered */
    APP_DISC_CMPL_EVT, /* End of Discovery */
    APP_DISC_DEV_INFO_EVT, /* Device Info discovery event */
    APP_DISC_REMOTE_NAME_EVT  /* Read remote device name event */
} tAPP_DISC_EVT;

typedef struct
{
	unsigned char bd_type;	/*bd type*/
    const char* bd_addr; /* BD address peer device. */
    const char* name;   /* Name of peer device. */
} tAPP_DISC_REMOTE_DEV;

typedef union
{
    tAPP_DISC_REMOTE_DEV disc_new; /* a New Device has been discovered */
} tAPP_DISC_MSG;

/* Discovery callback */
typedef void (tAPP_DISC_CBACK)(int event, tAPP_DISC_MSG p_data);

#endif /* __APP_DISC_CBACK_H__ */
