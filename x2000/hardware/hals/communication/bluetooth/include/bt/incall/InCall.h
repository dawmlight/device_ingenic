#ifndef INCALL_H_
#define INCALL_H_

#include <stdio.h>
#include <utils/RefBase.h>

namespace android {
class InCall: public RefBase {
public:
    InCall();
    virtual ~InCall();

private:
    bool setState(bool isActive);
};
}  // namespace android

#endif  // INCALL_H_
