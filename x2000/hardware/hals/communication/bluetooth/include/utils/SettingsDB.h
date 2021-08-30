 /**
 * @defgroup DataBase
 * @ingroup system
 * @brief 数据库模块 向外提供数据库操作的所有接口。
 *
 * 使用此接口需要引用的库为libsettingdb.so
 *
 * 使用此接口需要引用的头文件为 SettingsDB.h
 *
 * 使用示例一、获取数据库默认的一些属性值
 * @code
 * #include <SettingsDB.h>
 * using namespace android;
 * int main(int argc, char** argv)
 * {
 *     ALOGV("%s", __FUNCTION__);
 *
 *     int mWidth,mHeight;
 *     //获取系统支持的录像的宽高
 *     SettingsDB::getInt(DB_KEY_VIDEO_WIDTH,mWidth); 
 *     SettingsDB::getInt(DB_KEY_VIDEO_HEIGHT,mHeight);
 *  
 *     string videoPath = "";
 *     //获取系统默认支持的视频文件存放路径
 *     SettingsDB::getString(DB_KEY_VIDEO_PATH, videoPath);
 *
 *     return 0;
 * }
 *
 * @endcode
 *
 * 当前系统支持的Key值请参阅SettingsDB.h
 *
 * 使用示例二、修改系统默认属性
 * @code
 * #include <SettingsDB.h>
 * using namespace android;
 * int main(int argc, char** argv)
 * {
 *     ALOGV("%s", __FUNCTION__);
 *
 *     int mWidth,mHeight;
 *     //修改系统录像的宽高
 *     SettingsDB::setInt(DB_KEY_VIDEO_WIDTH,1280); 
 *     SettingsDB::setInt(DB_KEY_VIDEO_HEIGHT,720);
 *  
 *     return 0;
 * }

 * @endcode
 *
 * 使用示例三、自定义key
 * 用户可以利用现有数据库存放自定义的<key,value>值，用于重启机后的设置
 * @code
 * #include <SettingsDB.h>
 * using namespace android;
 * int main(int argc, char** argv)
 * {
 *     ALOGV("%s", __FUNCTION__);
 *
 *     string key="own_key";
 *     string value="own_value";
 *     SettingsDB::putString(key,value); 
 *  
 *     return 0;
 * }
 * @endcode
 * @attention key长度限制为小于30 value长度限制为小于40
 */

#ifndef SETTINGS_DB_H_
#define SETTINGS_DB_H_

#include <stdio.h>
#include <stdlib.h>
#include <utils/RefBase.h>
#include <string>
using std::string;
namespace android {

#define KEY_LENGTH 30
#define VALUE_LENGTH 40
struct setting_element{
	char key [KEY_LENGTH];
	char value[VALUE_LENGTH];
};

/*** 以下key值使用: getInt ***/
#define DB_KEY_DEVICE_VOLUME       "volume.persent"
#define DB_KEY_DISK_RESERVE_MB     "disk_reserve_MB"
#define DB_KEY_SLEEP_TIMEOUT       "sleep_timeout" 
#define DB_KEY_PREVIEW_WIDTH       "preview_width"
#define DB_KEY_PREVIEW_HEIGHT      "preview_height"
#define DB_KEY_PREVIEW_FRAMERATE   "preview_framerate"
#define DB_KEY_PICTURE_WIDTH       "picture_width"
#define DB_KEY_PICTURE_HEIGHT      "picture_height"
#define DB_KEY_VIDEO_WIDTH         "video_width"
#define DB_KEY_VIDEO_HEIGHT        "video_height"
#define DB_KEY_VIDEO_FRAMERATE     "video_framerate"
#define DB_KEY_WATERMARK_PICTURE   "picture_water_mark"
#define DB_KEY_WATERMARK_VIDEO     "video_water_mark"
#define CAMERA_TYPE                "capture"
#define DB_KEY_LANGUAGE            "language"
#define DB_KEY_LIVE_RECORD         "live_record"
#define DB_KEY_LIVE_AUDIO          "live_audio"
#define DB_KEY_CAR_MODE            "car_mode" //行车记录仪模式(循环录像), 0表示关闭行车记录仪模式
#define DB_KEY_SUBSECTION_RECORD   "video_time_interval_m" //分段录像时长(单位:分钟):每间隔*分钟保存一次录像, 0表示不分段
#define DB_KEY_PICTURE_SHOOTING_INTERVAL   "picture_shooting_interval"  //连续拍照(单位:s):每间隔*秒拍摄一张照片, 0表示关闭连续拍照功能,使用常规拍照功能
#define DB_KEY_OPTICAL_SOURCE_FREQ         "optical_source_freq"        //电网频率(单位:Hz, 如中国:50Hz)
#define DB_KEY_SOUND_ENABLE                "sound_notification_enable"  //蜂鸣器开关

#define DB_KEY_LCD_BRIGHT_LEVEL           "lcd_bright_level"
#define DB_LCD_BRIGHT_MIN_V         (1)
#define DB_LCD_BRIGHT_MAX_V         (8)
#define DB_LCD_BRAIGH_DFT_V         (3)

#define DB_KEY_DISPLAY_DIRECTION            "display_direction"     //0:left hand, 1:right hand
#define DB_KEY_PWROFF_TIME                  "sys.timing.shutdown"
#define DB_KEY_LCD_BRIGHT_TIMER     "sys.backlight.timer"
#define DB_KEY_SUSPEND_TIMER        "sys.suspend.timer"


/*** 以下key值使用: getString ***/
#define DB_KEY_TIME_LAPSE_FRAME_CAPTURE_MS "video_time_lapse_ms" //延时录像(单位:ms):每间隔*ms一帧图像, 0表示关闭延时录像
#define DB_KEY_VIDEO_QUALITY               "video_image_quality" //视频图像质量(value: high, med, low)
#define CAMERA_STATE                       "camera_state"
#define DB_KEY_BLUETOOTH_ENABLE            "bluetooth_enable"
#define DB_KEY_BLUETOOTH_OPEN_ENABLE       "ro.bluetooth.open.enable"
#define DB_KEY_WIFI_OPEN_ENABLE            "ro.wifi.open.enable"
#define DB_KEY_BLUETOOTH_NAME_COLDWAVE     "ro.bluetooth.name.coldwave"
#define DB_KEY_PHOTO_PATH                  "photo.path"
#define DB_KEY_VIDEO_PATH                  "video.path"
#define DB_KEY_THUMBNAIL_PATH              "thumbnail.path"
#define DB_KEY_BLUETOOTH_NAME              "bluetooth.name"
#define DB_KEY_TENCENTLIVE_CMPUID          "ro.tencent.live.cmpuid"
#define DB_KEY_FIRST_RUN                   "first_run" //false: is not first run

#define DB_DIR_PATH "/usr/data/database/"
#define SETTING_DB_FILE_PATH "/usr/data/database/settings.db"
#define DEVICE_CONF_PATH "/etc/device.conf"
#define USER_CONF_PATH "/data/user.conf"

class SettingsDB : public RefBase{
 public:
    static void putInt(const string & key, int value);
    static void putString(const string & key, const string & value);
    static int  getInt(const string & key);
    static int  getInt(const string & key,int& value);
    static void getString(const string & key, string & value);
    static void setUserConf();
 protected:
    static SettingsDB* getInstance();
    virtual ~SettingsDB();
 private:
    SettingsDB();
    static SettingsDB* sInstance;
    int insertValue(const string & key, const string & value);
    int readValue(const string & key, string & value);
    void setDeviceConf();
    bool checkDatabaseDir();
    void setConf(const string filePath);
};
}
#endif  // SETTINGS_DB_H_
