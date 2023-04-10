#include "pch.h"

#include <array>
#include <roapi.h>

using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::Devices::NearObject::System;

int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    winrt::init_apartment(winrt::apartment_type::single_threaded);

    std::array<winrt::hstring, 1> classNames{ winrt::hstring(winrt::name_of<NearObjectService>()) };
    std::array<HSTRING, 1> rawClassNames;
    std::array<PFNGETACTIVATIONFACTORY, 1> rawFactoryFunctions;
    for (auto i = 0U; i < 1; ++i)
    {
        rawClassNames[i] = wil::get_abi(classNames[i]);
        rawFactoryFunctions[i] = [](HSTRING clsidId, IActivationFactory** factory) -> HRESULT {
            return WINRT_GetActivationFactory(clsidId, reinterpret_cast<void**>(factory));
        };
    }

    wil::unique_ro_registration_cookie classRegistrationsCookie;
    winrt::check_hresult(RoRegisterActivationFactories(rawClassNames.data(), rawFactoryFunctions.data(), static_cast<UINT32>(rawClassNames.size()), classRegistrationsCookie.put()));
    ::MessageBoxW(::GetDesktopWindow(), L"Startup", L"NearObjectService Host", MB_OK);
}
