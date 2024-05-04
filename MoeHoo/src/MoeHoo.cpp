#include <node_api.h>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <string>

#if defined(_WIN_PLATFORM_)
#include <Windows.h>
#include <psapi.h>
#elif defined(_LINUX_PLATFORM_)
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>
#include <proc_maps.h>
#endif
// #include <libloaderapi.h>

typedef int (*SignFunctionType)(const char *cmd, const char *src, size_t src_len, int seq, char *result);
SignFunctionType SignFunction = nullptr;
// 签名函数定义

// Offsets写死了
int signOffsets = 767;
int ExtraOffsets = 511;
int TokenOffsets = 255;

void *InitSignCall()
{
	uint64_t wrapperModuleAddress = 0;
#if defined(_WIN_PLATFORM_)
	HMODULE wrapperModule = GetModuleHandleW(L"wrapper.node");
	MODULEINFO modInfo;
	if (wrapperModule == NULL || !GetModuleInformation(GetCurrentProcess(), wrapperModule, &modInfo, sizeof(MODULEINFO)))
	{
		return nullptr;
	}
	uint64_t HookAddress = reinterpret_cast<uint64_t>(wrapperModule) + 0x2E0D0;
#elif defined(_LINUX_PLATFORM_)
// 未测试
	auto pmap = hak::get_maps();
	do
	{
		if (pmap->module_name.find("wrapper.node") != std::string::npos && pmap->executable && pmap->readable)
		{
			wrapperModuleAddress = pmap->start();
			break;
		}
	} while ((pmap = pmap->next()) != nullptr);
	uint64_t HookAddress = wrapperModuleAddress + 0x33C3920;
#endif
	SignFunction = reinterpret_cast<SignFunctionType>(HookAddress);
	return nullptr;
}
auto InitStatus = InitSignCall();
// 将指定长度的字节转换为十六进制文本
std::string Sign_bytesToHex(const char *ptr, size_t length)
{
	std::ostringstream oss;
	for (size_t i = 0; i < length; ++i)
	{
		oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(ptr[i]));
	}
	return oss.str();
}
namespace demo
{
	napi_value InitSign(napi_env env, napi_callback_info args)
	{
		napi_value napiRet;
		size_t argc = 3;
		napi_value argv[3] = {nullptr};
		napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);
		if (argc < 3)
		{
			napi_create_int32(env, -1, &napiRet);
			return napiRet;
		}
		napi_get_value_int32(env, argv[0], &signOffsets);
		napi_get_value_int32(env, argv[1], &ExtraOffsets);
		napi_get_value_int32(env, argv[2], &TokenOffsets);
		napi_create_int32(env, 0, &napiRet);
		return napiRet;
	}
	napi_value CallSign(napi_env env, napi_callback_info args)
	{
		// 本函数返回值
		napi_value napiRet;
		napi_status retStatus;
		// -返回值内容
		napi_value signDataHexRetPtr;
		napi_value extraDataHexRetPtr;
		napi_value tokenDataHexRetPtr;
		// 本函数参数
		size_t argc = 4;
		napi_value argv[4] = {nullptr};
		// 初始化参数
		char *signArgCmd = new char[1024];
		char *signArgSrc = new char[1024];
		int32_t signSrc = 0;
		int32_t signSeq = 0;
		char *signResult = new char[1024];
		// 设置最大长度
		size_t str_size = 1024;
		// 初始化JS端数据
		napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

		// if (argc < 5)
		// {
		// 	napi_create_int32(env, -1, &napiRet);
		// 	return napiRet;
		// }

		// 参数从Js获取
		napi_get_value_string_utf8(env, argv[0], signArgCmd, str_size, &str_size);
		napi_get_value_string_utf8(env, argv[1], signArgSrc, str_size, &str_size);
		napi_get_value_int32(env, argv[2], &signSrc);
		napi_get_value_int32(env, argv[3], &signSeq);
		// 调用Sign
		if (SignFunction == nullptr)
		{
			delete[] signArgCmd;
			delete[] signArgSrc;
			delete[] signResult;
			// 初始化失败返回
			napi_create_string_utf8(env, "", NAPI_AUTO_LENGTH, &signDataHexRetPtr);
			napi_create_string_utf8(env, "", NAPI_AUTO_LENGTH, &extraDataHexRetPtr);
			napi_create_string_utf8(env, "", NAPI_AUTO_LENGTH, &tokenDataHexRetPtr);
			napi_create_object(env, &napiRet);
			napi_set_named_property(env, napiRet, "signDataHex", signDataHexRetPtr);
			napi_set_named_property(env, napiRet, "extarDataHex", extraDataHexRetPtr);
			napi_set_named_property(env, napiRet, "tokenDataHex", tokenDataHexRetPtr);
			return napiRet;
		}
		SignFunction(signArgCmd, signArgSrc, signSrc, signSeq, signResult);
		// 获取大小
		uint8_t *signSize = (uint8_t *)signResult + signOffsets;
		uint8_t *extraSize = (uint8_t *)signResult + ExtraOffsets;
		uint8_t *tokenSize = (uint8_t *)signResult + TokenOffsets;
		// 读取
		uint32_t signSizeU32 = *signSize;
		uint32_t extraSizeU32 = *extraSize;
		uint32_t tokenSizeU32 = *tokenSize;
		char *signData = signResult + 512;
		char *extraData = signResult + 256;
		char *tokenData = signResult;
		std::string signDataHex = Sign_bytesToHex(signData, signSizeU32);
		std::string extraDataHex = Sign_bytesToHex(extraData, extraSizeU32);
		std::string tokenDataHex = Sign_bytesToHex(tokenData, tokenSizeU32);
		napi_create_string_utf8(env, signDataHex.c_str(), NAPI_AUTO_LENGTH, &signDataHexRetPtr);
		napi_create_string_utf8(env, extraDataHex.c_str(), NAPI_AUTO_LENGTH, &extraDataHexRetPtr);
		napi_create_string_utf8(env, tokenDataHex.c_str(), NAPI_AUTO_LENGTH, &tokenDataHexRetPtr);
		// 读取完成创建返回数据
		napi_create_object(env, &napiRet);
		napi_set_named_property(env, napiRet, "signDataHex", signDataHexRetPtr);
		napi_set_named_property(env, napiRet, "extarDataHex", extraDataHexRetPtr);
		napi_set_named_property(env, napiRet, "tokenDataHex", tokenDataHexRetPtr);
		// 回收资源
		delete[] signArgCmd;
		delete[] signArgSrc;
		delete[] signResult;
		return napiRet;
	}
	napi_value init(napi_env env, napi_value exports)
	{
		napi_status status;
		napi_value fn;

		napi_create_function(env, nullptr, 0, CallSign, nullptr, &fn);
		napi_set_named_property(env, exports, "CallSign", fn);
		napi_create_function(env, nullptr, 0, InitSign, nullptr, &fn);
		napi_set_named_property(env, exports, "InitSign", fn);
		return exports;
	}
	NAPI_MODULE(NODE_GYP_MODULE_NAME, init)
}
