#include <Windows.h>
#include <tlhelp32.h>
#include <string.h>
#include <Psapi.h>

#include "../../API/RainmeterAPI.h"

#pragma comment(lib,"Psapi.lib")

// Overview: This is a blank canvas on which to build your plugin.

// Note: GetString and ExecuteBang have been commented out. If you need
// GetString and/or ExecuteBang and you have read what they are used for
// from the SDK docs, uncomment the function(s). Otherwise leave them
// commented out (or get rid of them)!

struct Measure
{
	WCHAR str_A[260];
};

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	Measure* measure = new Measure;
	*data = measure;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	Measure* measure = (Measure*)data;

	LPCWSTR value = RmReadString(rm, L"Process", L"");

	memset(measure->str_A, 0, sizeof(measure->str_A) );

	wcscpy_s(measure->str_A , value) ;

}

PLUGIN_EXPORT double Update(void* data)
{

	Measure* measure = (Measure*)data;

	PROCESSENTRY32 pe32;
	PROCESS_MEMORY_COUNTERS pmc;

	pe32.dwSize=sizeof(pe32);

	HANDLE hProcessSnap=::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);

	if(hProcessSnap==INVALID_HANDLE_VALUE)
	{
		return 0.0;
	}

	BOOL bMore=::Process32First(hProcessSnap,&pe32);

	while (bMore)
	{
		if (wcscmp(pe32.szExeFile, measure->str_A) == 0)
		{

			HANDLE h_Process=OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);

			if (h_Process == NULL){
				RmLog(LOG_ERROR, L"failed to open process");
				break;
			}

			GetProcessMemoryInfo(h_Process,&pmc,sizeof(pmc));

			CloseHandle(h_Process);

			return pmc.WorkingSetSize ;
		}

		bMore=::Process32Next(hProcessSnap,&pe32);

	}
	return 0.0;
}

//PLUGIN_EXPORT LPCWSTR GetString(void* data)
//{
//	Measure* measure = (Measure*)data;
//	return L"";
//}

//PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args)
//{
//	Measure* measure = (Measure*)data;
//}

PLUGIN_EXPORT void Finalize(void* data)
{
	Measure* measure = (Measure*)data;
	delete measure;
}
