#ifndef DB_HELPER_H_
#define DB_HELPER_H_

#include <stdio.h>
#include <utils/RefBase.h>

namespace android {

typedef struct
{
	bool bind_state; /* Bind state */
	char bind_addr[130]; /* Remote Device Addr */
} t_DB_CONFIG;

class DBHelper : public RefBase{
public :
     int writeStateToDB(bool bind_state, const char* bind_addr);
     int readStateFromDB(t_DB_CONFIG & xml_config);
private :
     bool checkDatabaseDir();
};
} // namespace android 

#endif //DB_HELPER_H_

