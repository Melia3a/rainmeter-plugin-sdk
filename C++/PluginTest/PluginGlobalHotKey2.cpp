#include <Windows.h>

#include "../../API/RainmeterAPI.h"

class hook_singleton{

private:

	static HHOOK unique_hook;

private:

	static LRESULT CALLBACK HookProc(int, WPARAM, LPARAM);

};

HHOOK hook_singleton::unique_hook = SetWindowsHookEx(WH_KEYBOARD,
	hook_singleton::HookProc,
	NULL,
	0UL);
