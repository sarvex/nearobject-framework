
#ifndef NEAR_OBJECT_SERVICE_HELPER_HXX
#define NEAR_OBJECT_SERVICE_HELPER_HXX

#include <winrt/Windows.Devices.NearObject.System.h>

namespace windows::devices::nearobject::system
{
winrt::Windows::Devices::NearObject::System::NearObjectService
InstantiateService();
};

#endif // NEAR_OBJECT_SERVICE_HELPER_HXX
