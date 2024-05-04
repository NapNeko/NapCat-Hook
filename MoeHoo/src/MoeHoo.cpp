#include <node_api.h>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <string>

#if defined(_WIN_PLATFORM_)
#elif defined(_LINUX_PLATFORM_)
#endif
namespace demo
{
	napi_value init(napi_env env, napi_value exports)
	{
		napi_status status;
		napi_value fn;

		napi_create_function(env, nullptr, 0, nullptr, nullptr, &fn);
		napi_set_named_property(env, exports, "CallSign", fn);
		napi_create_function(env, nullptr, 0, nullptr, nullptr, &fn);
		napi_set_named_property(env, exports, "InitSign", fn);
		return exports;
	}
	NAPI_MODULE(NODE_GYP_MODULE_NAME, init)
}
