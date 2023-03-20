
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

#include <magic_enum.hpp>
#include <nearobject/cli/NearObjectCli.hxx>
#include <notstd/tostring.hxx>

using namespace nearobject::cli;
using namespace strings::ostream_operators;

NearObjectCli::NearObjectCli(std::shared_ptr<NearObjectCliData> cliData, std::shared_ptr<NearObjectCliHandler> cliHandler) :
    m_cliData(cliData),
    m_cliHandler(cliHandler)
{
    m_cliApp = CreateParser();

    if (!m_cliApp) {
        throw std::runtime_error("failed to create command line parser");
    }
}

std::shared_ptr<NearObjectCliData>
NearObjectCli::GetData() const noexcept
{
    return m_cliData;
}

int
NearObjectCli::Parse(int argc, char* argv[]) noexcept
{
    try {
        m_cliApp->parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return m_cliApp->exit(e);
    }

    return 0;
}

CLI::App&
NearObjectCli::GetParser()
{
    return *m_cliApp;
}

CLI::App&
NearObjectCli::GetUwbApp() noexcept
{
    return *m_uwbApp;
}

CLI::App&
NearObjectCli::GetRangeApp() noexcept
{
    return *m_rangeApp;
}

CLI::App&
NearObjectCli::GetRawApp() noexcept
{
    return *m_rawApp;
}

CLI::App&
NearObjectCli::GetRangeStartApp() noexcept
{
    return *m_rangeStartApp;
}

CLI::App&
NearObjectCli::GetRangeStopApp() noexcept
{
    return *m_rangeStopApp;
}

std::shared_ptr<uwb::UwbDevice>
NearObjectCli::GetUwbDevice() noexcept
{
    auto uwbDevice = m_cliHandler->ResolveUwbDevice(*m_cliData);
    return std::move(uwbDevice);
}

std::unique_ptr<CLI::App>
NearObjectCli::CreateParser() noexcept
{
    // top-level command
    auto app = std::make_unique<CLI::App>("A command line tool to assist with all things nearobject", "nocli");
    app->require_subcommand();

    // sub-commands
    m_uwbApp = AddSubcommandUwb(app.get());

    return app;
}

namespace detail
{
/**
 * @brief Helper function to produce a mapping of enumeration field names to their
 * corresponding values.
 *
 * @tparam EnumType The type of the enumeration.
 * @return std::unordered_map<std::string, EnumType>
 */
template <typename EnumType>
// clang-format off
requires std::is_enum_v<EnumType>
const std::unordered_map<std::string, EnumType>
CreateEnumerationStringMap() noexcept
// clang-format on
{
    const auto& reverseMap = magic_enum::enum_entries<EnumType>();
    std::unordered_map<std::string, EnumType> map;

    for (const auto& [enumValue, enumName] : reverseMap) {
        map.emplace(enumName, enumValue);
    }

    return map;
}

/**
 * @brief Adds a cli option based on an enumeration.
 *
 * This uses the name of the enumeration as the option name and gives
 * the usage as the description.
 *
 * @tparam EnumType The enumeration type to add as an option.
 * @param app The target CLI11 application to add the option to.
 * @param assignTo The destination variable to store the parsed option in.
 * @param isMandatory Flag that determines whether or not the option is mandatory.
 * @return The added option, which additional configuration can be applied to.
 */
template <typename EnumType>
// clang-format off
requires std::is_enum_v<EnumType>
CLI::Option*
AddEnumOption(CLI::App* app, std::optional<EnumType>& assignTo, bool isMandatory)
// clang-format on
{
    std::string optionName{ std::string("--").append(magic_enum::enum_type_name<EnumType>()) };

    const auto map = CreateEnumerationStringMap<EnumType>();
    std::ostringstream enumUsage;

    auto it = std::cbegin(map);
    const auto& [name, value] = *it;
    enumUsage << "value in { " << static_cast<int>(value) << "(" << name << ")";
    for (it = std::next(std::cbegin(map)); it != std::cend(map); std::advance(it, 1)) {
        const auto& [name, value] = *it;
        enumUsage << ", " << static_cast<int>(value) << "(" << name << ")";
    }
    enumUsage << " }";

    if (isMandatory) {
        return app->add_option(std::move(optionName), assignTo, enumUsage.str())->capture_default_str()->required();
    } else {
        return app->add_option(std::move(optionName), assignTo, enumUsage.str())->capture_default_str();
    }
}

} // namespace detail

CLI::App*
NearObjectCli::AddSubcommandUwb(CLI::App* parent)
{
    // top-level command
    auto uwbApp = parent->add_subcommand("uwb", "commands related to uwb")->require_subcommand()->fallthrough();

    // sub-commands
    m_monitorApp = AddSubcommandUwbMonitor(uwbApp);
    m_rangeApp = AddSubcommandUwbRange(uwbApp);
    m_rawApp = AddSubcommandUwbRaw(uwbApp);

    return uwbApp;
}

CLI::App*
NearObjectCli::AddSubcommandUwbMonitor(CLI::App* parent)
{
    auto monitorApp = parent->add_subcommand("monitor", "commands relating to monitor mode")->fallthrough();

    monitorApp->final_callback([this] {
        m_cliHandler->HandleMonitorMode();
    });

    return monitorApp;
}

CLI::App*
NearObjectCli::AddSubcommandUwbRange(CLI::App* parent)
{
    // top-level command
    auto rangeApp = parent->add_subcommand("range", "commands related to ranging")->require_subcommand()->fallthrough();

    // options
    rangeApp->add_option("--SessionDataVersion", m_cliData->SessionData.sessionDataVersion)->capture_default_str();
    rangeApp->add_option("--SessionId", m_cliData->SessionData.sessionId)->capture_default_str();
    rangeApp->add_option("--SubSessionId", m_cliData->SessionData.subSessionId)->capture_default_str();

    // sub-commands
    m_rangeStartApp = AddSubcommandUwbRangeStart(rangeApp);
    m_rangeStopApp = AddSubcommandUwbRangeStop(rangeApp);

    return rangeApp;
}

CLI::App*
NearObjectCli::AddSubcommandUwbRaw(CLI::App* parent)
{
    // top-level command
    auto rawApp = parent->add_subcommand("raw", "individual commands")->require_subcommand()->fallthrough();

    // sub-commands
    AddSubcommandUwbRawDeviceReset(rawApp);
    AddSubcommandUwbRawGetDeviceInfo(rawApp);

    return rawApp;
}

CLI::App*
NearObjectCli::AddSubcommandUwbRangeStart(CLI::App* parent)
{
    auto& uwbConfig = m_cliData->uwbConfiguration;
    auto rangeStartApp = parent->add_subcommand("start", "start ranging. Please refer to Table 53 of the FiRa CSML spec for more info on the options")->fallthrough();

    // TODO is there a way to put all the enums into a list of [optionName, optionDestination, optionMap] so we don't have to create the initializer list each time

    // List mandatory params first
    detail::AddEnumOption(rangeStartApp, uwbConfig.deviceRole, true);
    detail::AddEnumOption(rangeStartApp, uwbConfig.multiNodeMode, true);
    rangeStartApp->add_option("--NumberOfControlees", uwbConfig.numberOfControlees, "1 <= N <= 8")->capture_default_str()->required(); // TODO: Input validation
    // TODO: Accept multiple controlees
    if (uwbConfig.macAddressMode == uwb::UwbMacAddressType::Extended) {
        rangeStartApp->add_option("--DeviceMacAddress", m_cliData->deviceMacAddress, "8-byte extended MAC address of own device: e.g. 12:34:56:78:87:65:43:21")->capture_default_str()->required();
        rangeStartApp->add_option("--DestinationMacAddress", m_cliData->destinationMacAddress, "8-byte extended MAC address of other device(s). If device is Controller, list NumberOfControlees mac addresses. If device is Controlee, list Controller mac address")->capture_default_str()->required();
    } else { // uwb::UwbMacAddressType::Short OR empty (default)
        rangeStartApp->add_option("--DeviceMacAddress", m_cliData->deviceMacAddress, "2-byte short MAC address of own device: e.g. 12:34")->capture_default_str()->required();
        rangeStartApp->add_option("--DestinationMacAddress", m_cliData->destinationMacAddress, "2-byte short MAC address of controlee. If device is Controller, list NumberOfControlees mac addresses. If device is Controlee, list Controller mac address")->capture_default_str()->required();
    }
    detail::AddEnumOption(rangeStartApp, uwbConfig.deviceType, true);

    // enumerations
    detail::AddEnumOption(rangeStartApp, uwbConfig.rangingDirection, false);
    detail::AddEnumOption(rangeStartApp, uwbConfig.rangingMeasurementReportMode, false);
    detail::AddEnumOption(rangeStartApp, uwbConfig.stsConfiguration, false);
    detail::AddEnumOption(rangeStartApp, uwbConfig.rangingTimeStruct, false);
    detail::AddEnumOption(rangeStartApp, uwbConfig.schedulingMode, false);
    detail::AddEnumOption(rangeStartApp, uwbConfig.channel, false);
    detail::AddEnumOption(rangeStartApp, uwbConfig.rframeConfig, false);
    detail::AddEnumOption(rangeStartApp, uwbConfig.convolutionalCodeConstraintLength, false);
    detail::AddEnumOption(rangeStartApp, uwbConfig.prfMode, false);
    detail::AddEnumOption(rangeStartApp, uwbConfig.macAddressFcsType, false);

    // booleans
    rangeStartApp->add_flag("--HoppingMode", uwbConfig.hoppingMode)->capture_default_str();
    rangeStartApp->add_flag("--BlockStriding", uwbConfig.blockStriding)->capture_default_str();

    // TODO check for int sizes when parsing input
    rangeStartApp->add_option("--UwbInitiationTime", uwbConfig.uwbInitiationTime, "uint32_t")->capture_default_str();
    rangeStartApp->add_option("--Sp0PhySetNumber", uwbConfig.sp0PhySetNumber, "uint8_t")->capture_default_str();
    rangeStartApp->add_option("--Sp1PhySetNumber", uwbConfig.sp1PhySetNumber, "uint8_t")->capture_default_str();
    rangeStartApp->add_option("--Sp3PhySetNumber", uwbConfig.sp3PhySetNumber, "uint8_t")->capture_default_str();
    rangeStartApp->add_option("--PreableCodeIndex", uwbConfig.preableCodeIndex, "uint8_t")->capture_default_str();
    rangeStartApp->add_option("--SlotsPerRangingRound", uwbConfig.slotsPerRangingRound, "uint8_t")->capture_default_str();
    rangeStartApp->add_option("--MaxContentionPhaseLength", uwbConfig.maxContentionPhaseLength, "uint8_t")->capture_default_str();
    rangeStartApp->add_option("--SlotDuration", uwbConfig.slotDuration, "uint8_t")->capture_default_str();
    rangeStartApp->add_option("--RangingInterval", uwbConfig.rangingInterval, "uint16_t")->capture_default_str();
    rangeStartApp->add_option("--KeyRotationRate", uwbConfig.keyRotationRate, "uint8_t")->capture_default_str();
    rangeStartApp->add_option("--MaxRangingRoundRetry", uwbConfig.maxRangingRoundRetry, "uint16_t")->capture_default_str();
    rangeStartApp->add_option("--StaticRangingInfoVendorId", m_cliData->StaticRanging.VendorId, "uint16_t. If --SecureRangingInfo* options are used, this option will be overridden")->capture_default_str();

    // arrays
    rangeStartApp->add_option("--StaticRangingInfoInitializationVector", m_cliData->StaticRanging.InitializationVector, "array of uint8_t. If --SecureRangingInfo* options are used, this option will be overridden")->delimiter(':');

    // strings
    rangeStartApp->add_option("--FiraPhyVersion", uwbConfig.firaPhyVersionString)->capture_default_str();
    rangeStartApp->add_option("--FiraMacVersion", uwbConfig.firaMacVersionString)->capture_default_str();
    rangeStartApp->add_option("--ResultReportConfiguration", uwbConfig.resultReportConfigurationString)->capture_default_str();

    rangeStartApp->parse_complete_callback([this] {
        // Set MAC addresses
        std::optional<uwb::UwbMacAddress> controllerMacAddress;
        std::optional<uwb::UwbMacAddress> controleeMacAddress;

        const auto macAddressType = m_cliData->uwbConfiguration.macAddressMode == uwb::UwbMacAddressType::Extended ? uwb::UwbMacAddressType::Extended : uwb::UwbMacAddressType::Short;

        // TODO: Support multiple controlees
        if (m_cliData->uwbConfiguration.deviceType == uwb::protocol::fira::DeviceType::Controller) {
            controllerMacAddress = uwb::UwbMacAddress::FromString(m_cliData->deviceMacAddress, macAddressType);
            controleeMacAddress = uwb::UwbMacAddress::FromString(m_cliData->destinationMacAddress, macAddressType);
        } else { // uwb::protocol::fira::DeviceType::Controlee
            controllerMacAddress = uwb::UwbMacAddress::FromString(m_cliData->destinationMacAddress, macAddressType);
            controleeMacAddress = uwb::UwbMacAddress::FromString(m_cliData->deviceMacAddress, macAddressType);
        }

        if (!controllerMacAddress.has_value()) {
            std::cerr << "Invalid ControllerMacAddress" << std::endl;
        } else {
            m_cliData->uwbConfiguration.controllerMacAddress = controllerMacAddress.value();
            // TEST
            std::cout << "ControllerMacAddress: " << m_cliData->uwbConfiguration.controllerMacAddress << std::endl;
        }

        if (!controleeMacAddress.has_value()) {
            std::cerr << "Invalid ControleeMacAddress" << std::endl;
        } else {
            m_cliData->uwbConfiguration.controleeShortMacAddress = controleeMacAddress.value();
            // TEST
            std::cout << "ControleeMacAddress: " << m_cliData->uwbConfiguration.controleeShortMacAddress << std::endl;
        }

        m_cliData->SessionData.uwbConfiguration = m_cliData->uwbConfiguration;
        m_cliData->SessionData.staticRangingInfo = m_cliData->StaticRanging;

        std::cout << "Selected parameters:" << std::endl;

        for (const auto& [parameterTag, parameterValue] : m_cliData->SessionData.uwbConfiguration.GetValueMap()) {
            std::visit([](auto&& arg) {
                using ParameterValueT = std::decay_t<decltype(arg)>;
                if constexpr (std::is_enum_v<ParameterValueT>) {
                    std::cout << magic_enum::enum_type_name<ParameterValueT>() << "::" << magic_enum::enum_name(arg) << std::endl;
                } else if constexpr (std::is_same_v<ParameterValueT, std::unordered_set<uwb::protocol::fira::ResultReportConfiguration>>) {
                    std::cout << "ResultReportConfigurations: " << uwb::protocol::fira::ResultReportConfigurationToString(arg) << std::endl;
                }
            },
                parameterValue);
        }

        std::cout << "StaticRangingInfo: { " << m_cliData->SessionData.staticRangingInfo << " }" << std::endl;
    });

    rangeStartApp->final_callback([this] {
        auto uwbDevice = GetUwbDevice();
        if (!uwbDevice) {
            std::cerr << "no device found" << std::endl;
            return;
        }
        if (!uwbDevice->Initialize()) {
            std::cerr << "device not initialized" << std::endl;
        }

        m_cliHandler->HandleStartRanging(uwbDevice, m_cliData->SessionData);
    });

    return rangeStartApp;
}

CLI::App*
NearObjectCli::AddSubcommandUwbRangeStop(CLI::App* parent)
{
    // top-level command
    auto rangeStopApp = parent->add_subcommand("stop", "stop ranging")->fallthrough();

    rangeStopApp->parse_complete_callback([this] {
        std::cout << "stop ranging" << std::endl;
        m_cliHandler->HandleStopRanging();
    });

    return rangeStopApp;
}

CLI::App*
NearObjectCli::AddSubcommandUwbRawDeviceReset(CLI::App* parent)
{
    // top-level command
    auto rawDeviceResetApp = parent->add_subcommand("devicereset", "DeviceReset")->fallthrough();

    rawDeviceResetApp->parse_complete_callback([this] {
        std::cout << "device reset" << std::endl;
    });

    rawDeviceResetApp->final_callback([this] {
        auto uwbDevice = GetUwbDevice();
        if (!uwbDevice) {
            std::cerr << "no device found" << std::endl;
            return;
        }
        if (!uwbDevice->Initialize()) {
            std::cerr << "device not initialized" << std::endl;
        }

        m_cliHandler->HandleDeviceReset(uwbDevice);
    });

    return rawDeviceResetApp;
}

CLI::App*
NearObjectCli::AddSubcommandUwbRawGetDeviceInfo(CLI::App* parent)
{
    // top-level command
    auto rawGetDeviceInfoApp = parent->add_subcommand("getdeviceinfo", "GetDeviceInfo")->fallthrough();

    rawGetDeviceInfoApp->parse_complete_callback([this] {
        std::cout << "get device info" << std::endl;
    });

    rawGetDeviceInfoApp->final_callback([this] {
        auto uwbDevice = GetUwbDevice();
        if (!uwbDevice) {
            std::cerr << "no device found" << std::endl;
            return;
        }
        if (!uwbDevice->Initialize()) {
            std::cerr << "device not initialized" << std::endl;
        }

        m_cliHandler->HandleGetDeviceInfo(uwbDevice);
    });

    return rawGetDeviceInfoApp;
}