#include "pch.h"
#include "NearObjectSession.h"
#include "NearObjectSession.g.cpp"

namespace winrt::Windows::Devices::NearObject::implementation
{
    uint32_t NearObjectSession::Id()
    {
        return m_id;
    }
    void NearObjectSession::Id(uint32_t id)
    {
        m_id = id;
    }
}
