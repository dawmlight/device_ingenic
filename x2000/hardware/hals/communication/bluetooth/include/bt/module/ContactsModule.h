#ifndef CONTACTS_MODULE_H_
#define CONTACTS_MODULE_H_

#include <stdio.h>
#include <protocol/SyncModule.h>

namespace android {
class ContactsModule : public SyncModule {
 public:
     static ContactsModule* getInstance(RefBase* protocol);
     virtual ~ContactsModule();
 private:
    ContactsModule(RefBase* protocol);
    static ContactsModule* sInstance;
    void onRetrive(const sp<SyncData> & data);
};
}
#endif  // CONTACTS_MODULE_H_
