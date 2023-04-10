#pragma once
#include "NearObjectService.g.h"

namespace winrt::Windows::Devices::NearObject::System::implementation
{
    struct NearObjectService : NearObjectServiceT<NearObjectService>
    {
        NearObjectService() = default;

        hstring Name();
        void Name(hstring const& value);

    private:
        winrt::hstring m_name{ L"NearObjectService" };
    };
}
namespace winrt::Windows::Devices::NearObject::System::factory_implementation
{
    struct NearObjectService : NearObjectServiceT<NearObjectService, implementation::NearObjectService>
    {
    };
}
