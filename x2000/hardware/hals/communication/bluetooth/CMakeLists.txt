cmake_minimum_required(VERSION 3.0)
project(bluetooth LANGUAGES C CXX)
set(PROBLUETOOTH btservice)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=gnu++11 -Wall -fpermissive -Wreturn-type -pthread")
SET(CMAKE_C_COMPILER "/group1/home/rzhao/openharmony/lite_ingenic_1/lite_ingenic/prebuilts/gcc/linux-x86/mips/mips-gcc720-glibc226/bin/mips-linux-gnu-gcc")
SET(CMAKE_CXX_COMPILER "/group1/home/rzhao/openharmony/lite_ingenic_1/lite_ingenic/prebuilts/gcc/linux-x86/mips/mips-gcc720-glibc226/bin/mips-linux-gnu-g++")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-D_GLIBCXX_USE_C99=1 -DUSE_INGENIC_AUDIO_SOCKET -DBLUETOOTH_HILINK_SUPPORT -DBLE_INCLUDED=true -DBUILDCFG -DAPP_TRACE_COLOR -DALSA_SUPPORT=true)

link_directories(
	"${PROJECT_SOURCE_DIR}/frameworks/lib"
	)

add_library(${PROBLUETOOTH} SHARED
#add_library(${PROBLUETOOTH} STATIC
		"frameworks/ohos_bt.c"
		#	"frameworks/BtMain.cpp"
		"frameworks/source/app_common/app_dm.c"
		"frameworks/source/app_common/app_utils.c"
		"frameworks/source/app_common/app_disc.c"
		"frameworks/source/app_common/app_mgt.c"
		"frameworks/source/app_common/app_wav.c"
		"frameworks/source/app_common/app_mutex.c"
		"frameworks/source/app_common/app_link.c"
		"frameworks/source/app_common/app_thread.c"
		"frameworks/source/app_common/app_playlist.c"
		"frameworks/source/app_common/nanoxml.c"
		"frameworks/source/app_common/app_services.c"
		"frameworks/source/app_common/app_xml_utils.c"
		"frameworks/source/app_common/app_xml_param.c"
		#"frameworks/source/app_unit.c"
		"frameworks/source/app_manager.c"
		"frameworks/source/app_avk.c"
		"frameworks/source/app_hs.c"
		"frameworks/source/app_dg.c"
		"frameworks/source/app_ble.c"
		"frameworks/source/app_ble_server.c"
		"frameworks/source/app_av.c"
		"frameworks/source/app_ag.c"
		"frameworks/protocol/SppProtocol.cpp"
		"frameworks/protocol/UnBindModule.cpp"
		"frameworks/protocol/SyncModule.cpp"
		"frameworks/protocol/SyncDataTools.cpp"
		"frameworks/protocol/SyncData.cpp"
		"frameworks/sdk/ThirdPartyServer.cpp"
		"frameworks/sdk/BluetoothServer.cpp"
		"frameworks/protocol/DBHelper.cpp"
		"frameworks/sdk/HilinkServer.cpp"
		"frameworks/ble/BleProtocol.cpp"
		"frameworks/source/ingenic_oss_output.c"
	)

target_link_libraries(
  ${PROBLUETOOTH}
	"bsa"
	"stdc++"
	"rt"
	"asound"
)

target_include_directories(${PROBLUETOOTH} PUBLIC
	"${PROJECT_SOURCE_DIR}/../../../../../../../foundation/communication/bluetooth/interfaces/innerkits/native_c/include"
	"${PROJECT_SOURCE_DIR}/../../../../../../../foundation/communication/bluetooth/interfaces/innerkits/native_c/include/utils"
	"${PROJECT_SOURCE_DIR}/../../../../../../../foundation/communication/bluetooth/interfaces/innerkits/native_c/include/bt"
	"${PROJECT_SOURCE_DIR}/../../../../../../../foundation/communication/bluetooth/interfaces/innerkits/native_c/include/bt/app_common"
	"${PROJECT_SOURCE_DIR}/../../../../../../../foundation/communication/bluetooth/interfaces/innerkits/native_c/include/bt/protocol"
	"${PROJECT_SOURCE_DIR}/../../../../../../../foundation/communication/bluetooth/interfaces/innerkits/native_c/include/bt/ble"
	"${PROJECT_SOURCE_DIR}/../../../../../../../foundation/communication/bluetooth/interfaces/innerkits/native_c/include/bt/incall"
	"${PROJECT_SOURCE_DIR}/../../../../../../../foundation/communication/bluetooth/interfaces/innerkits/native_c/include/bt/module"
	"${PROJECT_SOURCE_DIR}/../../../../../../../foundation/communication/bluetooth/interfaces/innerkits/native_c/include/bt/module/ios"
	"${PROJECT_SOURCE_DIR}/../../../../../../../foundation/communication/bluetooth/interfaces/innerkits/native_c/include/libbsa"
	"${PROJECT_SOURCE_DIR}/frameworks/sdk"
)
