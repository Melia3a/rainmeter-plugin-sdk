#include <cstdio>
#include <cstring>

#include <map>
using std::pair;
using std::map;
#include <list>
using std::list;

#include <Windows.h>

#include "../../API/RainmeterAPI.h"

// this is used to map a hot key combination to a rm_measure object so that
// various callbacks (objects' member functions) can be called by HookProc
typedef map<unsigned long, void *>	hk_map;
typedef pair<unsigned long, void *>	hk_map_pair;

// simple lock using busy waiting
static unsigned	hot_keys_lock = FALSE;
// map each registered HotKey to a rm_measure object
static hk_map	hot_keys;

// hook callback
static LRESULT CALLBACK HookProc(int, WPARAM, LPARAM);

// a hook with reference count
class thread_hook{

public:

	// map a thread id to a thread_hook object
	typedef map<DWORD, thread_hook>		id_map;
	typedef pair<DWORD, thread_hook>	id_map_pair;

private:

	HHOOK		sole_hook;	// real hook handle
	unsigned	ref_count;	// reference count

public:

	thread_hook(HHOOK hk):
	  sole_hook(hk), ref_count(1U)
	{
		// ...
	}

public:

	static unsigned thread_ids_lock;
	static id_map	thread_ids;

public:

	static bool add_hook(const DWORD &thread_id){
		// get lock by busy waiting
		while (InterlockedExchange(&thread_ids_lock, TRUE) != FALSE);
		// find thread ID
		bool result;
		id_map::iterator it = thread_ids.find(thread_id);
		// set hook
		if (it == thread_ids.end()){
			HHOOK hk = SetWindowsHookEx(WH_GETMESSAGE, HookProc, NULL, thread_id);
			if (hk == NULL){
				RmLog(LOG_ERROR, L"GlobalHotKey.dll: set hook: failed");
				result = false;
			}
			else{
				thread_ids.insert(id_map_pair(thread_id, thread_hook(hk)));
				result = true;
			}
		}
		// increase reference count
		else{
			++ (*it).second.ref_count;
			result = true;
		}
		// release lock
		InterlockedExchange(&thread_ids_lock, FALSE);

		return result;
	}

	static void del_hook(const DWORD &thread_id){
		// get lock by busy waiting
		while (InterlockedExchange(&thread_ids_lock, TRUE) != FALSE);
		// find thread ID
		id_map::iterator it = thread_ids.find(thread_id);
		if (it != thread_ids.end()){
			if (-- (*it).second.ref_count == 0){
				if (!UnhookWindowsHookEx((*it).second.sole_hook))
					RmLog(LOG_ERROR, L"GlobalHotKey.dll: unhook: failed");
				thread_ids.erase(it);	// erase it anyway!
			}
		}
		// release lock
		InterlockedExchange(&thread_ids_lock, FALSE);
	}

};

// implementation of static data member
unsigned			thread_hook::thread_ids_lock = FALSE;
thread_hook::id_map	thread_hook::thread_ids;

// map key name string (pointing to static string data section in this
// implementation) to a virtual-key code
class key_map{

private:

	LPCWSTR			name;
	unsigned short	value;

public:

	key_map(LPCWSTR const &s, const unsigned short &v):
	  name(s), value(v)
	{
		// ...
	}

private:

	static const int		table_size;
	static const key_map	table[];

private:

	// binary search
	static unsigned short search_in_table(LPCWSTR name){
		int l=0, r=table_size, m, rcmp;
		while (l < r){
			m = (l+r)>>1;
			rcmp = wcscmp(name, table[m].name);
			if (rcmp == 0) return table[m].value;	// return point
			if (rcmp < 0)	r = m;
			else			l = m+1;
		}
		return 0x00;								// return point
	}

public:

	// return a Virtual-Key Code
	static unsigned short translate_key_code(LPCWSTR name){
		// get length
		size_t len = wcslen(name);
		if (len == 0) return 0x00;
		// make a copy of the string
		WCHAR *copy = new WCHAR[len+1];
		for (size_t i = 0; i <= len; ++ i){
			if (name[i] >= L'a' && name[i] <= L'z')
				copy[i] = name[i]&WCHAR(0xDF);		// using capital case
			else
				copy[i] = name[i];
		}
		// translate
		unsigned short result = 0x00;
		// a number or letter (guess that the users will use this keys in most case)
		if (len == 1 &&
			(L'0' <= copy[0] && copy[0] <= L'9' ||
			L'A' <= copy[0] && copy[0] <= L'Z')){
				result = copy[0];
		}
		else{
			result = search_in_table(copy);
		}

		delete[] copy;

		return result;
	}

};

// implementation of static table
// make it an ordered list, so that we can perform a high-efficiency search

// NOT supported (yet):
//	VK_CANCEL		control-break?
//	VK_SHIFT		modifier
//	VK_CONTROL		modifier
//	VK_MENU			modifier: ALT
//	VK_PAUSE		break?
//	VK_CAPITAL		state changer: upper/lower case
//	VK_KANA			IME mode
//	VK_HANGUEL		IME mode
//	VK_HANGUL		IME mode
//	VK_FINAL		IME mode
//	VK_HANJA		IME mode
//	VK_KANJI		IME mode
//	VK_CONVERT		IME mode
//	VK_NONCONVERT	IME mode
//	VK_ACCEPT		IME mode
//	VK_MODECHANGE	IME mode
//	VK_SELECT		?
//	VK_PRINT		?
//	VK_EXECUTE		?
//	VK_SNAPSHOT		special action: print screen
//	VK_HELP			?
//	VK_LWIN			modifier
//	VK_RWIN			modifier
//	VK_APPS			?
//	VK_SLEEP		special action: sleep
//	VK_SEPARATOR	?
//	VK_NUMLOCK		state changer: number lock
//	VK_SCROLL		state changer: scroll lock
//	0x92-0x96		OEM specific
//	VK_LSHIFT		modifier
//	VK_RSHIFT		modifier
//	VK_LCONTROL		modifier
//	VK_RCONTROL		modifier
//	VK_LMENU		modifier
//	VK_RMENU		modifier
//	VK_BROWSER_BACK
//	VK_BROWSER_FORWARD
//	VK_BROWSER_REFRESH
//	VK_BROWSER_REFRESH
//	VK_BROWSER_STOP
//	VK_BROWSER_SEARCH
//	VK_BROWSER_FAVORITES
//	VK_BROWSER_HOME
//	VK_VOLUME_MUTE
//	VK_VOLUME_DOWN
//	VK_VOLUME_UP
//	VK_MEDIA_NEXT_TRACK
//	VK_MEDIA_PREV_TRACK
//	VK_MEDIA_STOP
//	VK_MEDIA_PLAY_PAUSE
//	VK_LAUNCH_MAIL
//	VK_LAUNCH_MEDIA_SELECT
//	VK_LAUNCH_APP1
//	VK_LAUNCH_APP2
//	VK_OEM_8		?
//	0xe1			OEM specific
//	VK_OEM_102		?
//	0xe3-0xe4		OEM specific
//	VK_PROCESSKEY	?
//	0xe6			OEM specific
//	VK_PACKET		?
//	0xe9-f5			OEM specific
//	VK_ATTN			?
//	VK_CRSEL		?
//	VK_EXSEL		?
//	VK_EREOF		?
//	VK_PLAY			?
//	VK_ZOOM			?
//	VK_NONAME		?
//	VK_PA1			?
//	VK_OEM_CLEAR	?
const int		key_map::table_size = 96;
const key_map	key_map::table[key_map::table_size] =
{
	key_map(L"\"", VK_OEM_7),
	key_map(L"'", VK_OEM_7),
	key_map(L"*", VK_MULTIPLY),
	key_map(L"+", VK_OEM_PLUS),
	key_map(L",", VK_OEM_COMMA),
	key_map(L"-", VK_OEM_MINUS),
	key_map(L".", VK_OEM_PERIOD),
	key_map(L"/", VK_OEM_2),
	key_map(L":", VK_OEM_1),
	key_map(L";", VK_OEM_1),
	key_map(L"<", VK_OEM_COMMA),
	key_map(L"=", VK_OEM_PLUS),
	key_map(L">", VK_OEM_PERIOD),
	key_map(L"?", VK_OEM_2),
	key_map(L"ADD", VK_ADD),
	key_map(L"ADDITION", VK_ADD),
	key_map(L"BACK", VK_BACK),
	key_map(L"BACKSPACE", VK_BACK),
	key_map(L"CLEAR", VK_CLEAR),
	key_map(L"DEC", VK_DECIMAL),
	key_map(L"DECIMAL", VK_DECIMAL),
	key_map(L"DEL", VK_DELETE),
	key_map(L"DELETE", VK_DELETE),
	key_map(L"DIV", VK_DIVIDE),
	key_map(L"DIVIDE", VK_DIVIDE),
	key_map(L"DOWN", VK_DOWN),
	key_map(L"END", VK_END),
	key_map(L"ENT", VK_RETURN),
	key_map(L"ENTER", VK_RETURN),
	key_map(L"ESC", VK_ESCAPE),
	key_map(L"ESCAPE", VK_ESCAPE),
	key_map(L"F1", VK_F1),
	key_map(L"F10", VK_F10),
	key_map(L"F11", VK_F11),
	key_map(L"F12", VK_F12),
	key_map(L"F13", VK_F13),
	key_map(L"F14", VK_F14),
	key_map(L"F15", VK_F15),
	key_map(L"F16", VK_F16),
	key_map(L"F17", VK_F17),
	key_map(L"F18", VK_F18),
	key_map(L"F19", VK_F19),
	key_map(L"F2", VK_F2),
	key_map(L"F20", VK_F20),
	key_map(L"F21", VK_F21),
	key_map(L"F22", VK_F22),
	key_map(L"F23", VK_F23),
	key_map(L"F24", VK_F24),
	key_map(L"F3", VK_F3),
	key_map(L"F4", VK_F4),
	key_map(L"F5", VK_F5),
	key_map(L"F6", VK_F6),
	key_map(L"F7", VK_F7),
	key_map(L"F8", VK_F8),
	key_map(L"F9", VK_F9),
	key_map(L"HOME", VK_HOME),
	key_map(L"INS", VK_INSERT),
	key_map(L"INSERT", VK_INSERT),
	key_map(L"LBUTTON", VK_LBUTTON),
	key_map(L"LEFT", VK_LEFT),
	key_map(L"MBUTTON", VK_MBUTTON),
	key_map(L"MUL", VK_MULTIPLY),
	key_map(L"MULTIPLY", VK_MULTIPLY),
	key_map(L"NUM0", VK_NUMPAD0),
	key_map(L"NUM1", VK_NUMPAD1),
	key_map(L"NUM2", VK_NUMPAD2),
	key_map(L"NUM3", VK_NUMPAD3),
	key_map(L"NUM4", VK_NUMPAD4),
	key_map(L"NUM5", VK_NUMPAD5),
	key_map(L"NUM6", VK_NUMPAD6),
	key_map(L"NUM7", VK_NUMPAD7),
	key_map(L"NUM8", VK_NUMPAD8),
	key_map(L"NUM9", VK_NUMPAD9),
	key_map(L"PAGE DOWN", VK_NEXT),
	key_map(L"PAGE UP", VK_PRIOR),
	key_map(L"PAGE-DOWN", VK_NEXT),
	key_map(L"PAGE-UP", VK_PRIOR),
	key_map(L"RBUTTON", VK_RBUTTON),
	key_map(L"RIGHT", VK_RIGHT),
	key_map(L"SPACE", VK_SPACE),
	key_map(L"SUB", VK_SUBTRACT),
	key_map(L"SUBTRACT", VK_SUBTRACT),
	key_map(L"TAB", VK_TAB),
	key_map(L"TABLE", VK_TAB),
	key_map(L"UP", VK_UP),
	key_map(L"XBUTTON1", VK_XBUTTON1),
	key_map(L"XBUTTON2", VK_XBUTTON2),
	key_map(L"[", VK_OEM_4),
	key_map(L"\\", VK_OEM_5),
	key_map(L"]", VK_OEM_6),
	key_map(L"_", VK_OEM_MINUS),
	key_map(L"`", VK_OEM_3),
	key_map(L"{", VK_OEM_4),
	key_map(L"|", VK_OEM_5),
	key_map(L"}", VK_OEM_6),
	key_map(L"~", VK_OEM_3)
};

// data object for rainmeter
class rm_measure{

private:
	// keep this message for convenience
	void	   *skin;
	HWND		skin_window;
	DWORD		skin_thread_id;
	//
	WCHAR		command[256];				// -warning: specified size buffer
	// hot key as a union
	union{
		unsigned long
				as_1x8;
		struct{
			unsigned short
				modifiers;
			unsigned short
				vk_code;
		}		as_2x4;
	}			hot_key;
	// for double strike feature
	DWORD		max_time_interval;
	DWORD		last_strike;
	// state
	BOOL		add_hook_ok;
	ATOM		atom_value;
	BOOL		key_registered;

public:

	rm_measure(void *rm):
		atom_value(0), key_registered(false), add_hook_ok(false), last_strike(0UL)
	{
		LPCWSTR	str_in = NULL;		// for reading
		WCHAR	log_message[1024];	// for logging -warning: specified size buffer
		// get skin
		skin = RmGetSkin(rm);
		// get skin window handle
		skin_window = RmGetSkinWindow(rm);
		// get thread id
		skin_thread_id = GetWindowThreadProcessId(skin_window, NULL);
		// assert that the skin windows is created by the current thread
		DWORD curr_thread_id = GetCurrentThreadId();
		if (skin_thread_id != curr_thread_id){
			RmLog(LOG_ERROR, L"GlobalHotKey.dll: thread id assertion failed");
			return;
		}
		// add hook
		add_hook_ok = thread_hook::add_hook(skin_thread_id);
		if (!add_hook_ok) return;
		// read hot key setting
		str_in = RmReadString(rm, L"Key", L"");
		hot_key.as_2x4.vk_code = key_map::translate_key_code(str_in);
		if (hot_key.as_2x4.vk_code == 0){
			wsprintf(log_message, L"GlobalHotKey.dll: invalid key value: %s", str_in);
			RmLog(LOG_ERROR, log_message);
			return;
		}
		// read modifiers setting
		int key_alt, key_ctrl, key_shift, key_win;
		key_alt = RmReadInt(rm, L"Alt", 0);
		key_ctrl = RmReadInt(rm, L"Ctrl", 0);
		key_shift = RmReadInt(rm, L"Shift", 0);
		key_win = RmReadInt(rm, L"Win", 0);
		// make modifiers
		hot_key.as_2x4.modifiers = 0x00;
		if (key_alt)	hot_key.as_2x4.modifiers |= MOD_ALT;
		if (key_ctrl)	hot_key.as_2x4.modifiers |= MOD_CONTROL;
		if (key_shift)	hot_key.as_2x4.modifiers |= MOD_SHIFT;
		if (key_win)	hot_key.as_2x4.modifiers |= MOD_WIN;
		// read double strike setting
		int time_int = RmReadInt(rm, L"DoubleStrike", 0);
		max_time_interval = time_int>0?static_cast<DWORD>(time_int):0UL;
		// read command
		str_in = RmReadString(rm, L"Command", L"");
		wcscpy_s(command, str_in);
		// make atom string
		WCHAR atom_string[256];				// warning: specified size buffer
		str_in = RmGetSkinName(rm);
		wcscpy_s(atom_string, str_in);
		str_in = RmGetMeasureName(rm);
		wcscat_s(atom_string, L"\\");
		wcscat_s(atom_string, str_in);
		// add atom
		if (GlobalFindAtom(atom_string) != 0){
			wsprintf(log_message,
				L"GlobalHotKey.dll: found global atom: %s",
				atom_string);
			RmLog(LOG_ERROR, log_message);
			return;
		}
		atom_value = GlobalAddAtom(atom_string);
		if (atom_value < 0xC000){
			wsprintf(log_message,
				L"GlobalHotKey.dll: add global atom \"%s\": failed",
				atom_string);
			RmLog(LOG_ERROR, log_message);
			return;
		}
		// register hot key
		key_registered = RegisterHotKey(skin_window, atom_value,
			hot_key.as_2x4.modifiers, hot_key.as_2x4.vk_code);
		if (!key_registered){
			wsprintf(log_message,
				L"GlobalHotKey.dll: register hot key(code:%lx): failed",
				hot_key.as_1x8);
			RmLog(LOG_ERROR, log_message);
			return;
		}
		// 
		while (InterlockedExchange(&hot_keys_lock, TRUE) != FALSE);
		hot_keys[hot_key.as_1x8] = this;	// add to hot key list
		InterlockedExchange(&hot_keys_lock, FALSE);
	}

	~rm_measure(){
		//
		if (key_registered){
			// 
			while (InterlockedExchange(&hot_keys_lock, TRUE) != FALSE);
			hot_keys.erase(hot_key.as_1x8);
			InterlockedExchange(&hot_keys_lock, FALSE);
			//
			UnregisterHotKey(skin_window, atom_value);
		}
		//
		if (atom_value != 0) GlobalDeleteAtom(atom_value);
		//
		if (add_hook_ok) thread_hook::del_hook(skin_thread_id);
	}

	void run_command(DWORD time){
		// using double-strike mode
		if (max_time_interval > 0){
			if (time > last_strike &&
				time - last_strike <= max_time_interval){
				last_strike = 0UL;			// set time
				RmExecute(skin, command);
			}
			else{
				last_strike = time;			// clean time
			}
		}
		// normal mode
		else{
			RmExecute(skin, command);
		}
	}

};

static LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam){
	// from the previous hook: do nothing
	if (nCode < 0) return CallNextHookEx(NULL, nCode, wParam, lParam);
	//
	MSG	*detail = (MSG *)lParam;
	if (detail->message == WM_HOTKEY){
		// get lock by busy waiting
		while (InterlockedExchange(&hot_keys_lock, TRUE) != FALSE);
		map<unsigned long, void *>::iterator it = hot_keys.find(
			static_cast<unsigned long>(detail->lParam));
		if (it != hot_keys.end()){
			nCode = -1;
			reinterpret_cast<rm_measure *>((*it).second)->
				run_command(detail->time);
		}
		InterlockedExchange(&hot_keys_lock, FALSE);
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// load/reload from Rainmeter runtime plugin object
PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	// create data
	*data = new rm_measure(rm);
}

// 
PLUGIN_EXPORT void Reload(void *, void *, double *){}

// 
PLUGIN_EXPORT void Finalize(void *data)
{
	delete reinterpret_cast<rm_measure *>(data);
}
