/*
 *
 */

#ifndef NEG_H_
#define NEG_H_

#include <stdio.h>
#include <stdint.h>
#include <utils/RefBase.h>

namespace android {
    static int MAX_VER = (1 << 7) - 1;
    static int MAX_REASON = (1 << 7) - 1;
    static int FAIL_ADDRESS_MISMATCH = 1;
    static int FAIL_VERSION_MISMATCH = 2;

class Neg : public RefBase{
 public:
    Neg(){
	memset(mDatas,0,sizeof(mDatas));
    };

    static bool isACK2() {
	return (mDatas[0] & 0x40) == 1;
    }

      // all from ACK2 below-----------------------
    static bool isPass() {
	return isACK2() && (((mDatas[1] & 0xff) >> 7) == 1);
    }

    static int getReason() {
	return isACK2() ? (mDatas[1] & 0x7f) : -1;
    }
 private:
    uint8_t mDatas[2];
};
}  // namespace android

#endif  // NEG_H_

