#define LOG_NDEBUG 0
#define LOG_TAG "DBHelper"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include <utils/Log.h>
#include "DBHelper.h"

#define DB_DIR_PATH "/usr/data/database/"
#define DB_CONFIG_FILE_PATH "/usr/data/database/bt_db.xml"

namespace android{

int DBHelper::writeStateToDB(bool bind_state, const char* bind_addr)
{
    if (!checkDatabaseDir()) {
	ALOGE("can't mkdir filep(%s)", DB_DIR_PATH);
	return 0;
    }

    FILE* pFile = fopen(DB_CONFIG_FILE_PATH,"w+"); 
    if (NULL == pFile) {
	    ALOGE("can't open filep(%s)", DB_CONFIG_FILE_PATH);
	    return 0;
    }
   
    t_DB_CONFIG cfg;
    cfg.bind_state = bind_state;
    memset(cfg.bind_addr, 0, sizeof(cfg.bind_addr));
    strncpy(cfg.bind_addr, bind_addr, 
	    strlen(bind_addr) > 0 ? strlen(bind_addr) : strlen(cfg.bind_addr));

    if(fwrite(&cfg, sizeof(t_DB_CONFIG), 1, pFile)){
	    fclose(pFile);
	    return 1;
    }

    fclose(pFile);
    return 0;
}

int DBHelper::readStateFromDB(t_DB_CONFIG & cfg)
{
    FILE* pFile = fopen(DB_CONFIG_FILE_PATH,"r+");
    if (NULL == pFile) {
	    ALOGE("can't open filep(%s)", DB_CONFIG_FILE_PATH);
	    return 0;
    }

    if(fread(&cfg, sizeof(t_DB_CONFIG), 1, pFile) != 1 && ferror(pFile)) {
	    ALOGE("read db error");
	    fclose(pFile); 
	    return 1;
    }

    ALOGD("read data : %d\n", cfg.bind_state);
    
    ALOGD("read addr : %02x:%02x:%02x:%02x:%02x:%02x\n",
	  0xFF & cfg.bind_addr[0], 0xFF & cfg.bind_addr[1],
	  0xFF & cfg.bind_addr[2], 0xFF & cfg.bind_addr[3], 
	  0xFF & cfg.bind_addr[4], 0xFF & cfg.bind_addr[5]);

    fclose(pFile);  

    return 0;
}

bool DBHelper::checkDatabaseDir(){
    DIR *dirptr;
    if ((dirptr = opendir(DB_DIR_PATH)) == NULL) {
	int ret = mkdir(DB_DIR_PATH, 0775);//创建文件夹
	if (ret < 0) {
	    ALOGE(" Creat database path Failed");
	    return false;
	}
    }else
        closedir(dirptr);
    return true;
}

}// namespace android
