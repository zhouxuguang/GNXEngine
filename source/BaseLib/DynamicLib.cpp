#include "DynamicLib.h"

NS_BASELIB_BEGIN

#if defined _WIN32 || defined _WIN64
#include <Windows.h>
	void *LoadLib(const char* strPath)
	{
		return LoadLibraryA(strPath);
	}

	void *GetSym(void *hLib,const char* strName)
	{
		return (GetProcAddress((HMODULE)hLib,strName));
	}

	void FreeLib(void *hLib)
	{
		FreeLibrary((HMODULE)hLib);
	}

#elif defined __linux__ || ( defined(__MACH__) && defined(__APPLE__) )
#include <dlfcn.h>
	void *LoadLib(const char* strPath)
	{
		return dlopen(strPath,RTLD_LAZY);
	}

	void *GetSym(void *hLib,const char* strName)
	{
		return dlsym(hLib,strName);
	}

	void FreeLib(void *hLib)
	{
		dlclose(hLib);
	}

#endif

NS_BASELIB_END
