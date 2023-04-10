#include "pch.h"
#include "NearObjectService.h"
#include "NearObjectService.g.cpp"

namespace winrt::Windows::Devices::NearObject::System::implementation
{
    hstring NearObjectService::Name()
    {
        return m_name;
    }
    void NearObjectService::Name(hstring const& value)
    {
        m_name = value;
    }
}
