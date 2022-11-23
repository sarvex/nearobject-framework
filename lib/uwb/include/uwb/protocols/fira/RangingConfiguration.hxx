
#ifndef FIRA_RANGING_CONFIGURATION_HXX
#define FIRA_RANGING_CONFIGURATION_HXX

#include <functional>

#include <notstd/hash.hxx>
#include <notstd/utility.hxx>

#include <uwb/protocols/fira/FiraDevice.hxx>

namespace uwb::protocol::fira
{
/**
 * @brief Effective ranging configuration.
 */
struct RangingConfiguration
{
    constexpr RangingConfiguration() = default;

    constexpr RangingConfiguration(RangingMethod method, MeasurementReportMode reportMode) :
        Method(method),
        ReportMode(reportMode)
    {}

    RangingMethod Method{ RangingMethod::OneWay };
    MeasurementReportMode ReportMode{ MeasurementReportMode::None };

    auto operator<=>(const RangingConfiguration& other) const = default;
};

} // namespace uwb::protocol::fira

namespace std
{
template <>
struct hash<uwb::protocol::fira::RangingConfiguration>
{
    std::size_t
    operator()(const uwb::protocol::fira::RangingConfiguration& rangingConfiguration) const noexcept
    {
        std::size_t hash = 0;
        notstd::hash_combine(hash, notstd::to_underlying(rangingConfiguration.Method), notstd::to_underlying(rangingConfiguration.ReportMode));
        return hash;
    }
};
} // namespace std

#endif // FIRA_RANGING_CONFIGURATION_HXX