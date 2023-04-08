#include <pch.h>
#include <Windows.h>

STDAPI_(BOOL) DllMain(HINSTANCE /*hInstance*/, DWORD dwReason, LPVOID /*lpReserved*/)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
    {
        break;
    }
    }

    return TRUE;
}
