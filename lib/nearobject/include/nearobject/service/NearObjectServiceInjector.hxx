
#ifndef NEAR_OBJECT_SERVICE_INJECTOR_HXX
#define NEAR_OBJECT_SERVICE_INJECTOR_HXX

#include <memory>

#include <nearobject/service/NearObjectDeviceManager.hxx>
#include <nearobject/service/NearObjectProfileManager.hxx>

namespace nearobject
{
namespace service
{
/**
 * @brief NearObjectService dependency injector.
 */
struct NearObjectServiceInjector
{
    std::shared_ptr<NearObjectProfileManager> ProfileManager;
    std::shared_ptr<NearObjectDeviceManager> DeviceManager;
};
} // namespace service
} // namespace nearobject

#endif // NEAR_OBJECT_SERVICE_INJECTOR_HXX