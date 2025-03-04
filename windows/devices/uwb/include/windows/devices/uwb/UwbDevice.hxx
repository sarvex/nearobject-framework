
#ifndef WINDOWS_DEVICE_UWB_HXX
#define WINDOWS_DEVICE_UWB_HXX

#include <concepts>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>

// NB: This must come before any other Windows include
#include <windows.h>

#include <cfgmgr32.h>
#include <wil/resource.h>

#include <uwb/UwbDevice.hxx>
#include <uwb/UwbRegisteredCallbacks.hxx>
#include <uwb/UwbSession.hxx>
#include <uwb/protocols/fira/FiraDevice.hxx>
#include <windows/devices/DeviceResource.hxx>
#include <windows/devices/uwb/IUwbDeviceDdi.hxx>
#include <windows/devices/uwb/IUwbSessionDdi.hxx>

namespace uwb
{
class UwbSession;
}

namespace windows::devices::uwb
{
/**
 * @brief Helper class to interact with Windows UWB devices using the Windows
 * UWB DDI.
 */
class UwbDevice :
    public ::uwb::UwbDevice,
    public std::enable_shared_from_this<UwbDevice>
{
protected:
    /**
     * @brief Construct a new Uwb Device object.
     *
     * @param deviceName The interface path name.
     */
    explicit UwbDevice(std::string deviceName);

public:
    /**
     * @brief Create a new UwbDevice object instance.
     *
     * @param deviceName The interface path name.
     * @return std::shared_ptr<UwbDevice>
     */
    static std::shared_ptr<UwbDevice>
    Create(std::string deviceName);

    /**
     * @brief Get the name of this device.
     *
     * @return const std::string&
     */
    const std::string&
    DeviceName() const noexcept;

    /**
     * @brief Determine if this device is the same as another.
     *
     * @param other
     * @return true
     * @return false
     */
    bool
    IsEqual(const ::uwb::UwbDevice& other) const noexcept override;

private:
    /**
     * @brief Create a new UWB session.
     *
     * @param sessionId
     * @param callbacks The event callback instance.
     * @return std::shared_ptr<uwb::UwbSession>
     */
    virtual std::shared_ptr<::uwb::UwbSession>
    CreateSessionImpl(uint32_t sessionId, std::weak_ptr<::uwb::UwbSessionEventCallbacks> callbacks) override;

    /**
     * @brief Attempt to resolve a session from the underlying UWB device.
     *
     * This is used when a session may be pre-existing within the driver but
     * this instance is not yet aware of.
     *
     * @param sessionId The identifier of the session to resolve.
     * @return std::shared_ptr<UwbSession>
     */
    virtual std::shared_ptr<::uwb::UwbSession>
    ResolveSessionImpl(uint32_t sessionId) override;

    /**
     * @brief Get the capabilities of the device.
     *
     * @return uwb::protocol::fira::UwbCapability
     */
    virtual ::uwb::protocol::fira::UwbCapability
    GetCapabilitiesImpl() override;

    /**
     * @brief Get the FiRa device information of the device.
     *
     * @return ::uwb::protocol::fira::UwbDeviceInformation
     */
    virtual ::uwb::protocol::fira::UwbDeviceInformation
    GetDeviceInformationImpl() override;

    /**
     * @brief Get the number of sessions associated with the device.
     *
     * @return uint32_t
     */
    virtual uint32_t
    GetSessionCountImpl() override;

    /**
     * @brief Reset the device to an initial clean state.
     */
    virtual void
    ResetImpl() override;

    /**
     * @brief Initialize the device.
     *
     * @return bool
     */
    virtual bool
    InitializeImpl() override;

protected:
    /**
     * @brief Get the device DDI connector object (derived classes only).
     *
     * @return std::shared_ptr<IUwbDeviceDdiConnector>
     */
    std::shared_ptr<IUwbDeviceDdiConnector>
    GetDeviceDdiConnector() noexcept;

    /**
     * @brief Get the session DDI connector object (derived classes only).
     *
     * @return std::shared_ptr<IUwbSessionDdiConnector>
     */
    std::shared_ptr<IUwbSessionDdiConnector>
    GetSessionDdiConnector() noexcept;

private:
    const std::string m_deviceName;
    std::shared_ptr<IUwbDeviceDdiConnector> m_uwbDeviceConnector;
    std::shared_ptr<IUwbSessionDdiConnector> m_uwbSessionConnector;
    std::shared_ptr<::uwb::UwbRegisteredDeviceEventCallbackTypes::OnStatusChanged> m_onStatusChangedCallback;
    std::shared_ptr<::uwb::UwbRegisteredDeviceEventCallbackTypes::OnDeviceStatusChanged> m_onDeviceStatusChangedCallback;
    std::shared_ptr<::uwb::UwbRegisteredDeviceEventCallbackTypes::OnSessionStatusChanged> m_onSessionStatusChangedCallback;
    ::uwb::UwbRegisteredDeviceEventCallbackTokens m_callbacksToken;
};
} // namespace windows::devices::uwb

#endif // WINDOWS_DEVICE_UWB_HXX
