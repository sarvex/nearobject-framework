
#ifndef UWB_CONFIGURATION_HXX
#define UWB_CONFIGURATION_HXX

#include <compare>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_set>

#include <TlvBer.hxx>
#include <notstd/hash.hxx>

#include <uwb/UwbMacAddress.hxx>
#include <uwb/protocols/fira/FiraDevice.hxx>
#include <uwb/protocols/fira/RangingConfiguration.hxx>

namespace uwb::protocol::fira
{
/**
 * @brief Describes UWB configuration parameters for a session.
 *
 * See FiRa Consortium Common Service Management Layer Technical Specification
 * v1.0.0, Section 6.4.3, 'UWB_CONFIGURATION', pages 50-54.
 */
struct UwbConfiguration
{
    /**
     * @brief See FiRa Consortium Common Service Management Layer Technical
     * Specification v1.0.0, Section 7.5.3.2, 'UWB Session Data structure',
     * Table 53, pages 103-107.
     */
    static constexpr uint8_t Tag = 0xA3;

    /**
     * @brief See FiRa Consortium Common Service Management Layer Technical
     * Specification v1.0.0, Section 7.5.3.2, 'UWB Session Data structure',
     * Table 53, pages 103-107.
     */
    enum class ParameterTag : uint8_t {
        FiraPhyVersion = 0x80,
        FiraMacVersion = 0x81,
        DeviceRole = 0x82,
        RangingMethod = 0x83,
        StsConfig = 0x84,
        MultiNodeMode = 0x85,
        RangingTimeStruct = 0x86,
        ScheduledMode = 0x87,
        HoppingMode = 0x88,
        BlockStriding = 0x89,
        UwbInitiationTime = 0x8A,
        ChannelNumber = 0x8B,
        RFrameConfig = 0x8C,
        CcConstraintLength = 0x8D,
        PrfMode = 0x8E,
        Sp0PhySetNumber = 0x8F,
        Sp1PhySetNumber = 0x90,
        Sp3PhySetNumber = 0x91,
        PreableCodeIndex = 0x92,
        ResultReportConfig = 0x93,
        MacAddressMode = 0x94,
        ControleeShortMacAddress = 0x95,
        ControllerMacAddress = 0x96,
        SlotsPerRr = 0x97,
        MaxContentionPhaseLength = 0x98,
        SlotDuration = 0x99,
        RangingInternval = 0x9A,
        KeyRotationRate = 0x9B,
        MacFcsType = 0x9C,
        MaxRrRetry = 0x9D,
    };

    /**
     * @brief Default values, if omitted, per FiRa.
     *
     * See FiRa Consortium Common Service Management Layer Technical
     * Specification v1.0.0, Section 7.5.3.2, 'UWB Session Data structure',
     * Table 53, pages 103-107.
     */
    static constexpr auto DeviceRoleDefault = DeviceRole::Responder;
    static constexpr auto RangingConfigurationDefault = RangingConfiguration{ RangingMethod::DoubleSidedTwoWay, MeasurementReportMode::Deferred };
    static constexpr auto StsConfigurationDefault = StsConfiguration::Static;
    static constexpr auto MultiNodeModeDefault = MultiNodeMode::Unicast;
    static constexpr auto RangingTimeStructDefault = RangingMode::Block;
    static constexpr auto ScheduledModeDefault = SchedulingMode::Time;
    static constexpr auto HoppingModeDefault = false;
    static constexpr auto BlockStridingDefault = false;
    static constexpr auto UwbInitiationTimeDefault = 0UL;
    static constexpr auto ChannelDefault = Channel::C9;
    static constexpr auto RFrameConfigDefault = StsPacketConfiguration::SP3;
    static constexpr auto CcConstraintLengthDefault = ConvolutionalCodeConstraintLength::K3;
    static constexpr auto PrfModeDefault = PrfMode::Bprf;
    static constexpr auto Sp0PhySetNumberDefault = 1;
    static constexpr auto Sp1PhySetNumberDefault = 1;
    static constexpr auto Sp3PhySetNumberDefault = 0;
    static constexpr auto PreableCodeIndexDefault = 0;
    static constexpr auto MacAddressModeDefault = UwbMacAddressType::Short;
    static constexpr auto KeyRotationRateDefault = 0UL;
    static constexpr auto MacFcsTypeDefault = UwbMacAddressFcsType::Crc16;
    static constexpr auto MaxRrRetryDefault = 0U;
    static const std::unordered_set<ResultReportConfiguration> ResultReportConfigurationsDefault;

    /**
     * @brief Construct UwbConfiguration object with default values and settings.
     */
    UwbConfiguration() = default;

    /**
     * @brief Default equality operator.
     *
     * @param other
     * @return true
     * @return false
     */
    bool
    operator==(const UwbConfiguration& other) const noexcept = default;

    /**
     * @brief Convert this object into a FiRa Data Object (DO).
     *
     * @return std::unique_ptr<encoding::TlvBer>
     */
    std::unique_ptr<encoding::TlvBer>
    ToDataObject() const;

    /**
     * @brief Attempt to create a UwbConfiguration object from a TlvBer.
     *
     * @param tlv
     * @return UwbConfiguration
     */
    static UwbConfiguration
    FromDataObject(const encoding::TlvBer& tlv);

    uint32_t FiraPhyVersion{ 0 };
    uint32_t FiraMacVersion{ 0 };
    DeviceRole DeviceRole{ DeviceRoleDefault };
    RangingConfiguration RangingConfiguration{ RangingConfigurationDefault };
    StsConfiguration StsConfiguration{ StsConfigurationDefault };
    MultiNodeMode MultiNodeMode{ MultiNodeModeDefault };
    RangingMode RangingTimeStruct{ RangingTimeStructDefault };
    SchedulingMode SchedulingMode{ ScheduledModeDefault };
    bool HoppingMode{ HoppingModeDefault };
    bool BlockStriding{ BlockStridingDefault };
    uint32_t UwbInitiationTime{ UwbInitiationTimeDefault };
    Channel Channel{ Channel::C9 };
    StsPacketConfiguration RFrameConfig{ RFrameConfigDefault };
    ConvolutionalCodeConstraintLength ConvolutionalCodeConstraintLength{ CcConstraintLengthDefault };
    PrfMode PrfMode{ PrfModeDefault };
    uint8_t Sp0PhySetNumber{ Sp0PhySetNumberDefault };
    uint8_t Sp1PhySetNumber{ Sp0PhySetNumberDefault };
    uint8_t Sp3PhySetNumber{ Sp0PhySetNumberDefault };
    uint8_t PreableCodeIndex{ PreableCodeIndexDefault };
    std::unordered_set<ResultReportConfiguration> ResultReportConfigurations{ ResultReportConfigurationsDefault };
    UwbMacAddressType MacAddressMode{ MacAddressModeDefault };
    std::optional<UwbMacAddress> ControleeShortMacAddress;
    UwbMacAddress ControllerMacAddress;
    uint8_t SlotsPerRangingRound{ 0 };
    uint8_t MaxContentionPhaseLength{ 0 };
    uint8_t SlotDuration{ 0 };
    uint16_t RangingInterval{ 0 };
    uint8_t KeyRotationRate{ KeyRotationRateDefault };
    UwbMacAddressFcsType MacAddressFcsType{ MacFcsTypeDefault };
    uint16_t MaxRangingRoundRetry{ MaxRrRetryDefault };
};

} // namespace uwb::protocol::fira

namespace std
{
template <>
struct hash<uwb::protocol::fira::UwbConfiguration>
{
    std::size_t
    operator()(const uwb::protocol::fira::UwbConfiguration& uwbConfiguration) const noexcept
    {
        std::size_t value = 0;
        notstd::hash_combine(value,
            uwbConfiguration.FiraPhyVersion,
            uwbConfiguration.FiraMacVersion,
            uwbConfiguration.DeviceRole,
            uwbConfiguration.RangingConfiguration,
            uwbConfiguration.StsConfiguration,
            uwbConfiguration.MultiNodeMode,
            uwbConfiguration.RangingTimeStruct,
            uwbConfiguration.SchedulingMode,
            uwbConfiguration.HoppingMode,
            uwbConfiguration.BlockStriding,
            uwbConfiguration.UwbInitiationTime,
            uwbConfiguration.Channel,
            uwbConfiguration.RFrameConfig,
            uwbConfiguration.ConvolutionalCodeConstraintLength,
            uwbConfiguration.PrfMode,
            uwbConfiguration.Sp0PhySetNumber,
            uwbConfiguration.Sp1PhySetNumber,
            uwbConfiguration.Sp3PhySetNumber,
            uwbConfiguration.PreableCodeIndex,
            notstd::hash_range(std::cbegin(uwbConfiguration.ResultReportConfigurations), std::cend(uwbConfiguration.ResultReportConfigurations)),
            uwbConfiguration.MacAddressMode,
            uwbConfiguration.ControleeShortMacAddress,
            uwbConfiguration.ControllerMacAddress,
            uwbConfiguration.SlotsPerRangingRound,
            uwbConfiguration.MaxContentionPhaseLength,
            uwbConfiguration.SlotDuration,
            uwbConfiguration.RangingInterval,
            uwbConfiguration.KeyRotationRate,
            uwbConfiguration.MacAddressFcsType,
            uwbConfiguration.MaxRangingRoundRetry);
        return value;
    }
};
} // namespace std

#endif // UWB_CONFIGURATION_HXX