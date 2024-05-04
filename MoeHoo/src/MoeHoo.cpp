#include <node_api.h>
#include <map>
#include <iostream>
#include <string>

std::map<uint64_t, uint64_t> offsetList;

#if defined(_WIN_PLATFORM_)
auto InitOffsetsStatus = InitOffsets();
void *InitOffsets()
{
	offsetList[23361] = 0x0;
}

#elif defined(_LINUX_PLATFORM_)
#endif

namespace demo
{
	napi_value init(napi_env env, napi_value exports)
	{
		napi_value fn;
		napi_create_function(env, nullptr, 0, nullptr, nullptr, &fn);
		napi_set_named_property(env, exports, "RegHookRkey", fn);
		napi_create_function(env, nullptr, 0, nullptr, nullptr, &fn);
		napi_set_named_property(env, exports, "SendPacket", fn);
		return exports;
	}
	NAPI_MODULE(NODE_GYP_MODULE_NAME, init)
}
