LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE = libBluetoothSDK
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES :=                      \
		BluetoothController.cpp \
		ThirdPartyModule.cpp \
		HilinkModule.cpp \
		../protocol/SyncData.cpp \
		../protocol/SyncDataTools.cpp

LOCAL_C_INCLUDES := ../include/protocol \
		../include

LOCAL_CFLAGS += -fPIC -D_GNU_SOURCE=1 -DHAVE_PTHREADS -DHAVE_ANDROID_OS -D_FILE_OFFSET_BITS=64 \
		-std=gnu++11
LOCAL_LDFLAGS := -rpath-link=$(OUT_DEVICE_SHARED_DIR)
LOCAL_LDLIBS := -lstdc++ -lrt -lpthread -lm -lc
LOCAL_DEPANNER_MODULES := BluetoothSDK
LOCAL_SHARED_LIBRARIES := libdlog libcutils libutils

ifeq ($(BLUETOOTH_HILINK_SUPPORT), true)
LOCAL_CFLAGS += -DBLUETOOTH_HILINK_SUPPORT
endif

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
###################copy
LOCAL_MODULE := BluetoothSDK
LOCAL_MODULE_TAGS :=optional
LOCAL_MODULE_PATH :=$(OUT_DEVICE_INCLUDE_DIR)
LOCAL_COPY_FILES := \
	bluetooth/HilinkModule.h:HilinkModule.h \
	bluetooth/ThirdPartyModule.h:ThirdPartyModule.h \
	bluetooth/SyncData.h:../include/protocol/SyncData.h \
	bluetooth/BluetoothController.h:BluetoothController.h \
	bluetooth/BluetoothUtils.h:BluetoothUtils.h \
	bluetooth/Type.h:../include/protocol/Type.h

include $(BUILD_MULTI_COPY)

include $(call all-makefiles-under,$(LOCAL_PATH)/../../../../../../../wifi/WpaManager)
