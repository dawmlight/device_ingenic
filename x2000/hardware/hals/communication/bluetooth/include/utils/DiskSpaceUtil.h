#ifndef DISK_SPACE_UTIL_H_

#define DISK_SPACE_UTIL_H_

namespace android {

class DiskSpaceUtil {
public:
	static bool dataDiskFull(int reserve_size_mb);
	static int64_t dataDiskFree();
	static int64_t dataTotalSize();
	static int64_t dataUsedSize();
};

}  // namespace android

#endif  // DISK_SPACE_UTIL_H_
