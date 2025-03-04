
#include <iostream>
#include <limits>
#include <string>
#include <string_view>

#include <magic_enum.hpp>
#include <notstd/guid.hxx>
#include <plog/Log.h>

#include <windows/devices/DeviceEnumerator.hxx>
#include <windows/devices/DevicePresenceMonitor.hxx>
#include <windows/devices/uwb/UwbDevice.hxx>
#include <windows/devices/uwb/UwbDeviceDriver.hxx>

#include "NearObjectCliDataWindows.hxx"
#include "NearObjectCliHandlerWindows.hxx"

using namespace nearobject::cli;
using namespace windows::devices;

namespace detail
{
/**
 * @brief Get the default UWB device on the system.
 *
 * This enumerates all the UWB devices on the system and returns the name of the
 * first one.
 *
 * @return std::string
 */
std::string
GetUwbDeviceNameDefault() noexcept
{
    const auto uwbDeviceNames = DeviceEnumerator::GetDeviceInterfaceClassInstanceNames(windows::devices::uwb::InterfaceClassUwb);
    return !uwbDeviceNames.empty() ? uwbDeviceNames.front() : "";
}

/**
 * @brief Probes for uwb devices and prompts the user to select one.
 *
 * @param deviceClassGuid
 * @return std::string
 */
std::string
GetUwbDeviceNameFromProbe(std::string_view deviceClassGuid = windows::devices::uwb::InterfaceClassUwbString)
{
    const auto uwbDeviceNames = DeviceEnumerator::GetDeviceInterfaceClassInstanceNames(deviceClassGuid);
    if (uwbDeviceNames.empty()) {
        return "";
    }

    int32_t index = 0;
    for (const auto& uwbDeviceName : uwbDeviceNames) {
        std::cout << "[" << index++ << "] " << uwbDeviceName << std::endl;
    }

    for (index = -1;;) {
        std::cout << "select the uwb device to use from the list above [0-" << uwbDeviceNames.size() - 1 << "]: ";
        std::cin >> index;
        if (index >= 0 && index < uwbDeviceNames.size()) {
            break;
        }
        if (std::cin.fail()) {
            std::cin.clear();
        }
        std::cout << "invalid device index specified; please enter an index between 0 and " << uwbDeviceNames.size() - 1 << std::endl;
    }

    return uwbDeviceNames[static_cast<std::size_t>(index)];
}

/**
 * @brief Tries to resolve the UwbDevice to use for ranging.
 *
 * @param cliData
 * @return std::shared_ptr<windows::devices::UwbDevice>
 */
std::shared_ptr<windows::devices::uwb::UwbDevice>
ResolveUwbDevice(const nearobject::cli::NearObjectCliDataWindows& cliData)
{
    std::string deviceName = cliData.DeviceNameProbe
        ? GetUwbDeviceNameFromProbe(cliData.DeviceClassGuid)
        : cliData.DeviceName.value_or(GetUwbDeviceNameDefault());

    // Ensure a device name was set.
    if (deviceName.empty()) {
        return nullptr;
    }

    return windows::devices::uwb::UwbDevice::Create(deviceName);
}
} // namespace detail

std::shared_ptr<::uwb::UwbDevice>
NearObjectCliHandlerWindows::ResolveUwbDevice(const nearobject::cli::NearObjectCliData& cliData) noexcept
{
    const auto* cliDataWindows = dynamic_cast<const NearObjectCliDataWindows*>(&cliData);
    if (!cliDataWindows) {
        return nullptr;
    }

    return detail::ResolveUwbDevice(*cliDataWindows);
}

void
NearObjectCliHandlerWindows::OnDeviceArrived(const std::string& deviceName)
{
    auto uwbDevice = windows::devices::uwb::UwbDevice::Create(deviceName);
    if (!uwbDevice) {
        PLOG_ERROR << "Failed to instantiate UWB device with name " << deviceName;
        return;
    }

    uwbDevice->Initialize();
    m_uwbDevices.push_back(std::move(uwbDevice));
}

void
NearObjectCliHandlerWindows::OnDeviceDeparted(const std::string& deviceName)
{
    auto numErased = std::erase_if(m_uwbDevices, [&](const auto& uwbDevice) {
        return uwbDevice->DeviceName() == deviceName;
    });
    if (numErased == 0) {
        PLOG_WARNING << "UWB device with name " << deviceName << " not found; ignoring removal event";
    }
}

void
NearObjectCliHandlerWindows::HandleMonitorMode() noexcept
try {
    DevicePresenceMonitor presenceMonitor(
        windows::devices::uwb::InterfaceClassUwb, [&](auto&& deviceGuid, auto&& presenceEvent, auto&& deviceName) {
            const auto presenceEventName = magic_enum::enum_name(presenceEvent);
            PLOG_INFO << deviceName << " " << presenceEventName << std::endl;

            switch (presenceEvent) {
            case DevicePresenceEvent::Arrived:
                OnDeviceArrived(deviceName);
                break;
            case DevicePresenceEvent::Departed:
                OnDeviceDeparted(deviceName);
                break;
            default:
                PLOG_ERROR << "Ignoring unknown presence event";
                break;
            }
        },
        /* enumerateInitialDevicesOnStart = */ true);

    presenceMonitor.Start();

    // Wait for input before stopping.
    PLOG_INFO << "UWB monitor mode started. Press <enter> to stop monitoring.";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    presenceMonitor.Stop();
    PLOG_INFO << "UWB monitor mode stopped";

} catch (...) {
    // TODO: handle this properly
}
