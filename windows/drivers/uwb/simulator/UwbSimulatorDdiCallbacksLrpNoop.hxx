
#ifndef UWB_SIMULATOR_DDI_CALLBACKS_LRP_NOOP
#define UWB_SIMULATOR_DDI_CALLBACKS_LRP_NOOP

#include <cstdint>
#include <vector>

#include "UwbSimulatorDdiCallbacksLrp.hxx"

namespace windows::devices::uwb::simulator
{
struct UwbSimulatorDdiCallbacksLrpNoop :
    public UwbSimulatorDdiCallbacksLrp
{
    virtual UwbStatus
    DeviceReset() override;

    virtual UwbStatus
    DeviceGetInfo(UwbDeviceInfo &deviceInfo) override;

    virtual UwbStatus
    DeviceGetCapabilities(UwbDeviceCapabilities &deviceCapabilities) override;

    virtual UwbStatus
    DeviceGetConfigurationParameters(std::vector<UwbDeviceConfigurationParameterType> &deviceConfigurationParameterTypes, std::vector<std::tuple<UwbDeviceConfigurationParameterType, UwbStatus, std::optional<UwbDeviceConfigurationParameter>>> &deviceConfigurationParameterResults) override;

    virtual UwbStatus
    DeviceSetConfigurationParameters(const std::vector<UwbDeviceConfigurationParameter> &deviceConfigurationParameters, std::vector<std::tuple<UwbDeviceConfigurationParameterType, UwbStatus>> &deviceConfigurationParameterResults) override;

    virtual UwbStatus
    SessionInitialize(uint32_t sessionId, UwbSessionType sessionType) override;

    virtual UwbStatus
    SessionDeninitialize(uint32_t sessionId) override;

    virtual UwbStatus
    SetApplicationConfigurationParameters(const std::vector<std::unique_ptr<IUwbAppConfigurationParameter>> &applicationConfigurationParameters, std::vector<std::tuple<UwbApplicationConfigurationParameterType, UwbStatus, std::unique_ptr<IUwbAppConfigurationParameter>>> &applicationConfigurationParameterResults) override;

    virtual UwbStatus
    GetApplicationConfigurationParameters(std::vector<UwbApplicationConfigurationParameter> &applicationConfigurationParameters) override;

    virtual UwbStatus
    GetSessionCount(uint32_t *sessionCount) override;

    virtual UwbStatus
    SessionGetState(uint32_t sessionId, UwbSessionState *sessionState) override;

    virtual UwbStatus
    SessionUpdateControllerMulticastList(const std::vector<UwbMacAddress> &controlees) override;

    virtual UwbStatus
    SessionStartRanging(uint32_t sessionId) override;

    virtual UwbStatus
    SessionStopRanging(uint32_t sessionId) override;

    virtual UwbStatus
    SessionGetRangingCount(uint32_t sessionId, uint32_t *rangingCount) override;
};
} // namespace windows::devices::uwb::simulator

#endif // UWB_SIMULATOR_DDI_CALLBACKS_LRP_NOOP