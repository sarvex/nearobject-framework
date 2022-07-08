
#ifndef __WINDOWS_DEVICE_SMARTCARD_HXX__
#define __WINDOWS_DEVICE_SMARTCARD_HXX__

#include <chrono>
#include <mutex>
#include <optional>
#include <future>
#include <vector>

#include <smartcard/ISmartCard.hxx>

namespace windows
{
namespace devices
{

/**
 * @brief Helper class to interact with Windows SmartCards using the Windows SmartCard DDI:
 * https://msdn.microsoft.com/en-us/library/windows/hardware/dn905601(v=vs.85).aspx
 */
class Smartcard :
    public smartcard::ISmartcard
{
public:
    /**
     * @brief Synchronously transmits a command apdu and provides the response.
     * This must handle any fragmentation of both transmitting the command and
     * receiving the response.
     *
     * @param command The command apdu to send.
     * @param response The response apdu to populate
     * @param timeout The timeout period after which the tranmission should be canceled.
     */
    void
    Transmit(const smartcard::ApduCommand& command, smartcard::ApduResponse& response, std::chrono::milliseconds timeout) override;

    /**
     * @brief Asynchronously transmits a command apdu and provides a future for
     * the response.
     * 
     * @param command The command apdu to send.
     * @param timeout The timeout period after which the tranmission should be canceled.
     * @return smartcard::ISmartcard::TransmitAsyncRequest 
     */
    smartcard::ISmartcard::TransmitAsyncRequest
    TransmitAsync(const smartcard::ApduCommand& command, std::chrono::milliseconds timeout) override;

private:
    /**
     * @brief Construct a new Complete Pending Transmission object
     * 
     * @param response 
     */
    void CompletePendingTransmission(smartcard::ApduResponse response);

private:
    // Mutex protecting resources for the pending transmission (tx), if any.
    // This includes reading from and writing to m_pendingTxResponsePromise.
    std::mutex m_pendingTxGate;
    std::optional<std::promise<smartcard::ApduResponse>> m_pendingTxResponsePromise;
};

} // namespace devices
} // namespace windows

#endif // __WINDOWS_DEVICE_SMARTCARD_HXX__