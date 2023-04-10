#include "pch.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Devices::NearObject;
using namespace Windows::Devices::NearObject::System;

int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    init_apartment(apartment_type::single_threaded);
    NearObject nearObject{};
    NearObjectSession nearObjectSession{};
    NearObjectService nearObjectService{};

    winrt::hstring serviceName = nearObjectService.Name();
    auto nameAsCStr = serviceName.c_str();

    MessageBoxW(::GetDesktopWindow(), L"Hello", nameAsCStr, MB_OK);
    //Uri uri(L"http://aka.ms/cppwinrt");
    //::MessageBoxW(::GetDesktopWindow(), uri.AbsoluteUri().c_str(), L"C++/WinRT Desktop Application", MB_OK);
}
