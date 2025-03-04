
#include <bitset>
#include <optional>
#include <regex>
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
using namespace ::uwb::protocol::fira;

NearObjectCli::NearObjectCli(std::shared_ptr<NearObjectCliData> cliData, std::shared_ptr<NearObjectCliHandler> cliHandler) :
    m_cliData(cliData),
    m_cliHandler(std::move(cliHandler)),
    m_cliApp(CreateParser())
{
    m_cliHandler->SetParent(this);

    if (!m_cliApp) {
        throw std::runtime_error("failed to create command line parser");
    }
}

std::shared_ptr<NearObjectCliData>
NearObjectCli::GetData() const noexcept
{
    return m_cliData;
}

std::shared_ptr<NearObjectCliControlFlowContext>
NearObjectCli::GetControlFlowContext() const noexcept
{
    return m_cliControlFlowContext;
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

void
NearObjectCli::RegisterCliAppWithOperation(CLI::App* app)
{
    m_cliAppOperations.insert(app);
}

void
NearObjectCli::SignalCliAppOperationCompleted(CLI::App* app)
{
    std::size_t numOperationsCompleted = m_cliAppOperations.erase(app);
    if (numOperationsCompleted > 0) {
        m_cliControlFlowContext->OperationSignalComplete(numOperationsCompleted);
    }
}

void
NearObjectCli::WaitForExecutionComplete()
{
    if (m_cliAppOperations.size() > 0) {
        m_cliControlFlowContext->OperationsWaitForComplete();
    }
}

void
NearObjectCli::CancelExecution()
{
    bool stopRequested = m_cliControlFlowContext->RequestStopExecution();
    if (!stopRequested) {
        // TODO: log?
    }
}

CLI::App&
NearObjectCli::GetParser()
{
    return *m_cliApp;
}

CLI::App&
NearObjectCli::GetDriverApp() noexcept
{
    return *m_driverApp;
}

CLI::App&
NearObjectCli::GetServiceApp() noexcept
{
    return *m_serviceApp;
}

CLI::App&
NearObjectCli::GetDriverUwbApp() noexcept
{
    return *m_driverUwbApp;
}

CLI::App&
NearObjectCli::GetDriverUwbRawApp() noexcept
{
    return *m_driverUwbRawApp;
}

CLI::App&
NearObjectCli::GetDriverUwbRangeApp() noexcept
{
    return *m_driverUwbRangeApp;
}

CLI::App&
NearObjectCli::GetServiceRangeApp() noexcept
{
    return *m_serviceRangeApp;
}

CLI::App&
NearObjectCli::GetDriverUwbRangeStartApp() noexcept
{
    return *m_driverUwbRangeStartApp;
}

CLI::App&
NearObjectCli::GetDriverUwbRangeStopApp() noexcept
{
    return *m_driverUwbRangeStopApp;
}

CLI::App&
NearObjectCli::GetServiceRangeStartApp() noexcept
{
    return *m_serviceRangeStartApp;
}

CLI::App&
NearObjectCli::GetServiceRangeStopApp() noexcept
{
    return *m_serviceRangeStopApp;
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
    app->parse_complete_callback([this] {
        m_cliControlFlowContext = std::make_shared<NearObjectCliControlFlowContext>(std::size(m_cliAppOperations));
    });

    // sub-commands
    m_driverApp = AddSubcommandDriver(app.get());
    m_serviceApp = AddSubcommandService(app.get());

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
AddEnumOption(CLI::App* app, std::optional<EnumType>& assignTo, bool isMandatory = false)
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

/**
 * @brief Validates non-enum application configuration parameter inputs from nocli.
 *
 * @param cliData The nocli input data containing the application configuration parameters
 */
void
ValidateNonEnumParameterValues(NearObjectCliData& cliData)
{
    // Note: No restrictions other than type bounds checking given in FiRa UCI Generic Technical Specification v1.1.0 for the following parameters:
    // SlotDuration, SlotsPerRangingRound, VendorId, MaxRangingRoundRetry, BlockStrideLength, MaxNumberOfMeasurements, RangingInterval, StsIndex,
    // HoppingMode, StaticStsIv

    auto& parametersData = cliData.applicationConfigurationParametersData;

    // NumberOfControlees (mandatory)
    if (parametersData.multiNodeMode == MultiNodeMode::Unicast) {
        if (parametersData.numberOfControlees != MinimumNumberOfControlees) {
            std::cerr << "Invalid NumberOfControlees. Only " << +MinimumNumberOfControlees << " controlee expected in Unicast mode." << std::endl;
        }
    } else {
        if (parametersData.numberOfControlees < MinimumNumberOfControlees) {
            std::cerr << "Invalid NumberOfControlees. At least " << +MinimumNumberOfControlees << " controlees expected." << std::endl;
        }
    }

    // DeviceMacAddress (mandatory) and DestinationMacAddresses (mandatory)
    const auto macAddressType = parametersData.macAddressMode == uwb::UwbMacAddressType::Extended ? uwb::UwbMacAddressType::Extended : uwb::UwbMacAddressType::Short;

    if (!parametersData.deviceMacAddress.has_value()) {
        std::cerr << "Invalid DeviceMacAddress. Does not match format of MacAddressMode: " << ToString(macAddressType) << std::endl;
    }

    if (!parametersData.destinationMacAddresses.has_value()) {
        std::cerr << "Invalid DestinationMacAddresses. Does not match format of MacAddressMode: " << ToString(macAddressType) << std::endl;
    } else {
        if (parametersData.deviceType == DeviceType::Controller) {
            if (parametersData.destinationMacAddresses.value().size() != parametersData.numberOfControlees) {
                std::cerr << "Invalid number of DestinationMacAddresses. Should be equal to NumberOfControlees when device is a Controller." << std::endl;
            }
        } else {
            if (parametersData.destinationMacAddresses.value().size() != DestinationMacAddressesCountWhenControlee) {
                std::cerr << "Invalid number of DestinationMacAddresses. Should only contain " << DestinationMacAddressesCountWhenControlee << " mac address for the Controller when device is a Controlee." << std::endl;
            }
        }
    }

    // RangeDataNotificationProximityNear
    if (parametersData.rangeDataNotificationProximityNear.has_value()) {
        if (parametersData.rangeDataNotificationConfig.has_value() && parametersData.rangeDataNotificationConfig.value() == RangeDataNotificationConfiguration::EnableInProximityRange) {
            if ((!parametersData.rangeDataNotificationProximityFar.has_value() && parametersData.rangeDataNotificationProximityNear.value() > DefaultRangeDataNotificationProximityFar) ||
                (parametersData.rangeDataNotificationProximityFar.has_value() && parametersData.rangeDataNotificationProximityNear.value() > parametersData.rangeDataNotificationProximityFar.value())) {
                std::cerr << "Invalid RangeDataNotificationProximityNear. Should be less than or equal to RangeDataNotificationProximityFar (default: " << +DefaultRangeDataNotificationProximityFar << ")." << std::endl;
            }
        }
    }

    // RangeDataNotificationProximityFar
    if (parametersData.rangeDataNotificationProximityFar.has_value()) {
        if (parametersData.rangeDataNotificationConfig.has_value() && parametersData.rangeDataNotificationConfig.value() == RangeDataNotificationConfiguration::EnableInProximityRange) {
            if (parametersData.rangeDataNotificationProximityNear.has_value() && parametersData.rangeDataNotificationProximityFar.value() < parametersData.rangeDataNotificationProximityNear.value()) {
                std::cerr << "Invalid RangeDataNotificationProximityFar. Should be greater than or equal to RangeDataNotificationProximityNear (default: " << +DefaultRangeDataNotificationProximityNear << ")." << std::endl;
            }
        }
    }

    // PreambleCodeIndex
    if (parametersData.preambleCodeIndex.has_value()) {
        if (!parametersData.prfMode.has_value() || (parametersData.prfMode.has_value() && parametersData.prfMode.value() == PrfModeDetailed::Bprf62MHz)) { // Either BPRF is set or PRF_MODE is left at default (BPRF)
            if (parametersData.preambleCodeIndex.value() < MinimumPreambleCodeIndexBprf || parametersData.preambleCodeIndex.value() > MaximumPreambleCodeIndexBprf) {
                std::cerr << "Invalid PreambleCodeIndex. Expected value range of " << +MinimumPreambleCodeIndexBprf << "-" << +MaximumPreambleCodeIndexBprf << " in BPRF mode." << std::endl;
            }
        } else { // HPRF mode
            if (parametersData.preambleCodeIndex.value() < MinimumPreambleCodeIndexHprf || parametersData.preambleCodeIndex.value() > MaximumPreambleCodeIndexHprf) {
                std::cerr << "Invalid PreambleCodeIndex. Expected value range of " << +MinimumPreambleCodeIndexHprf << "-" << +MaximumPreambleCodeIndexHprf << " in BPRF mode." << std::endl;
            }
        }
    }

    // SfdId
    if (parametersData.sfdId.has_value()) {
        if (!parametersData.prfMode.has_value() || (parametersData.prfMode.has_value() && parametersData.prfMode.value() == PrfModeDetailed::Bprf62MHz)) { // Either BPRF is set or PRF_MODE is left at default (BPRF)
            if (std::ranges::find(SfdIdValuesBprf, parametersData.sfdId.value()) == std::ranges::end(SfdIdValuesBprf)) {
                std::string errorMessage = "Invalid SfdId. Expected values of { ";
                for (const auto& val : SfdIdValuesBprf) {
                    errorMessage.append(std::to_string(val) + " ");
                }
                errorMessage.append("} in BPRF mode.");
                std::cerr << errorMessage << std::endl;
            }
        } else { // HPRF mode
            if (std::ranges::find(SfdIdValuesHprf, parametersData.sfdId.value()) == std::ranges::end(SfdIdValuesHprf)) {
                std::string errorMessage = "Invalid SfdId. Expected values of { ";
                for (const auto& val : SfdIdValuesHprf) {
                    errorMessage.append(std::to_string(val) + " ");
                }
                errorMessage.append("} in HPRF mode.");
                std::cerr << errorMessage << std::endl;
            }
        }
    }

    // ResponderSlotIndex
    if (parametersData.responderSlotIndex.has_value() && parametersData.responderSlotIndex.value() < MinimumResponderSlotIndex) { // TODO: > N, where N is number of Responders
        std::cerr << "Invalid ResponderSlotIndex. Expected value range of " << +MinimumResponderSlotIndex << "- N Responders." << std::endl;
    }

    // KeyRotationRate
    if (parametersData.keyRotationRate.has_value() && parametersData.keyRotationRate.value() > MaximumKeyRotationRate) {
        std::cerr << "Invalid KeyRotationRate. Expected value range of " << +MinimumKeyRotationRate << "-" << +MaximumKeyRotationRate << std::endl;
    }

    // SessionPriority
    if (parametersData.sessionPriority.has_value() && (parametersData.sessionPriority.value() < MinimumSessionPriority || parametersData.sessionPriority.value() > MaximumSessionPriority)) {
        std::cerr << "Invalid SessionPriority. Expected value range of " << +MinimumSessionPriority << "-" << +MaximumSessionPriority << std::endl;
    }

    // NumberOfStsSegments
    if (parametersData.numberOfStsSegments.has_value()) {
        if (parametersData.numberOfStsSegments.value() > MaximumNumberOfStsSegmentsHprf) {
            std::cerr << "Invalid NumberOfStsSegments. Expected value range of " << +MinimumNumberOfStsSegments << "-" << +MaximumNumberOfStsSegmentsHprf << " STS segments." << std::endl;
        }
        if (!parametersData.prfMode.has_value() || (parametersData.prfMode.has_value() && parametersData.prfMode.value() == PrfModeDetailed::Bprf62MHz)) { // Either BPRF is set or PRF_MODE is left at default (BPRF)
            if (parametersData.numberOfStsSegments.value() > MaximumNumberOfStsSegmentsBprf) {
                std::cerr << "Invalid NumberOfStsSegments. Expected value range of " << +MinimumNumberOfStsSegments << "-" << +MaximumNumberOfStsSegmentsBprf << " in BPRF mode." << std::endl;
            }
        }
        if (parametersData.rFrameConfiguration.value() == StsPacketConfiguration::SP0 && parametersData.numberOfStsSegments != MinimumNumberOfStsSegments) {
            std::cerr << "Invalid NumberOfStsSegments. No STS segments expected with non-STS frames" << std::endl;
        }
    }

    // UwbInitiationTime
    if (parametersData.uwbInitiationTime.has_value() && parametersData.uwbInitiationTime.value() > MaximumUwbInitiationTime) {
        std::cerr << "Invalid UwbInitiationTime. Expected value range of " << +MinimumUwbInitiationTime << "-" << +MaximumUwbInitiationTime << std::endl;
    }

    // ResultReportConfig
    constexpr int resultReportConfigurationSize = magic_enum::enum_count<ResultReportConfiguration>();
    auto IsValidResultReportConfigurationString = [resultReportConfigurationSize](const std::string& resultReportConfigurationString) {
        if (resultReportConfigurationString.empty()) {
            return false;
        } else if (!resultReportConfigurationString.empty() && resultReportConfigurationString.length() != resultReportConfigurationSize) {
            std::cerr << "Invalid ResultReportConfiguration length" << std::endl;
            return false;
        }
        return std::ranges::all_of(resultReportConfigurationString, [](const auto& c) {
            return c == '0' || c == '1';
        });
    };
    if (IsValidResultReportConfigurationString(cliData.resultReportConfigurationString)) {
        parametersData.resultReportConfig.emplace();

        const std::bitset<resultReportConfigurationSize> resultReportConfigurationBits(cliData.resultReportConfigurationString);
        for (auto i = 0; i < resultReportConfigurationSize; i++) {
            if (resultReportConfigurationBits.test(i)) {
                parametersData.resultReportConfig.value().insert(magic_enum::enum_value<ResultReportConfiguration>(i));
            }
        }
    }

    // InBandTerminationAttemptCount
    if (parametersData.inBandTerminationAttemptCount.has_value() && parametersData.inBandTerminationAttemptCount.value() > MaximumInBandTerminationAttemptCount) {
        std::cerr << "Invalid InBandTerminationAttemptCount. Expected value range of " << +MinimumInBandTerminationAttemptCount << "-" << +MaximumInBandTerminationAttemptCount << std::endl;
    }

    // SubSessionId
    if (parametersData.stsConfiguration.has_value() && parametersData.stsConfiguration.value() == StsConfiguration::DynamicWithResponderSubSessionKey) {
        if (!parametersData.subSessionId.has_value()) {
            std::cerr << "SubSessionId is required for Dynamic STS with Responder Sub-Session Key" << std::endl;
        }
    }

    // BprfPhrDataRate
    if (parametersData.prfMode.has_value() && parametersData.prfMode.value() != PrfModeDetailed::Bprf62MHz) {
        if (parametersData.bprfPhrDataRate.has_value()) {
            std::cerr << "Invalid BprfPhrDataRate. Value expected only in BPRF mode" << std::endl;
        }
    } else {
        if (parametersData.bprfPhrDataRate.has_value() && !magic_enum::enum_contains<BprfPhrDataRate>(parametersData.bprfPhrDataRate.value())) {
            std::cerr << "Invalid BprfPhrDataRate" << std::endl;
        }
    }
}

/**
 * @brief Validates the enum application configuration parameter inputs from nocli.
 *
 * @tparam EnumType The type of the enumeration.
 * @param applicationConfigurationParameter The application configuration parameter of type EnumType.
 */
template <typename EnumType>
// clang-format off
requires std::is_enum_v<EnumType>
void
ValidateEnumParameterValue(const EnumType& applicationConfigurationParameter)
// clang-format on
{
    auto parameterType = magic_enum::enum_type_name<EnumType>();
    if (!magic_enum::enum_contains<EnumType>(applicationConfigurationParameter)) {
        std::cerr << "Invalid " << parameterType << std::endl;
    }
}

/**
 * @brief Validates, displays, and returns application configuration parameters.
 *
 * @param cliData The nocli input data containing the application configuration parameters.
 * @return std::vector<uwb::protocol::fira::UwbApplicationConfigurationParameter>.
 */
std::vector<uwb::protocol::fira::UwbApplicationConfigurationParameter>
ProcessApplicationConfigurationParameters(NearObjectCliData& cliData)
{
    detail::ValidateNonEnumParameterValues(cliData);
    auto& applicationConfigurationParameterData = cliData.applicationConfigurationParametersData;

    std::vector<uwb::protocol::fira::UwbApplicationConfigurationParameter> applicationConfigurationParameters;

    std::cout << "Selected parameters:" << std::endl;
    const auto applicationConfigurationParameterValues = applicationConfigurationParameterData.GetValueMap();
    for (const auto& [applicationConfigurationParameter, applicationConfigurationParameterValue] : applicationConfigurationParameterValues) {
        std::visit([&applicationConfigurationParameter, &applicationConfigurationParameters](auto&& arg) {
            applicationConfigurationParameters.push_back({ applicationConfigurationParameter, arg });

            using ParameterValueT = std::decay_t<decltype(arg)>;
            std::ostringstream oss;
            oss << magic_enum::enum_name(applicationConfigurationParameter) << "::";
            if constexpr (std::is_enum_v<ParameterValueT>) {
                ValidateEnumParameterValue(arg);
                oss << magic_enum::enum_name(arg);
            } else if constexpr (std::is_same_v<ParameterValueT, bool>) {
                oss << std::boolalpha << arg;
            } else if constexpr (std::is_unsigned_v<ParameterValueT>) {
                oss << +arg;
            } else if constexpr (std::is_same_v<ParameterValueT, ::uwb::UwbMacAddress>) {
                oss << ToString(arg);
            } else if constexpr (std::is_same_v<ParameterValueT, std::unordered_set<::uwb::UwbMacAddress>>) {
                oss << ToString(arg);
            } else if constexpr (std::is_same_v<ParameterValueT, std::unordered_set<ResultReportConfiguration>>) {
                oss << ToString(arg);
            } else if constexpr (std::is_same_v<ParameterValueT, std::array<uint8_t, StaticStsInitializationVectorLength>>) {
                for (const auto& value : arg) {
                    oss << "0x" << std::setw(2) << std::internal << std::setfill('0') << std::hex << +value << " ";
                }
            }

            std::cout << oss.str() << std::endl;
        },
            applicationConfigurationParameterValue);
    }

    return applicationConfigurationParameters;
}

std::optional<std::unordered_set<UwbMacAddress>>
MacAddressesFromString(const std::string& addressesString, ::uwb::UwbMacAddressType addressType)
{
    const std::regex shortRegex("^([0-9A-Fa-f]{2}:){1}([0-9A-Fa-f]{2})(,([0-9A-Fa-f]{2}:){1}([0-9A-Fa-f]{2}))*$");
    const std::regex extendedRegex("^([0-9A-Fa-f]{2}:){7}([0-9A-Fa-f]{2})(,([0-9A-Fa-f]{2}:){7}([0-9A-Fa-f]{2}))*$");

    if ((addressType == ::uwb::UwbMacAddressType::Short && !std::regex_match(addressesString, shortRegex)) ||
        (addressType == ::uwb::UwbMacAddressType::Extended && !std::regex_match(addressesString, extendedRegex))) {
        return std::nullopt;
    }

    std::unordered_set<UwbMacAddress> macAddresses{};
    std::stringstream ss(addressesString);

    std::string macAddressString;
    while (std::getline(ss, macAddressString, ',')) {
        auto macAddress = UwbMacAddress::FromString(macAddressString, addressType);
        if (macAddress.has_value()) {
            macAddresses.insert(macAddress.value());
        }
    }

    return macAddresses;
}

} // namespace detail

CLI::App*
NearObjectCli::AddSubcommandDriver(CLI::App* parent)
{
    // top-level command
    auto driverApp = parent->add_subcommand("driver", "Commands related to NearObject technology driver testing")->require_subcommand()->fallthrough();

    // sub-commands
    m_driverUwbApp = AddSubcommandDriverUwb(driverApp);

    return driverApp;
}

CLI::App*
NearObjectCli::AddSubcommandService(CLI::App* parent)
{
    // top-level command
    auto serviceApp = parent->add_subcommand("service", "Commands related to NearObject service testing")->require_subcommand()->fallthrough();

    // sub-commands
    m_serviceMonitorApp = AddSubcommandServiceMonitor(serviceApp);
    m_serviceRangeApp = AddSubcommandServiceRange(serviceApp);

    return serviceApp;
}

CLI::App*
NearObjectCli::AddSubcommandDriverUwb(CLI::App* parent)
{
    // top-level command
    auto uwbApp = parent->add_subcommand("uwb", "Commands related to UWB driver testing")->require_subcommand()->fallthrough();

    // sub-commands
    m_driverUwbRawApp = AddSubcommandDriverUwbRaw(uwbApp);
    m_driverUwbRangeApp = AddSubcommandDriverUwbRange(uwbApp);

    return uwbApp;
}

CLI::App*
NearObjectCli::AddSubcommandDriverUwbRaw(CLI::App* parent)
{
    // top-level command
    auto rawApp = parent->add_subcommand("raw", "Individual commands for UWB driver testing")->require_subcommand()->fallthrough();

    // sub-commands
    AddSubcommandDriverUwbRawDeviceReset(rawApp);
    AddSubcommandDriverUwbRawGetDeviceInfo(rawApp);
    AddSubcommandDriverUwbRawSessionDeinitialize(rawApp);
    AddSubcommandDriverUwbRawGetSessionCount(rawApp);
    AddSubcommandDriverUwbRawGetSessionState(rawApp);

    return rawApp;
}

CLI::App*
NearObjectCli::AddSubcommandDriverUwbRange(CLI::App* parent)
{
    // top-level command
    auto rangeApp = parent->add_subcommand("range", "Commands related to UWB ranging")->require_subcommand()->fallthrough();

    // options
    rangeApp->add_option("--SessionId", m_cliData->RangingParameters.SessionId)->required();

    // sub-commands
    m_driverUwbRangeStartApp = AddSubcommandDriverUwbRangeStart(rangeApp);
    m_driverUwbRangeStopApp = AddSubcommandDriverUwbRangeStop(rangeApp);

    return rangeApp;
}

CLI::App*
NearObjectCli::AddSubcommandServiceMonitor(CLI::App* parent)
{
    auto monitorApp = parent->add_subcommand("monitor", "Commands related to NearObject service monitor mode")->fallthrough();

    monitorApp->parse_complete_callback([this, monitorApp] {
        RegisterCliAppWithOperation(monitorApp);
    });
    monitorApp->final_callback([this, monitorApp] {
        m_cliHandler->HandleMonitorMode();
        SignalCliAppOperationCompleted(monitorApp);
    });

    return monitorApp;
}

CLI::App*
NearObjectCli::AddSubcommandServiceRange(CLI::App* parent)
{
    // top-level command
    auto rangeApp = parent->add_subcommand("range", "Commands related to NearObject service ranging")->require_subcommand()->fallthrough();

    // options
    rangeApp->add_option("--SessionDataVersion", m_cliData->SessionData.sessionDataVersion);
    rangeApp->add_option("--SessionId", m_cliData->SessionData.sessionId);
    rangeApp->add_option("--SubSessionId", m_cliData->SessionData.subSessionId);

    // sub-commands
    m_serviceRangeStartApp = AddSubcommandServiceRangeStart(rangeApp);
    m_serviceRangeStopApp = AddSubcommandServiceRangeStop(rangeApp);

    return rangeApp;
}

CLI::App*
NearObjectCli::AddSubcommandDriverUwbRawDeviceReset(CLI::App* parent)
{
    // top-level command
    auto rawDeviceResetApp = parent->add_subcommand("devicereset", "Reset the UWB device")->fallthrough();

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
NearObjectCli::AddSubcommandDriverUwbRawGetDeviceInfo(CLI::App* parent)
{
    // top-level command
    auto rawGetDeviceInfoApp = parent->add_subcommand("getdeviceinfo", "Get the UWB device info")->fallthrough();

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

CLI::App*
NearObjectCli::AddSubcommandDriverUwbRawSessionDeinitialize(CLI::App* parent)
{
    // top-level command
    auto rawSessionDeinitializeApp = parent->add_subcommand("sessiondeinit", "Deinitialize a pre-existing session")->fallthrough();
    rawSessionDeinitializeApp->add_option("Session Id, --SessionId", m_cliData->SessionId)->required();

    rawSessionDeinitializeApp->parse_complete_callback([this, rawSessionDeinitializeApp] {
        RegisterCliAppWithOperation(rawSessionDeinitializeApp);
        std::cout << "deinitialize session " << m_cliData->SessionId << std::endl;
    });

    rawSessionDeinitializeApp->final_callback([this, rawSessionDeinitializeApp] {
        auto uwbDevice = GetUwbDevice();
        if (!uwbDevice) {
            std::cerr << "no device found" << std::endl;
            return;
        }
        if (!uwbDevice->Initialize()) {
            std::cerr << "device not initialized" << std::endl;
        }

        m_cliHandler->HandleSessionDeinitialize(uwbDevice, m_cliData->SessionId);

        SignalCliAppOperationCompleted(rawSessionDeinitializeApp);
    });

    return rawSessionDeinitializeApp;
}

CLI::App*
NearObjectCli::AddSubcommandDriverUwbRawGetSessionCount(CLI::App* parent)
{
    // top-level command
    auto rawGetSessionCountApp = parent->add_subcommand("getsessioncount", "Get the number of sessions")->fallthrough();

    rawGetSessionCountApp->parse_complete_callback([this, rawGetSessionCountApp] {
        RegisterCliAppWithOperation(rawGetSessionCountApp);
        std::cout << "get session count" << std::endl;
    });

    rawGetSessionCountApp->final_callback([this, rawGetSessionCountApp] {
        auto uwbDevice = GetUwbDevice();
        if (!uwbDevice) {
            std::cerr << "no device found" << std::endl;
            return;
        }
        if (!uwbDevice->Initialize()) {
            std::cerr << "device not initialized" << std::endl;
        }

        m_cliHandler->HandleGetSessionCount(uwbDevice);

        SignalCliAppOperationCompleted(rawGetSessionCountApp);
    });

    return rawGetSessionCountApp;
}

CLI::App*
NearObjectCli::AddSubcommandDriverUwbRawGetSessionState(CLI::App* parent)
{
    // top-level command
    auto rawGetSessionStateApp = parent->add_subcommand("getsessionstate", "Get the current state of a session")->fallthrough();
    rawGetSessionStateApp->add_option("Session Id, --SessionId", m_cliData->SessionId)->required();

    rawGetSessionStateApp->parse_complete_callback([this, rawGetSessionStateApp] {
        RegisterCliAppWithOperation(rawGetSessionStateApp);
        std::cout << "get session state of session " << m_cliData->SessionId << std::endl;
    });

    rawGetSessionStateApp->final_callback([this, rawGetSessionStateApp] {
        auto uwbDevice = GetUwbDevice();
        if (!uwbDevice) {
            std::cerr << "no device found" << std::endl;
            return;
        }
        if (!uwbDevice->Initialize()) {
            std::cerr << "device not initialized" << std::endl;
        }

        m_cliHandler->HandleGetSessionState(uwbDevice, m_cliData->SessionId);

        SignalCliAppOperationCompleted(rawGetSessionStateApp);
    });

    return rawGetSessionStateApp;
}

CLI::App*
NearObjectCli::AddSubcommandDriverUwbRangeStart(CLI::App* parent)
{
    auto& applicationConfigurationParametersData = m_cliData->applicationConfigurationParametersData;
    auto rangeStartApp = parent->add_subcommand("start", "Start a UWB ranging session")->fallthrough();

    // Remove labels from options
    rangeStartApp->get_formatter()->label("ENUM", "");
    rangeStartApp->get_formatter()->label("TEXT", "");
    rangeStartApp->get_formatter()->label("UINT", "");
    rangeStartApp->get_formatter()->label("[UINT,UINT,UINT,UINT,UINT,UINT]", "");

    // List mandatory params first
    detail::AddEnumOption(rangeStartApp, applicationConfigurationParametersData.deviceRole, true);
    detail::AddEnumOption(rangeStartApp, applicationConfigurationParametersData.multiNodeMode, true);
    rangeStartApp->add_option("--NumberOfControlees", applicationConfigurationParametersData.numberOfControlees, "1-byte integer. Value in { 1-8 }")->capture_default_str()->required();
    rangeStartApp->add_option("--DeviceMacAddress", m_cliData->deviceMacAddressString, "2-byte/8-byte hexadecimal value, colon-delimited. Short/Extended MAC address of own device, e.g. 12:34")->capture_default_str()->required();
    rangeStartApp->add_option("--DestinationMacAddresses", m_cliData->destinationMacAddressesString, "Comma-delimited array with 2-byte/8-byte hexadecimal values, colon-delimited. Short/Extended MAC address(es) of other device(s). If device is Controller, list NumberOfControlees mac addresses. If device is Controlee, list Controller mac address, e.g. 12:34,56:78")->capture_default_str()->required();
    detail::AddEnumOption(rangeStartApp, applicationConfigurationParametersData.deviceType, true);

    // List remaining params
    // uint8_t
    rangeStartApp->add_option("--BlockStrideLength", applicationConfigurationParametersData.blockStrideLength, "1-byte integer. Value in { 0(No block striding), 1-MAX(Number of ranging blocks to be skipped) }")->capture_default_str();
    rangeStartApp->add_option("--InBandTerminationAttemptCount", applicationConfigurationParametersData.inBandTerminationAttemptCount, "1-byte integer. Value in { 0(Disable), 1-10(In-band termination attempt count) }")->capture_default_str();
    rangeStartApp->add_option("--KeyRotationRate", applicationConfigurationParametersData.keyRotationRate, "1-byte integer. Exponent n where 2^n is the key rotation rate. Value in { 0-15 }")->capture_default_str();
    rangeStartApp->add_option("--NumberOfStsSegments", applicationConfigurationParametersData.numberOfStsSegments, "1-byte integer. Value in { 0-4 }. Note: 2-4 for HPRF Mode only")->capture_default_str();
    rangeStartApp->add_option("--PreambleCodeIndex", applicationConfigurationParametersData.preambleCodeIndex, "1-byte integer. Value in { 9-12(BPRF), 25-32(HPRF) }")->capture_default_str();
    rangeStartApp->add_option("--ResponderSlotIndex", applicationConfigurationParametersData.responderSlotIndex, "1-byte integer. Responder index in TWR. Value in { 1-N(Number of responders) }")->capture_default_str();
    rangeStartApp->add_option("--SessionPriority", applicationConfigurationParametersData.sessionPriority, "1-byte integer. Value in { 1-100 }")->capture_default_str();
    rangeStartApp->add_option("--SfdId", applicationConfigurationParametersData.sfdId, "1-byte integer. Value in { 0, 2 } for BPRF Mode; { 1-4 } for HPRF Mode")->capture_default_str();
    rangeStartApp->add_option("--SlotsPerRangingRound", applicationConfigurationParametersData.slotsPerRangingRound, "1-byte integer. Value in { 0-MAX }")->capture_default_str();

    // uint16_t
    rangeStartApp->add_option("--MaxNumberOfMeasurements", applicationConfigurationParametersData.maxNumberOfMeasurements, "2-byte integer. Value in { 0(Unlimited), 1-MAX(Max number of ranging measurements in a session) }")->capture_default_str();
    rangeStartApp->add_option("--MaxRangingRoundRetry", applicationConfigurationParametersData.maxRangingRoundRetry, "2-byte integer. Number of failed RR attempts before stopping the session. Value in { 0-MAX }")->capture_default_str();
    rangeStartApp->add_option("--RangeDataNotificationProximityFar", applicationConfigurationParametersData.rangeDataNotificationProximityFar, "2-byte integer. Upper bound in cm for ranging proximity mode. Value in { RangeDataNotificationProximityNear-MAX }")->capture_default_str();
    rangeStartApp->add_option("--RangeDataNotificationProximityNear", applicationConfigurationParametersData.rangeDataNotificationProximityNear, "2-byte integer. Lower bound in cm for ranging proximity mode. Value in { 0-RangeDataNotificationProximityFar }")->capture_default_str();
    rangeStartApp->add_option("--SlotDuration", applicationConfigurationParametersData.slotDuration, "2-byte integer. Duration of a ranging slot in the unit of RSTU. Value in { 0-MAX }")->capture_default_str();
    rangeStartApp->add_option("--VendorId", applicationConfigurationParametersData.vendorId, "2-byte hexadecimal value. Unique ID for vendor. Used for static STS")->capture_default_str();

    // uint32_t
    rangeStartApp->add_option("--RangingInterval", applicationConfigurationParametersData.rangingInterval, "4-byte integer. Ranging interval in the unit of 1200 RSTU (1ms) between ranging rounds. Value in { Duration of one ranging round-MAX }")->capture_default_str();
    rangeStartApp->add_option("--StsIndex", applicationConfigurationParametersData.stsIndex, "4-byte hexadecimal value. Test Mode only.")->capture_default_str();
    rangeStartApp->add_option("--SubSessionId", applicationConfigurationParametersData.subSessionId, "4-byte hexadecimal value. Sub-session ID for the controlee device. Required for Dynamic STS with responder specific sub-session key")->capture_default_str();
    rangeStartApp->add_option("--UwbInitiationTime", applicationConfigurationParametersData.uwbInitiationTime, "4-byte integer. UWB initiation time in the unit of 1200 RSTU (1ms). Value in { 0-10000 }")->capture_default_str();

    // bool
    rangeStartApp->add_flag("--HoppingMode", applicationConfigurationParametersData.hoppingMode, "Flag. Setting this enables FiRa hopping");

    // enums
    detail::AddEnumOption(rangeStartApp, applicationConfigurationParametersData.aoaResultRequest);
    detail::AddEnumOption(rangeStartApp, applicationConfigurationParametersData.bprfPhrDataRate);
    detail::AddEnumOption(rangeStartApp, applicationConfigurationParametersData.channelNumber);
    detail::AddEnumOption(rangeStartApp, applicationConfigurationParametersData.keyRotation);
    detail::AddEnumOption(rangeStartApp, applicationConfigurationParametersData.macAddressMode);
    detail::AddEnumOption(rangeStartApp, applicationConfigurationParametersData.macFcsType);
    detail::AddEnumOption(rangeStartApp, applicationConfigurationParametersData.preambleDuration);
    detail::AddEnumOption(rangeStartApp, applicationConfigurationParametersData.prfMode);
    detail::AddEnumOption(rangeStartApp, applicationConfigurationParametersData.rangeDataNotificationConfig);
    detail::AddEnumOption(rangeStartApp, applicationConfigurationParametersData.rangingRoundControl);
    detail::AddEnumOption(rangeStartApp, applicationConfigurationParametersData.rFrameConfiguration);
    detail::AddEnumOption(rangeStartApp, applicationConfigurationParametersData.scheduledMode);
    detail::AddEnumOption(rangeStartApp, applicationConfigurationParametersData.stsConfiguration);
    detail::AddEnumOption(rangeStartApp, applicationConfigurationParametersData.stsLength);
    detail::AddEnumOption(rangeStartApp, applicationConfigurationParametersData.txAdaptivePayloadPower);

    // other
    rangeStartApp->add_option("--ResultReportConfig", m_cliData->resultReportConfigurationString, "4-bit value, encoded as bit string, e.g. 0101. b3(AOA FOM), b2(AOA Elevation), b1(AOA Azimuth), b0(TOF)")->capture_default_str();
    rangeStartApp->add_option("--StaticStsInitializationVector", applicationConfigurationParametersData.staticStsIv, "6-byte hexadecimal value, colon-delimited. Vendor-defined static STS initialization vector, e.g. 11:22:33:44:55:66")->delimiter(':');

    rangeStartApp->parse_complete_callback([this, rangeStartApp] {
        auto& applicationConfigurationParametersData = m_cliData->applicationConfigurationParametersData;

        // Set DeviceMacAddress and DestinationMacAddresses
        const auto macAddressType = applicationConfigurationParametersData.macAddressMode == uwb::UwbMacAddressType::Extended ? uwb::UwbMacAddressType::Extended : uwb::UwbMacAddressType::Short;
        applicationConfigurationParametersData.deviceMacAddress = uwb::UwbMacAddress::FromString(m_cliData->deviceMacAddressString, macAddressType);
        applicationConfigurationParametersData.destinationMacAddresses = detail::MacAddressesFromString(m_cliData->destinationMacAddressesString, macAddressType);

        m_cliData->RangingParameters.ApplicationConfigurationParameters = detail::ProcessApplicationConfigurationParameters(*m_cliData);
        RegisterCliAppWithOperation(rangeStartApp);
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

        m_cliHandler->HandleDriverStartRanging(uwbDevice, m_cliData->RangingParameters);
    });

    return rangeStartApp;
}

CLI::App*
NearObjectCli::AddSubcommandDriverUwbRangeStop(CLI::App* parent)
{
    // top-level command
    auto rangeStopApp = parent->add_subcommand("stop", "Stop a UWB ranging session")->fallthrough();

    rangeStopApp->parse_complete_callback([this, rangeStopApp] {
        std::cout << "stop ranging" << std::endl;
        RegisterCliAppWithOperation(rangeStopApp);
    });

    rangeStopApp->final_callback([this, rangeStopApp] {
        m_cliHandler->HandleStopRanging();
        SignalCliAppOperationCompleted(rangeStopApp);
    });

    return rangeStopApp;
}

CLI::App*
NearObjectCli::AddSubcommandServiceRangeStart(CLI::App* parent)
{
    auto& uwbConfig = m_cliData->uwbConfiguration;
    auto rangeStartApp = parent->add_subcommand("start", "Start a NearObject ranging session")->fallthrough();

    // Remove labels from options
    rangeStartApp->get_formatter()->label("ENUM", "");
    rangeStartApp->get_formatter()->label("TEXT", "");
    rangeStartApp->get_formatter()->label("UINT", "");
    rangeStartApp->get_formatter()->label("[UINT,UINT,UINT,UINT,UINT,UINT]", "");

    // List mandatory params first
    detail::AddEnumOption(rangeStartApp, uwbConfig.deviceRole, true);
    detail::AddEnumOption(rangeStartApp, uwbConfig.multiNodeMode, true);
    rangeStartApp->add_option("--NumberOfControlees", uwbConfig.numberOfControlees, "1-byte integer. Value in { 1-8 }")->capture_default_str()->required();
    rangeStartApp->add_option("--ControllerMacAddress", m_cliData->deviceMacAddressString, "2-byte/8-byte hexadecimal value, colon-delimited. Short/Extended MAC address of Controller, e.g. 12:34")->capture_default_str()->required();
    rangeStartApp->add_option("--ControleeMacAddresses", m_cliData->destinationMacAddressesString, "Comma-delimited array with 2-byte/8-byte hexadecimal values, colon-delimited. Short/Extended MAC address(es) of Controlee(s), e.g. 12:34,56:78")->capture_default_str()->required();
    detail::AddEnumOption(rangeStartApp, uwbConfig.deviceType, true);

    // List remaining params
    // uint8_t
    rangeStartApp->add_option("--Sp0PhySetNumber", uwbConfig.sp0PhySetNumber, "1-byte integer. Value in { 0, 2 } for BPRF Mode; { 1-4, 32 } for HPRF Mode")->capture_default_str();
    rangeStartApp->add_option("--Sp1PhySetNumber", uwbConfig.sp1PhySetNumber, "1-byte integer. Value in { 3, 5 } for BPRF Mode; { 5-19, 33-34 } for HPRF Mode")->capture_default_str();
    rangeStartApp->add_option("--Sp3PhySetNumber", uwbConfig.sp3PhySetNumber, "1-byte integer. Value in { 4, 6 } for BPRF Mode; { 20-31, 35 } for HPRF Mode")->capture_default_str();
    rangeStartApp->add_option("--PreambleCodeIndex", uwbConfig.preambleCodeIndex, "1-byte integer. Value in { 9-12 } for BPRF Mode; { 25-32 } for HPRF Mode")->capture_default_str();
    rangeStartApp->add_option("--SlotsPerRangingRound", uwbConfig.slotsPerRangingRound, "1-byte integer. Value in { 0-MAX }")->capture_default_str();
    rangeStartApp->add_option("--MaxContentionPhaseLength", uwbConfig.maxContentionPhaseLength, "1-byte integer. Value in { 0-MAX }")->capture_default_str();
    rangeStartApp->add_option("--KeyRotationRate", uwbConfig.keyRotationRate, "1-byte integer. Exponent n where 2^n is the key rotation rate. Value in { 0-15 }")->capture_default_str();

    // uint16_t
    rangeStartApp->add_option("--RangingInterval", uwbConfig.rangingInterval, "2-byte integer. Ranging interval in the unit of 1200 RSTU (1ms) between ranging rounds. Value in { Duration of one ranging round-MAX }")->capture_default_str();
    rangeStartApp->add_option("--MaxRangingRoundRetry", uwbConfig.maxRangingRoundRetry, "2-byte integer. Number of failed RR attempts before stopping the session. Value in { 0-MAX }")->capture_default_str();
    rangeStartApp->add_option("--StaticRangingInfoVendorId", m_cliData->StaticRanging.VendorId, "2-byte hexadecimal value. Unique ID for vendor. Used for static STS. If --SecureRangingInfo* options are used, this option will be overridden");
    rangeStartApp->add_option("--SlotDuration", uwbConfig.slotDuration, "2-byte integer. Duration of a ranging slot in the unit of RSTU. Value in { 0-MAX }")->capture_default_str();

    // uint32_t
    rangeStartApp->add_option("--UwbInitiationTime", uwbConfig.uwbInitiationTime, "4-byte integer. UWB initiation time in the unit of 1200 RSTU (1ms). Value in { 0-10000 }")->capture_default_str();

    // booleans
    rangeStartApp->add_flag("--HoppingMode", uwbConfig.hoppingMode, "Flag. Setting this enables FiRa hopping")->capture_default_str();
    rangeStartApp->add_flag("--BlockStriding", uwbConfig.blockStriding, "Flag. Setting this enables block striding")->capture_default_str();

    // enums
    detail::AddEnumOption(rangeStartApp, uwbConfig.rangingDirection);
    detail::AddEnumOption(rangeStartApp, uwbConfig.rangingMeasurementReportMode);
    detail::AddEnumOption(rangeStartApp, uwbConfig.stsConfiguration);
    detail::AddEnumOption(rangeStartApp, uwbConfig.rangingTimeStruct);
    detail::AddEnumOption(rangeStartApp, uwbConfig.schedulingMode);
    detail::AddEnumOption(rangeStartApp, uwbConfig.channel);
    detail::AddEnumOption(rangeStartApp, uwbConfig.rframeConfig);
    detail::AddEnumOption(rangeStartApp, uwbConfig.convolutionalCodeConstraintLength);
    detail::AddEnumOption(rangeStartApp, uwbConfig.prfMode);
    detail::AddEnumOption(rangeStartApp, uwbConfig.macAddressMode);
    detail::AddEnumOption(rangeStartApp, uwbConfig.macAddressFcsType);

    // other
    rangeStartApp->add_option("--StaticRangingInfoInitializationVector", m_cliData->StaticRanging.InitializationVector, "6-byte hexadecimal value, colon-delimited. Vendor-defined static STS initialization vector, e.g. 11:22:33:44:55:66. If --SecureRangingInfo* options are used, this option will be overridden")->delimiter(':');
    rangeStartApp->add_option("--FiraPhyVersion", uwbConfig.firaPhyVersionString, "4-character string value. FiRa PHY version to be used, e.g. 0101 = Version 1.1")->capture_default_str();
    rangeStartApp->add_option("--FiraMacVersion", uwbConfig.firaMacVersionString, "4-character string value. FiRa MAC version to be used, e.g. 0101 = Version 1.1")->capture_default_str();
    rangeStartApp->add_option("--ResultReportConfiguration", uwbConfig.resultReportConfigurationString, "4-bit value, encoded as bit string, e.g. 0101. b3(AOA FOM), b2(AOA Elevation), b1(AOA Azimuth), b0(TOF)")->capture_default_str();

    rangeStartApp->parse_complete_callback([this, rangeStartApp] {
        // Set ControllerMacAddress and ControleeMacAddresses
        const auto macAddressType = m_cliData->uwbConfiguration.macAddressMode == uwb::UwbMacAddressType::Extended ? uwb::UwbMacAddressType::Extended : uwb::UwbMacAddressType::Short;
        // TODO: Support multiple controlees as well as extended controlee mac addresses
        if (m_cliData->uwbConfiguration.deviceType == DeviceType::Controller) {
            m_cliData->uwbConfiguration.controllerMacAddress = uwb::UwbMacAddress::FromString(m_cliData->deviceMacAddressString, macAddressType);
            m_cliData->uwbConfiguration.controleeShortMacAddress = uwb::UwbMacAddress::FromString(m_cliData->destinationMacAddressesString, macAddressType);
        } else {
            m_cliData->uwbConfiguration.controllerMacAddress = uwb::UwbMacAddress::FromString(m_cliData->destinationMacAddressesString, macAddressType);
            m_cliData->uwbConfiguration.controleeShortMacAddress = uwb::UwbMacAddress::FromString(m_cliData->deviceMacAddressString, macAddressType);
        }

        m_cliData->SessionData.uwbConfiguration = m_cliData->uwbConfiguration;
        m_cliData->SessionData.staticRangingInfo = m_cliData->StaticRanging;

        std::cout << "Selected parameters:" << std::endl;
        const auto parameterValues = m_cliData->SessionData.uwbConfiguration.GetValueMap();
        for (const auto& [parameterTag, parameterValue] : parameterValues) {
            std::visit([&parameterTag](auto&& arg) {
                using ParameterValueT = std::decay_t<decltype(arg)>;
                std::ostringstream oss;
                oss << magic_enum::enum_name(parameterTag) << "::";
                if constexpr (std::is_enum_v<ParameterValueT>) {
                    detail::ValidateEnumParameterValue(arg);
                    oss << magic_enum::enum_name(arg);
                } else if constexpr (std::is_same_v<ParameterValueT, bool>) {
                    oss << std::boolalpha << arg;
                } else if constexpr (std::is_unsigned_v<ParameterValueT>) {
                    oss << +arg;
                } else if constexpr (std::is_same_v<ParameterValueT, ::uwb::UwbMacAddress>) {
                    oss << ToString(arg);
                } else if constexpr (std::is_same_v<ParameterValueT, std::unordered_set<::uwb::UwbMacAddress>>) {
                    oss << ToString(arg);
                } else if constexpr (std::is_same_v<ParameterValueT, std::unordered_set<ResultReportConfiguration>>) {
                    oss << ToString(arg);
                } else if constexpr (std::is_same_v<ParameterValueT, std::array<uint8_t, StaticStsInitializationVectorLength>>) {
                    for (const auto& value : arg) {
                        oss << "0x" << std::setw(2) << std::internal << std::setfill('0') << std::hex << +value << " ";
                    }
                }

                std::cout << oss.str() << std::endl;
            },
                parameterValue);
        }

        std::cout << "StaticRangingInfo: { " << m_cliData->SessionData.staticRangingInfo << " }" << std::endl;
        RegisterCliAppWithOperation(rangeStartApp);
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
NearObjectCli::AddSubcommandServiceRangeStop(CLI::App* parent)
{
    // top-level command
    auto rangeStopApp = parent->add_subcommand("stop", "Stop a NearObject ranging session")->fallthrough();

    rangeStopApp->parse_complete_callback([this, rangeStopApp] {
        std::cout << "stop ranging" << std::endl;
        RegisterCliAppWithOperation(rangeStopApp);
    });

    rangeStopApp->final_callback([this, rangeStopApp] {
        m_cliHandler->HandleStopRanging();
        SignalCliAppOperationCompleted(rangeStopApp);
    });

    return rangeStopApp;
}
