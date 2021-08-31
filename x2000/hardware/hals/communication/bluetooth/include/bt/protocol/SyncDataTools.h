
#ifndef SYNCDATATOOLS_H_
#define SYNCDATATOOLS_H_

#include <stdio.h>
#include "SyncData.h"
namespace android {
class SyncDataTools : public RefBase{
 public:
    static int data2Bytes(const sp<SyncData> & data,char* bytes);
    static int bytes2Data(sp<SyncData> & syncData,char* dataBytes,int size);

 private:
    class Decoder;
    class Encoder;

};
};
#endif  // SYNCDATATOOLS_H_
