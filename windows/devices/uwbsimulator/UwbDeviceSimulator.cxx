
#include <windows/devices/uwb/simulator/UwbDeviceSimulator.hxx>
#include <windows/devices/uwb/simulator/UwbSessionSimulator.hxx>

using namespace windows::devices::uwb::simulator;

UwbDeviceSimulator::UwbDeviceSimulator(std::string deviceName) :
    UwbDevice(deviceName),
    m_deviceName(std::move(deviceName))
{
    // The simulator driver publishes device interfaces for both itself
    // (GUID_DEVINTERFACE_UWB_SIMULATOR) and UWB (GUID_UWB_DEVICE_INTERFACE). It
    // will respond to requests for both interfaces on each device instance, so
    // we simply push the deviceName to the underlying UWB device.
    // eg. with 'UwbDevice(deviceName)' above.
}

const std::string&
UwbDeviceSimulator::DeviceName() const noexcept
{
    return m_deviceName;
}

bool
UwbDeviceSimulator::Initialize()
{
    UwbDevice::Initialize();
    m_handleDriver = UwbDevice::DriverHandle();
    return true;
}

std::shared_ptr<::uwb::UwbSession>
UwbDeviceSimulator::CreateSessionImpl(std::weak_ptr<::uwb::UwbSessionEventCallbacks> callbacks)
{
    return UwbDevice::CreateSessionImpl<UwbSessionSimulator>(std::move(callbacks));
}

UwbSimulatorCapabilities
UwbDeviceSimulator::GetSimulatorCapabilities()
{
    UwbSimulatorCapabilities uwbSimulatorCapabilities{};
    // TODO: invoke IOCTL_UWB_DEVICE_SIM_GET_CAPABILITIES IOCTL
    return uwbSimulatorCapabilities;
}
