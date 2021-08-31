#ifndef NET_UTIL_H_
#define NET_UTIL_H_
#include <string>
using namespace std;

namespace android {
class NetUtil {
 public:
	static bool isNetAvailable();

	static int getData(string host, int port, string path, string get_content,char* buf);
 private:
	static int socketHttp(string host, int port, string request, char* buf);
};
}
#endif  // NET_UTIL_H_
