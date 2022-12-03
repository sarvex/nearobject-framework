
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>

#include <CLI/CLI.hpp>
#include <magic_enum.hpp>
#include <notstd/guid.hxx>

#include <windows/devices/DeviceEnumerator.hxx>
#include <windows/devices/DevicePresenceMonitor.hxx>
#include <windows/uwb/UwbDevice.hxx>
#include <windows/uwb/UwbDeviceDriver.hxx>
#include <nearobject/cli/NearObjectCli.hxx>

using namespace windows::devices;

namespace detail
{
/**
 * @brief Get the default UWB device on the system.
 *
 * This enumerates all the UWB devices on the system and returns the name of the
 * first one.
 *
 * @return std::optional<std::string>
 */
std::optional<std::string>
GetDefaultUwbDeviceName() noexcept
{
    const auto uwbDeviceNames = DeviceEnumerator::GetDeviceInterfaceClassInstanceNames(windows::devices::uwb::InterfaceClassUwb);
    return !uwbDeviceNames.empty()
        ? std::optional<std::string>(uwbDeviceNames.front())
        : std::nullopt;
}

} // namespace detail

int
main(int argc, char* argv[])
{
    nearobject::cli::NearObjectCli cli{};
    CLI::App& app = cli.GetParser();

    // Configure the cli parsing app with Windows-specific options.
    bool deviceNameProbe{ false };
    std::optional<std::string> deviceName;
    std::string deviceClassGuid = windows::devices::uwb::InterfaceClassUwbString;

    app.add_option("--deviceName", deviceName, "uwb device name (path)");
    app.add_option("--deviceClass", deviceClassGuid, "uwb device class guid (override)");
    app.add_flag("--probe", deviceNameProbe, "probe for the uwb device name to use");

    // Parse the arguments.
    int result = cli.Parse(argc, argv);
    if (result != 0) {
        return result;
    }

    if (deviceNameProbe) {
        if (deviceName.has_value()) {
            std::cout << "warning: device name '" << deviceName.value() << "' will be ignored due to device name probe request" << std::endl;
        }

        const auto uwbDeviceNames = DeviceEnumerator::GetDeviceInterfaceClassInstanceNames(deviceClassGuid);
        if (!uwbDeviceNames.empty()) {
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

            deviceName = uwbDeviceNames[static_cast<std::size_t>(index)];
        }
    }

    // Ensure a device name was set.
    if (!deviceName.has_value()) {
        deviceName = detail::GetDefaultUwbDeviceName();
        if (!deviceName) {
            std::cerr << "error: no uwb device could be found" << std::endl;
            return -1;
        }
    }

    std::cout << "Using UWB device " << deviceName.value() << std::endl;
    auto uwbDevice = std::make_unique<windows::devices::UwbDevice>(deviceName.value());
    if (!uwbDevice) {
        std::cerr << "error: failed to create instance of uwb device " << deviceName.value() << std::endl;
        return -1;
    }

    return 0;
}