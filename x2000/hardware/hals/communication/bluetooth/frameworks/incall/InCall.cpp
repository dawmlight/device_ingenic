#define LOG_NDEBUG 0
#define LOG_TAG "InCall"

#include <fcntl.h>
#include <sys/stat.h>
#include <monitor/ProcessMonitor.h>

#include "InCall.h"
#define ALOGV printf
#define ALOGE printf

namespace android {

InCall::InCall(){
	ALOGV("InCall in");
	setState(true);
}

InCall::~InCall()
{
	setState(false);
	ALOGV("~InCall out");
}

bool InCall::setState(bool isActive)
{
	int ret = -1;
	if(isActive){
		ret = setProcessStatus(STATUS_INCALL);//set state machine: BluetoothManager process is in incall state
	}else {
		ret = setProcessStatus(STATUS_INVALID);//set state machine: BluetoothManager process is in empty state
	}
	if(ret < 0){
		ALOGE("InCall setState failed!");
		return false;
	}
	return true;
}

} // namespace android
