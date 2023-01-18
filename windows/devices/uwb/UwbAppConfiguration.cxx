
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <tuple>

#include <windows/devices/uwb/UwbAppConfiguration.hxx>

bool
operator==(const UWB_APP_CONFIG_PARAM& lhs, const UWB_APP_CONFIG_PARAM& rhs) noexcept
{
    return (lhs.size == rhs.size) && (std::memcmp(&lhs, &rhs, lhs.size) == 0);
}

using namespace windows::devices;

IUwbAppConfigurationParameter::IUwbAppConfigurationParameter(UWB_APP_CONFIG_PARAM_TYPE parameterType, std::span<const uint8_t> parameterValue) :
    m_buffer(offsetof(UWB_APP_CONFIG_PARAM, paramValue[parameterValue.size()])),
    m_parameter(*reinterpret_cast<UWB_APP_CONFIG_PARAM*>(m_buffer.data()))
{
    m_parameter.size = std::size(m_buffer);
    m_parameter.paramType = parameterType;
    m_parameter.paramLength = parameterValue.size();
    std::memcpy(&m_parameter.paramValue, parameterValue.data(), parameterValue.size());
}

UWB_APP_CONFIG_PARAM&
IUwbAppConfigurationParameter::DdiParameter() noexcept
{
    return m_parameter;
}

std::vector<uint8_t>&
IUwbAppConfigurationParameter::DdiBuffer() noexcept
{
    return m_buffer;
}

namespace
{
std::size_t
CalculateTotalUwbAppConfigurationBufferSize(const std::vector<std::shared_ptr<IUwbAppConfigurationParameter>>& parameters)
{
    // TODO:
    return 0;
}
} // namespace

UwbSetAppConfigurationParameters::UwbSetAppConfigurationParameters(const std::vector<std::shared_ptr<IUwbAppConfigurationParameter>>& parameters) :
    m_buffer(offsetof(UWB_APP_CONFIG_PARAMS, appConfigParamsCount) + CalculateTotalUwbAppConfigurationBufferSize(parameters)),
    m_parameters(*reinterpret_cast<UWB_APP_CONFIG_PARAMS*>(std::data(m_buffer)))
{
}

UWB_APP_CONFIG_PARAMS& 
UwbSetAppConfigurationParameters::DdiParameters() noexcept
{
    return m_parameters;
}

std::vector<uint8_t>&
UwbSetAppConfigurationParameters::DdiBuffer() noexcept
{
    return m_buffer;
}
