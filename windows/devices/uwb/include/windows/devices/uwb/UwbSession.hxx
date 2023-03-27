
#ifndef WINDOWS_UWB_SESSION_HXX
#define WINDOWS_UWB_SESSION_HXX

#include <cstdint>
#include <memory>

#include <uwb/UwbMacAddress.hxx>
#include <uwb/UwbRegisteredCallbacks.hxx>
#include <uwb/UwbSession.hxx>
#include <uwb/UwbSessionEventCallbacks.hxx>
#include <uwb/protocols/fira/UwbConfiguration.hxx>
#include <windows/devices/uwb/UwbDeviceConnector.hxx>
#include <windows/devices/uwb/UwbDevice.hxx>

namespace windows::devices::uwb
{
/**
 * @brief Windows concrete implementation of a UWB session.
 */
class UwbSession :
    public ::uwb::UwbSession
{
public:
    /**
     * @brief Construct a new UwbSession object.
     * This also registers the callbacks with the UwbDeviceConnector
     *
     * @param sessionId
     * @param callbacks The event callback instance.
     * @param uwbDevice The uwb device tied to the session.
     */
    UwbSession(uint32_t sessionId, std::weak_ptr<::uwb::UwbSessionEventCallbacks> callbacks, UwbDevice* uwbDevice, ::uwb::protocol::fira::DeviceType deviceType = ::uwb::protocol::fira::DeviceType::Controller);

private:
    /**
     * @brief Configure the session for use.
     *
     * @param uwbSessionData The session data to configure the session with.
     */
    virtual void
    ConfigureImpl(const std::vector<::uwb::protocol::fira::UwbApplicationConfigurationParameter> configParams) override;

    /**
     * @brief Start ranging.
     */
    virtual void
    StartRangingImpl() override;

    /**
     * @brief Stop ranging.
     */
    virtual void
    StopRangingImpl() override;

    /**
     * @brief Add a new peer to the session.
     */
    virtual void
    AddPeerImpl(::uwb::UwbMacAddress peerMacAddress) override;

    /**
     * @brief Get the application configuration parameters for this session.
     * 
     * @return std::vector<::uwb::protocol::fira::UwbApplicationConfigurationParameter> 
     */
    virtual std::vector<::uwb::protocol::fira::UwbApplicationConfigurationParameter>
    GetApplicationConfigurationParametersImpl() override;

protected:
    /**
     * @brief Obtain a shared instance of the device driver connector.
     *
     * @return std::shared_ptr<UwbDeviceConnector>
     */
    std::shared_ptr<UwbDeviceConnector>
    GetUwbDeviceConnector() noexcept;

private:
    std::shared_ptr<UwbDeviceConnector> m_uwbDeviceConnector;
    std::shared_ptr<::uwb::UwbRegisteredSessionEventCallbacks> m_registeredCallbacks;
    RegisteredCallbackToken* m_registeredCallbacksToken;
};

} // namespace windows::devices::uwb

#endif // WINDOWS_UWB_SESSION_HXX
