#include "../../host_platform.hpp"
#define htobe64 OSSwapHostToBigInt64
#define htobe32 OSSwapHostToBigInt32
#define htobe16 OSSwapHostToBigInt16
#define letoh16 OSSwapLittleToHostInt16
#define htole16 OSSwapHostToLittleInt16
#define le32toh OSSwapLittleToHostInt32
