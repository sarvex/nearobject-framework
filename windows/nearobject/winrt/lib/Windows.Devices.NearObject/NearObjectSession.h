#pragma once
#include "NearObjectSession.g.h"

namespace winrt::Windows::Devices::NearObject::implementation
{
    struct NearObjectSession : NearObjectSessionT<NearObjectSession>
    {
        NearObjectSession() = default;

        uint32_t Id();
        void Id(uint32_t value);

    private:
        uint32_t m_id;
    };
}
namespace winrt::Windows::Devices::NearObject::factory_implementation
{
    struct NearObjectSession : NearObjectSessionT<NearObjectSession, implementation::NearObjectSession>
    {
    };
}
