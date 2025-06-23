#include "EnvironmentUtility.h"
#include <sstream>

#ifdef  __APPLE__
#include <sys/sysctl.h>
#endif

NS_BASELIB_BEGIN

#if defined(_WIN32) && !defined(__CYGWIN__)
#  define ENVIRONMENT_UTILITY_UNIX 0
#  include <direct.h>
#else
#  define ENVIRONMENT_UTILITY_UNIX 1
#endif

std::string EnvironmentUtility::GetEnvironmentVariable(const std::string& variable) const
{
	std::string result;
	char* lookup = getenv(variable.c_str());
	if (lookup != NULL)
	{
		result = (const char*)lookup;
	}
	return result;
}

std::string EnvironmentUtility::GetUserDir() const
{
	std::string result;

#if ENVIRONMENT_UTILITY_UNIX
	result = GetEnvironmentVariable("HOME");
#else
	result = GetEnvironmentVariable("USERPROFILE");
#endif

	return result;
}

std::string EnvironmentUtility::GetUserName() const
{
#if ENVIRONMENT_UTILITY_UNIX
	return GetEnvironmentVariable("USER");
#else
	return GetEnvironmentVariable("USERNAME");
#endif
}

std::string EnvironmentUtility::GetCurrentWorkingDir() const
{
	std::string result;

#if ENVIRONMENT_UTILITY_UNIX
	//result = GetEnvironmentVariable("PWD");
    char buffer[PATH_MAX] = {0};
    if (getcwd(buffer, sizeof(buffer)))
    {
        result = std::string(buffer);
    }
    else
    {
        // 错误处理
        //perror("getcwd() error");
        return "";
    }
#else
	char buf[512];
	_getcwd(buf, 512);
	result = buf;
#endif

	return result;
}

bool EnvironmentUtility::SetCurrentDir(const char* path)
{
#ifdef _WIN32
	return SetCurrentDirectoryA(path);
#else
	return chdir(path) == 0;
#endif
}

int EnvironmentUtility::GetProcessorCount() const
{
#if defined(WIN32)
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
#elif defined(__linux__)
	int count = sysconf(_SC_NPROCESSORS_ONLN);   //这个是返回当前可用的核心数目，我需要返回总的核心数
	if (count <= 0) count = 1;
	return count;

#elif __open_bsd__
	int mib[2] = { CTL_HW, HW_NCPU };
	int mib[2];
	mib[0] = CTL_HW;
	size_t length = 2;
	if (sysctlnametomib("hw.logicalcpu", mib, &length) == -1) 
	{
		return 2;
	}
#else
	int nCores = 0;
	size_t size = sizeof(nCores);

	/* get the number of CPUs from the system */
	if (sysctlbyname("hw.ncpu", &nCores, &size, NULL, 0) == -1)
	{
		return 2;
	}
	return nCores;
#endif
}

#ifdef WIN32

#pragma warning(disable: 4996)

#define INVALID_VERSION -1
typedef enum _NTOS_VERSION {
	NTOS_UNKNOWN,
	NTOS_WINXP,
	NTOS_WINXPSP1,
	NTOS_WINXPSP2,
	NTOS_WINXPSP3,
	NTOS_WIN2003,
	NTOS_WIN2003SP1,
	NTOS_WIN2003SP2,
	NTOS_WINVISTA,
	NTOS_WINVISTASP1,
	NTOS_WINVISTASP2,
	NTOS_WIN7,
	NTOS_WIN7SP1,
	NTOS_WIN8,
	NTOS_WIN81,
	NTOS_WIN10_1507, //10240
	NTOS_WIN10_1511, //10586
	NTOS_WIN10_1607, //14393
	NTOS_WIN10_1703, //15063
	NTOS_WIN10_1709, //16299
	NTOS_WIN10_1803, //17134
	NTOS_WIN10_1809, //17763
	NTOS_WIN10_1903, //18362
	NTOS_WIN10_1909, //18363
	NTOS_WIN10_2004, //19041
	NTOS_WIN10_20H2, //19042
	NTOS_WIN10_21H1, //19043
	NTOS_WIN10_21H2, //19044
	NTOS_WIN10_22H2, //19045
	NTOS_WIN11_21H2, //22000
	NTOS_WIN11_22H2, //22621
	NTOS_WIN_MAX, //22621
} NTOS_VERSION, * PNTOS_VERSION;

typedef struct _OSVERSIONINFO
{
	DWORD dwOSVersionInfoSize;
	DWORD dwMajorVersion;
	DWORD dwMinorVersion;
	DWORD dwBuildNumber;
	DWORD dwPlatformId;
	TCHAR szCSDVersion[128];
} RtlOSVERSIONINFO;

typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(RtlOSVERSIONINFO*);

NTOS_VERSION OsNtVersion()
{
	RtlOSVERSIONINFO info = { sizeof(RtlOSVERSIONINFO) };
	HMODULE hNtdll = GetModuleHandle("ntdll.dll");
	if (nullptr == hNtdll) 
	{
		return NTOS_UNKNOWN;
	}

	RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hNtdll, "RtlGetVersion");
	if (RtlGetVersion) 
	{
		RtlGetVersion(&info);
		//printf("Version: %d.%d.%d\n", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
	}

	/*switch (info.dwMajorVersion) {
	case 5: {
		if (info.dwMinorVersion == 1) {
			if (info.wServicePackMajor == 1) return NTOS_WINXPSP1;
			if (info.wServicePackMajor == 2) return NTOS_WINXPSP2;
			if (info.wServicePackMajor == 3) return NTOS_WINXPSP3;
			return NTOS_WINXP;
		}
		if (info.dwMinorVersion == 2) {
			if (info.wServicePackMajor == 1) return NTOS_WIN2003SP1;
			if (info.wServicePackMajor == 2) return NTOS_WIN2003SP2;
			return NTOS_WIN2003;
		}
		break;
	} case 6: {
		if (info.dwMinorVersion == 0) {
			if (info.wServicePackMajor == 1) return NTOS_WINVISTASP1;
			if (info.wServicePackMajor == 2) return NTOS_WINVISTASP2;
			return NTOS_WINVISTA;
		}
		if (info.dwMinorVersion == 1) {
			if (info.wServicePackMajor == 1) return NTOS_WIN7SP1;
			return NTOS_WIN7;
		}
		if (info.dwMinorVersion == 2) {
			return NTOS_WIN8;
		}
		if (info.dwMinorVersion == 3) {
			return NTOS_WIN81;
		}
		break;
	} case 10: {
		if (info.dwBuildNumber == 10240) return NTOS_WIN10_1507;
		if (info.dwBuildNumber == 10586) return NTOS_WIN10_1511;
		if (info.dwBuildNumber == 14393) return NTOS_WIN10_1607;
		if (info.dwBuildNumber == 15063) return NTOS_WIN10_1703;
		if (info.dwBuildNumber == 16299) return NTOS_WIN10_1709;
		if (info.dwBuildNumber == 17134) return NTOS_WIN10_1803;
		if (info.dwBuildNumber == 17763) return NTOS_WIN10_1809;
		if (info.dwBuildNumber == 18362) return NTOS_WIN10_1903;
		if (info.dwBuildNumber == 18363) return NTOS_WIN10_1909;
		if (info.dwBuildNumber == 19041) return NTOS_WIN10_2004;
		if (info.dwBuildNumber == 19042) return NTOS_WIN10_20H2;
		if (info.dwBuildNumber == 19043) return NTOS_WIN10_21H1;
		if (info.dwBuildNumber == 19044) return NTOS_WIN10_21H2;
		if (info.dwBuildNumber == 19045) return NTOS_WIN10_22H2;
		if (info.dwBuildNumber == 22000) return NTOS_WIN11_21H2;
		if (info.dwBuildNumber == 22621) return NTOS_WIN11_22H2;
		if (info.dwBuildNumber > 22621) return NTOS_WIN_MAX;
	}
	default:
		break;
	}*/
	return NTOS_UNKNOWN;
}

std::string EnvironmentUtility::GetSystemName() const
{
	OSVERSIONINFO vi;
	vi.dwOSVersionInfoSize = sizeof(vi);
	if (GetVersionEx(&vi) == 0) return("Unknown");
	switch (vi.dwPlatformId)
	{
	case VER_PLATFORM_WIN32s:
		return "Windows 3.x";
	case VER_PLATFORM_WIN32_WINDOWS:
		return vi.dwMinorVersion == 0 ? "Windows 95" : "Windows 98";
	case VER_PLATFORM_WIN32_NT:
		return "Windows NT";
	default:
		return "Unknown";
	}
}

std::string EnvironmentUtility::GetDisplayName() const
{
	OsNtVersion();
	OSVERSIONINFOEX vi;	// OSVERSIONINFOEX is supported starting at Windows 2000 
	vi.dwOSVersionInfoSize = sizeof(vi);
	if (GetVersionEx((OSVERSIONINFO*) &vi) == 0) return("Unknown");
	switch (vi.dwMajorVersion)
	{
	case 10:
		switch (vi.dwMinorVersion)
		{
		case 0:
			return vi.wProductType == VER_NT_WORKSTATION ? "Windows 10" : "Windows Server 2016";
		}
	case 6:
		switch (vi.dwMinorVersion)
		{
		case 0:
			return vi.wProductType == VER_NT_WORKSTATION ? "Windows Vista" : "Windows Server 2008";
		case 1:
			return vi.wProductType == VER_NT_WORKSTATION ? "Windows 7" : "Windows Server 2008 R2";
		case 2:
			return vi.wProductType == VER_NT_WORKSTATION ? "Windows 8" : "Windows Server 2012";
		case 3:
			return vi.wProductType == VER_NT_WORKSTATION ? "Windows 8.1" : "Windows Server 2012 R2";
		default:
			return "Unknown";
		}
	case 5:
		switch (vi.dwMinorVersion)
		{
		case 0:
			return "Windows 2000";
		case 1:
			return "Windows XP";
		case 2:
			return "Windows Server 2003/Windows Server 2003 R2";
		default:
			return "Unknown";
		}
	default:
		return "Unknown";
	}
}

std::string EnvironmentUtility::GetNodeName() const
{
	char name[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD size = sizeof(name);
	if (GetComputerNameA(name, &size) == 0) return ("Unkown");
	return std::string(name);
}

std::string EnvironmentUtility::GetArchitecture() const
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	switch (si.wProcessorArchitecture)
	{
	case PROCESSOR_ARCHITECTURE_INTEL:
		return "IA32";
	case PROCESSOR_ARCHITECTURE_MIPS:
		return "MIPS";
	case PROCESSOR_ARCHITECTURE_ALPHA:
		return "ALPHA";
	case PROCESSOR_ARCHITECTURE_PPC:
		return "PPC";
	case PROCESSOR_ARCHITECTURE_IA64:
		return "IA64";
#ifdef PROCESSOR_ARCHITECTURE_IA32_ON_WIN64
	case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
		return "IA64/32";
#endif
#ifdef PROCESSOR_ARCHITECTURE_AMD64
	case PROCESSOR_ARCHITECTURE_AMD64:
		return "AMD64";
#endif
	default:
		return "Unknown";
	}
}

std::string EnvironmentUtility::GetSystemVersion() const
{
	OSVERSIONINFO vi;
	vi.dwOSVersionInfoSize = sizeof(vi);
	if (GetVersionEx(&vi) == 0) return("Unkown");
	std::ostringstream str;
	str << vi.dwMajorVersion << "." << vi.dwMinorVersion << " (Build " << (vi.dwBuildNumber & 0xFFFF);
	if (vi.szCSDVersion[0]) str << ": " << vi.szCSDVersion;
	str << ")";
	return str.str();
}

#else

#include <sys/utsname.h>

std::string EnvironmentUtility::GetSystemName() const
{
	struct utsname uts;
	uname(&uts);
	return uts.sysname;
}

std::string EnvironmentUtility::GetDisplayName() const
{
	return GetSystemName();
}

std::string EnvironmentUtility::GetNodeName() const
{
	struct utsname uts;
	uname(&uts);
	return uts.nodename;
}

std::string EnvironmentUtility::GetArchitecture() const
{
	struct utsname uts;
	uname(&uts);
	return uts.machine;
}

std::string EnvironmentUtility::GetSystemVersion() const
{
	struct utsname uts;
	uname(&uts);
	return uts.release;
}

#endif

NS_BASELIB_END
