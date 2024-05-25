#include "SystemInfo.h"

#ifdef __APPLE__
    #include <mach/mach.h>
    #include <sys/sysctl.h>
    #include <mach-o/dyld.h>
#elif defined __ANDROID__
    #include <sys/resource.h>
#elif defined WIN32
	#include <Psapi.h>
	#pragma comment(lib,"Psapi.lib")
#endif

#ifdef __linux__
#include <errno.h>
extern char *program_invocation_name;
#endif

NS_BASELIB_BEGIN

struct CPUOption
{
	int nCores;
	CPUOption()
	{
		nCores = 0;
	}
};

float GetDPI()
{
#ifdef __APPLE__
    return 120.0f;
#elif defined WIN32
    return 96.0f;
#elif defined __ANDROID__
    return 132.0f;
#endif
}

double GetCPUUserTime() 
{
//#if __TBB_WIN8UI_SUPPORT
//	return 0;
#if _WIN32
	FILETIME my_times[4];
	bool status = GetProcessTimes(GetCurrentProcess(), my_times, my_times+1, my_times+2, my_times+3)!=0;
	//ASSERT( status, NULL );
	LARGE_INTEGER usrtime;
	usrtime.LowPart = my_times[3].dwLowDateTime;
	usrtime.HighPart = my_times[3].dwHighDateTime;
	return double(usrtime.QuadPart)*1E-7;
#else
	// Generic UNIX, including __APPLE__

	// On Linux, there is no good way to get CPU usage info for the current process:
	//   getrusage(RUSAGE_SELF, ...) that is used now only returns info for the calling thread;
	//   getrusage(RUSAGE_CHILDREN, ...) only counts for finished children threads;
	//   tms_utime and tms_cutime got with times(struct tms*) are equivalent to the above items;
	//   finally, /proc/self/task/<task_id>/stat doesn't exist on older kernels
	//      and it isn't quite convenient to read it for every task_id.

	struct rusage resources;
	bool status = getrusage(RUSAGE_SELF, &resources)==0;
	return (double(resources.ru_utime.tv_sec)*1E6 + double(resources.ru_utime.tv_usec))*1E-6;
#endif
}

#ifdef __APPLE__

std::string GetExePath()
{
    char buf[256] = {0};
    uint32_t size = 0;
    _NSGetExecutablePath(buf,&size);
    char* path = (char*)malloc(size+1);
    path[size] = 0;
    _NSGetExecutablePath(path,&size);
    char* pCur = strrchr(path, '/');
    *pCur = 0;

    std::string strExePath = path;

    return strExePath;
}

std::string GetExeName()
{
    std::string strExePath = getprogname();
    return strExePath;
}

double GetAppMemory()
{
    //这个方法获得的和xcode不一样
    task_basic_info_64_data_t taskInfo;
    mach_msg_type_number_t infoCount = sizeof(taskInfo);
    kern_return_t kernReturn = task_info(mach_task_self(),
                                         TASK_BASIC_INFO_64,
                                         (task_info_t)&taskInfo,
                                         &infoCount);
    
    return (kernReturn != KERN_SUCCESS) ? 0 : taskInfo.resident_size / (1024.0 * 1024.0);
}

double GetAppCpuUsage()
{
    kern_return_t kr;
    task_info_data_t tinfo;
    mach_msg_type_number_t task_info_count;
    
    double fTotalCpu = 0;
    
    task_info_count = TASK_INFO_MAX;
    kr = task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)tinfo, &task_info_count);
    if (kr != KERN_SUCCESS)
    {
        return fTotalCpu;
    }
    
    task_basic_info_t      basic_info;
    thread_array_t         thread_list;
    mach_msg_type_number_t thread_count;
    
    thread_info_data_t     thinfo;
    mach_msg_type_number_t thread_info_count;
    
    thread_basic_info_t basic_info_th;
    uint32_t stat_thread = 0; // Mach threads
    
    basic_info = (task_basic_info_t)tinfo;
    
    // get threads in the task
    kr = task_threads(mach_task_self(), &thread_list, &thread_count);
    if (kr != KERN_SUCCESS)
    {
        return fTotalCpu;
    }
    
    if (thread_count > 0)
        stat_thread += thread_count;
        
    long tot_sec = 0;
    long tot_usec = 0;
    int j;
    
    for (j = 0; j < thread_count; j++)
    {
        thread_info_count = THREAD_INFO_MAX;
        kr = thread_info(thread_list[j], THREAD_BASIC_INFO,
                         (thread_info_t)thinfo, &thread_info_count);
        if (kr != KERN_SUCCESS)
        {
            return fTotalCpu;
        }
        
        basic_info_th = (thread_basic_info_t)thinfo;
        
        if (!(basic_info_th->flags & TH_FLAGS_IDLE))
        {
            tot_sec = tot_sec + basic_info_th->user_time.seconds + basic_info_th->system_time.seconds;
            tot_usec = tot_usec + basic_info_th->system_time.microseconds + basic_info_th->system_time.microseconds;
            fTotalCpu = fTotalCpu + basic_info_th->cpu_usage / (float)TH_USAGE_SCALE * 100.0;
        }
        
    } // for each thread
    
    kr = vm_deallocate(mach_task_self(), (vm_offset_t)thread_list, thread_count * sizeof(thread_t));
    assert(kr == KERN_SUCCESS);
    
    return fTotalCpu;
}

#elif defined __ANDROID__

std::string GetExePath()
{
    //linux下可以   ANDROID不行
    char sysfile[20] = "/proc/self/exe";
    int  namelen = 512;
    char selfname[namelen];
    memset( selfname, 0, namelen );
    
    if ( -1 != readlink( sysfile,
                        selfname,
                        namelen) )
    {
        printf( " self name = %s\n", selfname );
    }
    
    return std::string(selfname);
    
    
    //另一种方法
    const char*pszSdPath = getenv("SECONDARY_STORAGE");
    if (NULL == pszSdPath)
    {
        pszSdPath = getenv("EXTERNAL_STORAGE");
    }
    
    char szBuf[255];
    sprintf(szBuf, "/proc/%d/cmdline",getpid());
    const char* pszPath = szBuf;
    FILE* fp = fopen(pszPath, "r");
    if (fp)
    {
        fread(szBuf, 1, 255, fp);
        fclose(fp);
    }
    
    std::string strPath = pszSdPath;
    strPath += "/Android/data/";
    strPath += szBuf;
    return strPath;
}

std::string GetExeName()
{
#if defined(__APPLE__) || defined(__FreeBSD__)
    const char * appname = getprogname();
#elif defined(_GNU_SOURCE)
    //const char * appname = program_invocation_name;
#else
    const char * appname = "?";
#endif
    
    //return appname;
}

#elif defined WIN32

std::string GetExePath()
{
	char szFilePath[MAX_PATH + 1];
	memset(szFilePath,0,MAX_PATH + 1);
	GetModuleFileNameA(NULL, szFilePath, MAX_PATH + 1); 
	std::string str(szFilePath);
	return str;
}

std::string GetExeName()
{
	char szFilePath[MAX_PATH + 1];
	memset(szFilePath,0,MAX_PATH + 1);
	GetModuleFileNameA(NULL, szFilePath, MAX_PATH + 1); 
	std::string str(szFilePath);
	return str;
}

double GetAppMemory()
{  
	PROCESS_MEMORY_COUNTERS pmc;  
	GetProcessMemoryInfo(GetCurrentProcess(),&pmc,sizeof(pmc));

	return pmc.WorkingSetSize / (1024.0 * 1024.0);
}

/// 获得CPU的核数
static int get_processor_number()
{
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	return (int)info.dwNumberOfProcessors;
}

static unsigned long long FileTime2LongLong(const FILETIME* ftime)
{
	LARGE_INTEGER li;

	assert(ftime);
	li.LowPart = ftime->dwLowDateTime;
	li.HighPart = ftime->dwHighDateTime;
	return li.QuadPart;
}

double GetAppCpuUsage()
{
	//cpu数量
	static int processor_count_ = -1;
	//上一次的时间
	static long long last_time_ = 0;
	static long long last_system_time_ = 0;

	FILETIME now;
	FILETIME creation_time;
	FILETIME exit_time;
	FILETIME kernel_time;
	FILETIME user_time;
	long long system_time;
	long long time;
	long long system_time_delta;
	long long time_delta;

	int cpu = -1;

	if(processor_count_ == -1)
	{
		processor_count_ = get_processor_number();
	}

	GetSystemTimeAsFileTime(&now);

	if (!GetProcessTimes(GetCurrentProcess(), &creation_time, &exit_time, &kernel_time, &user_time))
	{
		// We don’t assert here because in some cases (such as in the TaskManager)
			// we may call this function on a process that has just exited but we have
			// not yet received the notification.
		return 0;
	}
	system_time = (FileTime2LongLong(&kernel_time) + FileTime2LongLong(&user_time)) / processor_count_;
	time = FileTime2LongLong(&now);

	if ((last_system_time_ == 0) || (last_time_ == 0))
	{
		// First call, just set the last values.
		last_system_time_ = system_time;
		last_time_ = time;
		return 0;
	}

	system_time_delta = system_time - last_system_time_;
	time_delta = time - last_time_;

	assert(time_delta != 0);

	if (time_delta == 0)
		return 0;

	// We add time_delta / 2 so the result is rounded.
	cpu = (int)((system_time_delta * 100 + time_delta * processor_count_) / time_delta);
	last_system_time_ = system_time;
	last_time_ = time;
	return cpu;
}

class CpuUsage
{
public:
	CpuUsage(DWORD dwProcessID);
	ULONGLONG GetUsageEx();
	ULONGLONG GetSystemNonIdleTimes();
	ULONGLONG GetProcessNonIdleTimes();
private:
	ULONGLONG SubtractTimes(const FILETIME& ftA, const FILETIME& ftB);
	ULONGLONG AddTimes(const FILETIME& ftA, const FILETIME& ftB);
	bool EnoughTimePassed();
	inline bool IsFirstRun() const { return (m_dwLastRun == 0); }

	//system total times
	FILETIME m_ftPrevSysKernel;
	FILETIME m_ftPrevSysUser;

	//process times
	FILETIME m_ftPrevProcKernel;
	FILETIME m_ftPrevProcUser;

	ULONGLONG m_ullPrevSysNonIdleTime;//这个变量和后面的便利记录上次获取的非idle的系统cpu时间和进程cpu时间.
	ULONGLONG m_ullPrevProcNonIdleTime;//这个类只绑定一个进程, 在构造函数里面初始化进来..

	ULONGLONG m_nCpuUsage;
	ULONGLONG m_dwLastRun;
	DWORD m_dwProcessID;
	HANDLE m_hProcess;
	volatile LONG m_lRunCount;
};

#include <windows.h>
#include <strsafe.h>
//#define USE_DEPRECATED_FUNCS
BOOL SetPrivilege(HANDLE hProcess, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege);

#ifdef USE_DEPRECATED_FUNCS
#define SystemBasicInformation                    0
#define SystemPerformanceInformation            2
#define SystemTimeInformation                    3
#define SystemProcessorPerformanceInformation    8
#define ProcessTimes                            4

#define Li2Double(x) ((double)((x).HighPart) * 4.294967296E9 + (double)((x).LowPart))

typedef struct
{
	DWORD dwUnknown1;
	ULONG uKeMaximumIncrement;
	ULONG uPageSize;
	ULONG uMmNumberOfPhysicalPages;
	ULONG uMmLowestPhysicalPage;
	ULONG uMmHighestPhysicalPage;
	ULONG uAllocationGranularity;
	PVOID pLowestUserAddress;
	PVOID pMmHighestUserAddress;
	ULONG uKeActiveProcessors;
	BYTE bKeNumberProcessors;
	BYTE bUnknown2;
	WORD wUnknown3;
} SYSTEM_BASIC_INFORMATION;

typedef struct
{
	LARGE_INTEGER liIdleTime;
	DWORD dwSpare[312];
} SYSTEM_PERFORMANCE_INFORMATION;

typedef struct
{
	LARGE_INTEGER liKeBootTime;
	LARGE_INTEGER liKeSystemTime;
	LARGE_INTEGER liExpTimeZoneBias;
	ULONG uCurrentTimeZoneId;
	DWORD dwReserved;
} SYSTEMTEXTIME_INFORMATION;

typedef struct
_SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION
{
	LARGE_INTEGER IdleTime;
	LARGE_INTEGER KernelTime;
	LARGE_INTEGER UserTime;
	LARGE_INTEGER Reserved1[2];
	ULONG Reserved2;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;

typedef struct _KERNEL_USERTEXTIMES
{
	LARGE_INTEGER CreateTime;
	LARGE_INTEGER ExitTime;
	LARGE_INTEGER KernelTime;
	LARGE_INTEGER UserTime;
} KERNEL_USERTEXTIMES, *PKERNEL_USERTEXTIMES;

typedef LONG (WINAPI *PROCNTQSI)(UINT, PVOID, ULONG, PULONG);
PROCNTQSI NtQuerySystemInformation;

typedef LONG (WINAPI *PROCNTQIP)(HANDLE, UINT, PVOID, ULONG, PULONG);
PROCNTQIP NtQueryInformationProcess;

ULONGLONG CpuUsage::GetSystemNonIdleTimes()
{
	SYSTEM_PERFORMANCE_INFORMATION SysPerfInfo;
	SYSTEMTEXTIME_INFORMATION SysTimeInfo;
	SYSTEM_BASIC_INFORMATION SysBaseInfo;
	SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION SysProcPerfInfo[32];
	LONG status;
	NtQuerySystemInformation = (PROCNTQSI)GetProcAddress(GetModuleHandle(TEXT("ntdll")), "NtQuerySystemInformation");
	if (!NtQuerySystemInformation)
		return 0;
	status = NtQuerySystemInformation(SystemBasicInformation, &SysBaseInfo, sizeof(SysBaseInfo), NULL);
	if (status != NO_ERROR)
	{
		return 0;
	}
	status = NtQuerySystemInformation(SystemProcessorPerformanceInformation, SysProcPerfInfo, sizeof(SysProcPerfInfo), NULL);
	if(status != NO_ERROR) return 0;
	int nProcessors = SysBaseInfo.bKeNumberProcessors; //机器内部CPU的个数
	ULONGLONG ullSysTotal = 0;
	for(int i = 0; i < nProcessors; i++)
	{
		ullSysTotal += SysProcPerfInfo[i].KernelTime.QuadPart + SysProcPerfInfo[i].UserTime.QuadPart;
	}
	return ullSysTotal;
}

ULONGLONG CpuUsage::GetProcessNonIdleTimes()
{
	KERNEL_USERTEXTIMES KernelUserTimes;
	::ZeroMemory(&KernelUserTimes, sizeof(KernelUserTimes));
	NtQueryInformationProcess = (PROCNTQIP)GetProcAddress(GetModuleHandle(TEXT("ntdll")), "NtQueryInformationProcess");
	LONG status = NtQueryInformationProcess(m_hProcess, ProcessTimes, &KernelUserTimes, sizeof(KernelUserTimes), NULL);
	if(status == 0)
	{
		return 0;
	}
	return KernelUserTimes.KernelTime.QuadPart + KernelUserTimes.UserTime.QuadPart;

}

#endif

CpuUsage::CpuUsage(DWORD dwProcessID)
: m_nCpuUsage(0),
m_dwLastRun(0),
m_lRunCount(0),
m_dwProcessID(dwProcessID),
m_ullPrevProcNonIdleTime(0),
m_ullPrevSysNonIdleTime(0)
{
	HANDLE hProcess = GetCurrentProcess();
	SetPrivilege(hProcess, SE_DEBUG_NAME, TRUE);

	m_hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION , TRUE, m_dwProcessID);
	if(m_hProcess == 0)
	{
	}
	ZeroMemory(&m_ftPrevSysKernel, sizeof(FILETIME));
	ZeroMemory(&m_ftPrevSysUser, sizeof(FILETIME));

	ZeroMemory(&m_ftPrevProcKernel, sizeof(FILETIME));
	ZeroMemory(&m_ftPrevProcUser, sizeof(FILETIME));
}


ULONGLONG CpuUsage::SubtractTimes(const FILETIME &ftA, const FILETIME &ftB)
{
	LARGE_INTEGER a, b;
	a.LowPart = ftA.dwLowDateTime;
	a.HighPart = ftA.dwHighDateTime;

	b.LowPart = ftB.dwLowDateTime;
	b.HighPart = ftB.dwHighDateTime;

	return a.QuadPart - b.QuadPart;
}

ULONGLONG CpuUsage::AddTimes(const FILETIME &ftA, const FILETIME &ftB)
{
	LARGE_INTEGER a, b;
	a.LowPart = ftA.dwLowDateTime;
	a.HighPart = ftA.dwHighDateTime;

	b.LowPart = ftB.dwLowDateTime;
	b.HighPart = ftB.dwHighDateTime;

	return a.QuadPart + b.QuadPart;
}

bool CpuUsage::EnoughTimePassed()
{
	const int minElapsedMS = 250;//milliseconds

	ULONGLONG dwCurrentTickCount = GetTickCount();
	return (dwCurrentTickCount - m_dwLastRun) > minElapsedMS;
}
#ifndef USE_DEPRECATED_FUNCS

ULONGLONG CpuUsage::GetSystemNonIdleTimes()
{
	FILETIME ftSysIdle, ftSysKernel, ftSysUser;
	if(!GetSystemTimes(&ftSysIdle, &ftSysKernel, &ftSysUser))
	{
		return 0;
	}
	return AddTimes(ftSysKernel, ftSysUser);
}


ULONGLONG CpuUsage::GetProcessNonIdleTimes()
{
	FILETIME ftProcCreation, ftProcExit, ftProcKernel, ftProcUser;
	if(!GetProcessTimes(m_hProcess, &ftProcCreation, &ftProcExit, &ftProcKernel, &ftProcUser) && false)
	{
		return 0;
	}
	return AddTimes(ftProcKernel, ftProcUser);
}
#endif

ULONGLONG CpuUsage::GetUsageEx()
{
	ULONGLONG nCpuCopy = m_nCpuUsage;
	if (::InterlockedIncrement(&m_lRunCount) == 1)
	{
		if (!EnoughTimePassed())
		{
			::InterlockedDecrement(&m_lRunCount);
			return nCpuCopy;
		}
		ULONGLONG ullSysNonIdleTime = GetSystemNonIdleTimes();
		ULONGLONG ullProcNonIdleTime = GetProcessNonIdleTimes();
		if (!IsFirstRun())
		{
			ULONGLONG ullTotalSys = ullSysNonIdleTime - m_ullPrevSysNonIdleTime;
			if(ullTotalSys == 0)
			{
				::InterlockedDecrement(&m_lRunCount);
				return nCpuCopy;
			}
			m_nCpuUsage = ULONGLONG((ullProcNonIdleTime - m_ullPrevProcNonIdleTime) * 100.0 / (ullTotalSys));
			m_ullPrevSysNonIdleTime = ullSysNonIdleTime;
			m_ullPrevProcNonIdleTime = ullProcNonIdleTime;
		}
		m_dwLastRun = (ULONGLONG)GetTickCount();
		nCpuCopy = m_nCpuUsage;
	}
	::InterlockedDecrement(&m_lRunCount);
	return nCpuCopy;
}

BOOL SetPrivilege(HANDLE hProcess, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege)
{
	HANDLE hToken;
	if(!OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken))
	{
		return FALSE;
	}
	LUID luid;
	if(!LookupPrivilegeValue(NULL, lpszPrivilege, &luid))
	{
		return FALSE;
	}
	TOKEN_PRIVILEGES tkp;
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = luid;
	tkp.Privileges[0].Attributes = (bEnablePrivilege) ? SE_PRIVILEGE_ENABLED : FALSE;
	if(!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL))
	{
		return FALSE;
	}
	return TRUE;
}

//double GetAppCpuUsage()
//{
//	CpuUsage cpu(GetCurrentProcessId());
//	return cpu.GetUsageEx();
//}

#endif

NS_BASELIB_END
