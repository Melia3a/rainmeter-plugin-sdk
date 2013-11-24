#include <cstdio>
#include <cstring>

#include <Windows.h>
#include "../../API/RainmeterAPI.h"

// TODO:
//   1. MOD_NOREPEAT feature support
//   2. double press feature support
//   3. assert that the skin window thread is also these functions' caller

LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam){

	CWPSTRUCT *detail_ptr = (CWPSTRUCT *)lParam;
	if (detail_ptr->message == WM_KEYDOWN)
		MessageBox(NULL, L"Fired by rainmeter", L"Test.dll", MB_OK);

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

class rm_measure_data{

private:

	WCHAR		log_message[1024];

	HWND		skin_window;
	DWORD		skin_window_thread_id;

	unsigned	modifiers, vk_code;
	int			key_alt, key_ctrl, key_shift, key_win;

	WCHAR		atom_string[256];
	ATOM		atom_value;

	BOOL		hot_key_registered;

	HHOOK		hook;

public:

	rm_measure_data(void *rm):
	  modifiers(0U), atom_value(0), hot_key_registered(false)
	{
		// get skin window handle
		skin_window = RmGetSkinWindow(rm);
		// get thread id
		skin_window_thread_id = GetWindowThreadProcessId(skin_window, NULL);
		// read hot key setting
		LPCWSTR key_name = RmReadString(rm, L"Key", L"");
		vk_code = translate_key_code(key_name);
		if (vk_code == 0){
			wsprintf(log_message, L"Test.dll: Invalid key value: %s", key_name);
			RmLog(LOG_ERROR, log_message);
			return;
		}
		// read modifiers setting
		key_alt = RmReadInt(rm, L"Alt", 0);
		key_ctrl = RmReadInt(rm, L"Ctrl", 0);
		key_shift = RmReadInt(rm, L"Shift", 0);
		key_win = RmReadInt(rm, L"Win", 0);
		// make modifiers
		if (key_alt)	modifiers |= MOD_ALT;
		if (key_ctrl)	modifiers |= MOD_CONTROL;
		if (key_shift)	modifiers |= MOD_SHIFT;
		if (key_win)	modifiers |= MOD_WIN;
		// for debug
		wsprintf(log_message, L"Test.dll: confirm hot key: ");
		if (key_alt) wsprintf(log_message+wcslen(log_message), L"ALT+");
		if (key_ctrl) wsprintf(log_message+wcslen(log_message), L"CTRL+");
		if (key_shift) wsprintf(log_message+wcslen(log_message), L"SHIFT+");
		if (key_win) wsprintf(log_message+wcslen(log_message), L"WIN+");
		wsprintf(log_message+wcslen(log_message), L"KEY(code:%x)", vk_code);
		RmLog(LOG_DEBUG, log_message);
		// get atom
		LPCWSTR skin_name = RmGetSkinName(rm);
		wcscpy_s(atom_string, skin_name);
		LPCWSTR measure_name = RmGetMeasureName(rm);
		wcscat_s(atom_string, L"\\");
		wcscat_s(atom_string, measure_name);
		if (GlobalFindAtom(atom_string) != 0){
			wsprintf(log_message, L"Test.dll: found global atom: %s", atom_string);
			RmLog(LOG_ERROR, log_message);
			return;
		}
		atom_value = GlobalAddAtom(atom_string);
		wsprintf(log_message, L"Test.dll: add global atom: %s: ", atom_string);
		if (atom_value < 0xC000){
			wsprintf(log_message+wcslen(log_message), L"failed");
			RmLog(LOG_ERROR, log_message);
			return;
		}
		wsprintf(log_message+wcslen(log_message), L"succeed");
		RmLog(LOG_DEBUG, log_message);
		// register hot key
		hot_key_registered = RegisterHotKey(skin_window, atom_value, modifiers, vk_code);
		if (!hot_key_registered){
			RmLog(LOG_ERROR, L"Test.dll: register hot key: failed");
			return;
		}
		RmLog(LOG_DEBUG, L"Test.dll: register hot key: succeed");
		//
		hook = SetWindowsHookEx(WH_CALLWNDPROC, HookProc, NULL, skin_window_thread_id);
		if (hook == NULL){
			RmLog(LOG_ERROR, L"Test.dll: set message hook: failed");
			return;
		}
		RmLog(LOG_DEBUG, L"Test.dll: set message hook: succeed");
	}

	~rm_measure_data(){
		// 
		if (hook != NULL)
			UnhookWindowsHookEx(hook);
		// unregister hot key
		if (hot_key_registered)
			UnregisterHotKey(skin_window, atom_value);
		// delete atom
		if (atom_value != 0)
			GlobalDeleteAtom(atom_value);
	}

private:

	// return a Virtual-Key Code
	static unsigned translate_key_code(LPCWSTR wstr){
		unsigned result = 0U;
		// get length
		size_t		len = wcslen(wstr);
		if (len == 0) return result;
		// copy string
		WCHAR  *tmp = new WCHAR[len+1];
		for (size_t i = 0; i <= len; ++ i){
			if (wstr[i] >= L'a' && wstr[i] <= L'z')
				tmp[i] = wstr[i]&WCHAR(0xDF);		// use capital case
			else
				tmp[i] = wstr[i];
		}
		// translate
		if (len == 1){
			// a number
			if (tmp[0] >= L'0' && tmp[0] <= L'9' ||
				tmp[0] >= L'A' && tmp[0] <= L'Z'){
					result = tmp[0];
			}
		}
		else{
			if (wcscmp(tmp, L"LEFT") ==0)
				result = 0x25U;
			else if (wcscmp(tmp, L"UP") == 0)
				result = 0x26U;
			else if (wcscmp(tmp, L"RIGHT") == 0)
				result = 0x27U;
			else if (wcscmp(tmp, L"DOWN") == 0)
				result = 0x28U;
		}

		delete[] tmp;

		return result;
	}

public:

	static void release_data(void *data){
		delete reinterpret_cast<rm_measure_data *>(data);
	}

};

// call after memory object "rm" created(launch/refresh skin: read local .ini file)
PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	// create data
	*data = new rm_measure_data(rm);
}

// load something from memory object "rm"
PLUGIN_EXPORT void Reload(void *, void *, double *){}

// update data
PLUGIN_EXPORT double Update(void *){ return 0.0; }

// 
PLUGIN_EXPORT void Finalize(void *data)
{
	rm_measure_data::release_data(data);
}
