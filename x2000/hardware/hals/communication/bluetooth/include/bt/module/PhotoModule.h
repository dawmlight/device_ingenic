#ifndef PHOTO_MODULE_H_
#define PHOTO_MODULE_H_

#include <stdio.h>
#include <protocol/SyncModule.h>

namespace android {
class PhotoModule : public SyncModule {
 public:
     static PhotoModule* getInstance(RefBase* protocol);
     virtual ~PhotoModule();
 private:
    PhotoModule(RefBase* protocol);
    static PhotoModule* sInstance;
    void onRetrive(const sp<SyncData> & data);
    void sendCameraState();
    int takePicture();
    int handleVideoRecorder();
};
}
#endif  // PHOTO_MODULE_H_
