#define LOG_TAG "UnBindModule"

//#include <dlog.h>
#include "UnBindModule.h"
#include "DBHelper.h"
#ifdef __cplusplus
extern "C"
{
#endif
extern int setBTVisibility(bool discoverable, bool connectable);
#ifdef __cplusplus
}
#endif
#define ALOGD printf
//#define SOUND_UNBIND "/usr/bin/MediaPlayer /etc/sound/unbind.mp3 &"

namespace android {

static const string MODULE_NAME = "system_module";
static const string UNBIND_TYPE = "unbind";

UnBindModule* UnBindModule::sInstance = NULL;

UnBindModule* UnBindModule::getInstance(RefBase* protocol)
{
    if(sInstance == NULL){
	    sInstance = new UnBindModule(protocol);
    }
    return sInstance;
}

UnBindModule::UnBindModule(RefBase* protocol) : SyncModule(MODULE_NAME, protocol)
{
    ALOGD("UnBindModule in\n");

}

UnBindModule::~UnBindModule()
{

}

void UnBindModule::onRetrive(const sp<SyncData> & data)
{
     bool unbind = data->getBoolean("unbind");
     if (unbind) {
	 ALOGD("unbind has clear DB\n");
	 //system(SOUND_UNBIND);
	 sp<SyncData> syncdata = new SyncData();
	 send(syncdata);
	 sp<DBHelper> helper = new DBHelper();
	 helper->writeStateToDB(false, "");
	 setBTVisibility(true, true);
     }
}

} //namespace android
