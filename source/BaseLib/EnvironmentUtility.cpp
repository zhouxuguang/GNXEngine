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

//EnvironmentUtility* EnvironmentUtility::m_pInstance = NULL;
//
//EnvironmentUtility* EnvironmentUtility::GetInstance()
//{
//	if(NULL == m_pInstance)
//	{
//		m_pInstance = new EnvironmentUtility;
//	}
//
//	return m_pInstance;
//}

//EnvironmentUtility::EnvironmentUtility(void)
//{
//}
//
//EnvironmentUtility::~EnvironmentUtility(void)
//{
//}

std::string EnvironmentUtility::GetEnvironmentVariable(const std::string& variable) const
{
	std::string result;
	char* lookup = getenv(variable.c_str());
	// 鈭毬€濃€撀捖碘垰鈭懧德縉ULL.
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
	result = GetEnvironmentVariable("PWD");
#else
	char buf[512];
	_getcwd(buf, 512);
	result = buf;
#endif

	return result;
}

int EnvironmentUtility::GetProcessorCount() const
{
#if defined(WIN32)
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
#elif defined(__linux__)
	int count = sysconf(_SC_NPROCESSORS_ONLN);
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
    
    //iiOS涓婅幏寰楃殑鏈哄櫒鐨勭被鍨嬪悕绉帮紝鍙互纭畾iPhone鎴栬€卛pad鐨勫悕绉?
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
