#pragma once
#include "NearObjectService.g.h"

namespace winrt::Windows::Devices::NearObject::System::implementation
{
    struct NearObjectService : NearObjectServiceT<NearObjectService>
    {
        NearObjectService() = default;

    };
}
namespace winrt::Windows::Devices::NearObject::System::factory_implementation
{
    struct NearObjectService : NearObjectServiceT<NearObjectService, implementation::NearObjectService>
    {
    };
}
