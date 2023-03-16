
#include <algorithm>
#include <iterator>
#include <ranges>

#include <magic_enum.hpp>

#include <uwb/protocols/fira/FiraDevice.hxx>
#include <windows/devices/uwb/UwbCxAdapterDdiLrp.hxx>

#include "IUwbSimulator.hxx"
#include "UwbSimulatorDdiCallbacks.hxx"
#include "UwbSimulatorTracelogging.hxx"

using namespace windows::devices::uwb;
using namespace windows::devices::uwb::simulator;

/**
 * @brief Namespace alias to reduce typing but preserve clarity regarding DDI
 * conversion.
 */
namespace UwbCxDdi = windows::devices::uwb::ddi::lrp;

UwbSimulatorDdiCallbacks::UwbSimulatorDdiCallbacks(UwbSimulatorDeviceFile *deviceFile) :
    m_simulatorCapabilities({ IUwbSimulator::Version }),
    m_deviceFile(deviceFile)
{}

NTSTATUS
UwbSimulatorDdiCallbacks::RaiseUwbNotification(UwbNotificationData uwbNotificationData)
{
    // Acquire the notification lock to ensure the notification proimise can be safely inspected and updated.
    std::unique_lock notificationLock{ m_notificationGate };

    TraceLoggingWrite(
        UwbSimulatorTraceloggingProvider,
        "UwbNotification",
        TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
        TraceLoggingString("EventRaised", "Action"),
        TraceLoggingBool(m_notificationPromise.has_value(), "WaitPending"),
        TraceLoggingString(std::data(ToString(uwbNotificationData)), "Data"));

    if (!m_notificationPromise.has_value()) {
        // No outstanding client waiting for a result, so nothing to do.
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    m_notificationPromise->set_value(std::move(uwbNotificationData));
    m_notificationPromise.reset();

    return STATUS_SUCCESS;
}

void
UwbSimulatorDdiCallbacks::SessionUpdateState(UwbSimulatorSession &session, UwbSessionState sessionState, std::optional<UwbSessionReasonCode> reasonCode = std::nullopt)
{
    TraceLoggingWrite(
        UwbSimulatorTraceloggingProvider,
        "SessionStateUpdate",
        TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
        TraceLoggingString(std::data(magic_enum::enum_name(session.State)), "StatePrevious"),
        TraceLoggingString(std::data(magic_enum::enum_name(sessionState)), "StateNew"));

    session.State = sessionState;

    // Create and raise a notification with the state change.
    UwbNotificationData notificationData{
        UwbSessionStatus{
            .SessionId = session.Id,
            .State = sessionState,
            .ReasonCode = reasonCode }
    };

    RaiseUwbNotification(std::move(notificationData));
}

UwbStatus
UwbSimulatorDdiCallbacks::DeviceReset()
{
    TraceLoggingWrite(
        UwbSimulatorTraceloggingProvider,
        "DeviceReset",
        TraceLoggingLevel(TRACE_LEVEL_INFORMATION));

    return UwbStatusOk;
}

UwbStatus
UwbSimulatorDdiCallbacks::DeviceGetInformation(UwbDeviceInformation &deviceInformation)
{
    deviceInformation = m_deviceInformation;

    TraceLoggingWrite(
        UwbSimulatorTraceloggingProvider,
        "DeviceGetInformation",
        TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
        TraceLoggingString(deviceInformation.ToString().c_str(), "DeviceInformation"));

    return UwbStatusOk;
}

UwbStatus
UwbSimulatorDdiCallbacks::DeviceGetCapabilities(UwbCapability &deviceCapabilities)
{
    deviceCapabilities = m_deviceCapabilities;

    TraceLoggingWrite(
        UwbSimulatorTraceloggingProvider,
        "DeviceGetCapabilities",
        TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
        TraceLoggingString(deviceCapabilities.ToString().c_str(), "DeviceCapabilities"));

    return UwbStatusOk;
}

UwbStatus
UwbSimulatorDdiCallbacks::DeviceGetConfigurationParameters(std::vector<UwbDeviceConfigurationParameterType> & /* deviceConfigurationParameterTypes */, std::vector<std::tuple<UwbDeviceConfigurationParameterType, UwbStatus, std::optional<UwbDeviceConfigurationParameter>>> &deviceConfigurationParameterResults)
{
    deviceConfigurationParameterResults = {};
    return UwbStatusOk;
}

UwbStatus
UwbSimulatorDdiCallbacks::DeviceSetConfigurationParameters(const std::vector<UwbDeviceConfigurationParameter> & /* deviceConfigurationParameters */, std::vector<std::tuple<UwbDeviceConfigurationParameterType, UwbStatus>> &deviceConfigurationParameterResults)
{
    deviceConfigurationParameterResults = {};
    return UwbStatusOk;
}

UwbStatus
UwbSimulatorDdiCallbacks::SessionInitialize(uint32_t sessionId, UwbSessionType sessionType)
{
    TraceLoggingWrite(
        UwbSimulatorTraceloggingProvider,
        "SessionInitialize",
        TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
        TraceLoggingUInt32(sessionId, "Session Id"),
        TraceLoggingString(std::data(magic_enum::enum_name(sessionType)), "Session Type"));

    std::unique_lock sessionsWriteLock{ m_sessionsGate };
    auto [sessionIt, inserted] = m_sessions.try_emplace(sessionId, sessionId, sessionType);
    if (!inserted) {
        return UwbStatusSession::Duplicate;
    }

    auto &[_, session] = *sessionIt;
    SessionUpdateState(session, UwbSessionState::Initialized);

    return UwbStatusOk;
}

UwbStatus
UwbSimulatorDdiCallbacks::SessionDeninitialize(uint32_t sessionId)
{
    TraceLoggingWrite(
        UwbSimulatorTraceloggingProvider,
        "SessionDeninitialize",
        TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
        TraceLoggingUInt32(sessionId, "Session Id"));

    decltype(m_sessions)::node_type nodeHandle;
    {
        std::unique_lock sessionsWriteLock{ m_sessionsGate };
        nodeHandle = m_sessions.extract(sessionId);
    }
    if (nodeHandle.empty()) {
        return UwbStatusSession::NotExist;
    }

    auto &session = nodeHandle.mapped();
    SessionUpdateState(session, UwbSessionState::Deinitialized);

    return UwbStatusOk;
}

UwbStatus
UwbSimulatorDdiCallbacks::SetApplicationConfigurationParameters(uint32_t sessionId, const std::vector<std::shared_ptr<IUwbAppConfigurationParameter>> & /* applicationConfigurationParameters */, std::vector<std::tuple<UwbApplicationConfigurationParameterType, UwbStatus>> &applicationConfigurationParameterResults)
{
    TraceLoggingWrite(
        UwbSimulatorTraceloggingProvider,
        "SetApplicationConfigurationParameters",
        TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
        TraceLoggingUInt32(sessionId, "Session Id"));

    std::vector<std::tuple<UwbApplicationConfigurationParameterType, UwbStatus>> results{};
    applicationConfigurationParameterResults = std::move(results);
    return UwbStatusOk;
}

UwbStatus
UwbSimulatorDdiCallbacks::GetApplicationConfigurationParameters(uint32_t sessionId, [[maybe_unused]] const std::vector<UwbApplicationConfigurationParameterType> &applicationConfigurationParameterTypes, std::vector<std::shared_ptr<IUwbAppConfigurationParameter>> &applicationConfigurationParameters)
{
    TraceLoggingWrite(
        UwbSimulatorTraceloggingProvider,
        "GetApplicationConfigurationParameters",
        TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
        TraceLoggingUInt32(sessionId, "Session Id"));

    std::shared_lock sessionsReadLock{ m_sessionsGate };
    auto sessionIt = m_sessions.find(sessionId);
    if (sessionIt == std::cend(m_sessions)) {
        applicationConfigurationParameters = {};
        return UwbStatusSession::NotExist;
    }

    const auto &[_, session] = *sessionIt;
    applicationConfigurationParameters = session.ApplicationConfigurationParameters;
    // TODO: filter above with applicationConfigurationParameterTypes
    return UwbStatusOk;
}

UwbStatus
UwbSimulatorDdiCallbacks::GetSessionCount(uint32_t &sessionCount)
{
    std::shared_lock sessionsReadLock{ m_sessionsGate };
    sessionCount = static_cast<uint32_t>(std::size(m_sessions));

    TraceLoggingWrite(
        UwbSimulatorTraceloggingProvider,
        "GetSessionCount",
        TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
        TraceLoggingUInt32(sessionCount, "Session Count"));

    return UwbStatusOk;
}

UwbStatus
UwbSimulatorDdiCallbacks::SessionGetState(uint32_t sessionId, UwbSessionState &sessionState)
{
    std::shared_lock sessionsReadLock{ m_sessionsGate };
    auto sessionIt = m_sessions.find(sessionId);
    if (sessionIt == std::cend(m_sessions)) {
        return UwbStatusSession::NotExist;
    }

    const auto &[_, session] = *sessionIt;
    sessionState = session.State;

    TraceLoggingWrite(
        UwbSimulatorTraceloggingProvider,
        "SessionGetState",
        TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
        TraceLoggingUInt32(sessionId, "Session Id"),
        TraceLoggingString(std::data(magic_enum::enum_name(sessionState)), "Session State"));

    return UwbStatusOk;
}

UwbStatus
UwbSimulatorDdiCallbacks::SessionUpdateControllerMulticastList(uint32_t sessionId, UwbMulticastAction action, std::vector<UwbSessionUpdateMulticastListEntry> updateMulticastListEntries)
{
    TraceLoggingWrite(
        UwbSimulatorTraceloggingProvider,
        "SessionUpdateControllerMulticastList",
        TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
        TraceLoggingUInt32(sessionId, "Session Id"),
        TraceLoggingString(std::data(magic_enum::enum_name(action)), "Multicast Action"));

    std::unique_lock sessionsWriteLock{ m_sessionsGate };
    auto sessionIt = m_sessions.find(sessionId);
    if (sessionIt == std::cend(m_sessions)) {
        return UwbStatusSession::NotExist;
    }

    auto &[_, session] = *sessionIt;

    const auto getControlee = [](const UwbSessionUpdateMulticastListEntry &entry) {
        return entry.ControleeMacAddress;
    };

    switch (action) {
    case UwbMulticastAction::AddShortAddress: {
        // TODO: updateMulticastListEntry.SubSessionId needs to be handled in future.
        std::ranges::move(updateMulticastListEntries | std::views::transform(getControlee), std::inserter(session.Controlees, std::end(session.Controlees)));
        break;
    }
    case UwbMulticastAction::DeleteShortAddress: {
        // TODO: updateMulticastListEntry.SubSessionId needs to be handled in future.
        std::erase_if(session.Controlees, [&](const auto &controleeToRemove) {
            return std::ranges::any_of(updateMulticastListEntries | std::views::transform(getControlee), [&](const auto &controlee) {
                return controleeToRemove == controlee;
            });
        });
        break;
    }
    default:
        return UwbStatusGeneric::InvalidParameter;
    }

    return UwbStatusOk;
}

UwbStatus
UwbSimulatorDdiCallbacks::SessionStartRanging(uint32_t sessionId)
{
    TraceLoggingWrite(
        UwbSimulatorTraceloggingProvider,
        "SessionStartRanging",
        TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
        TraceLoggingUInt32(sessionId, "Session Id"));

    std::unique_lock sessionsWriteLock{ m_sessionsGate };
    auto sessionIt = m_sessions.find(sessionId);
    if (sessionIt == std::cend(m_sessions)) {
        return UwbStatusSession::NotExist;
    }

    auto &[_, session] = *sessionIt;
    session.RangingCount++;
    return UwbStatusOk;
}

UwbStatus
UwbSimulatorDdiCallbacks::SessionStopRanging(uint32_t sessionId)
{
    TraceLoggingWrite(
        UwbSimulatorTraceloggingProvider,
        "SessionStopRanging",
        TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
        TraceLoggingUInt32(sessionId, "Session Id"));

    std::unique_lock sessionsWriteLock{ m_sessionsGate };
    auto sessionIt = m_sessions.find(sessionId);
    if (sessionIt == std::cend(m_sessions)) {
        return UwbStatusSession::NotExist;
    }

    auto &[_, session] = *sessionIt;

    return UwbStatusOk;
}

UwbStatus
UwbSimulatorDdiCallbacks::SessionGetRangingCount(uint32_t sessionId, uint32_t &rangingCount)
{
    std::unique_lock sessionsWriteLock{ m_sessionsGate };
    auto sessionIt = m_sessions.find(sessionId);
    if (sessionIt == std::cend(m_sessions)) {
        return UwbStatusSession::NotExist;
    }

    auto &[_, session] = *sessionIt;
    rangingCount = session.RangingCount;

    TraceLoggingWrite(
        UwbSimulatorTraceloggingProvider,
        "SessionGetRangingCount",
        TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
        TraceLoggingUInt32(sessionId, "Session Id"),
        TraceLoggingUInt32(rangingCount, "Session Ranging Count"));

    return UwbStatusOk;
}

NTSTATUS
UwbSimulatorDdiCallbacks::UwbNotification(UwbNotificationData &notificationData, std::size_t &notificationDataSize)
{  
    std::size_t outputBufferSizeRequired = 0;
    std::optional<UwbNotificationData> notificationDataOpt;

    auto *ioEventQueue = m_deviceFile->GetIoEventQueue();
    NTSTATUS status = ioEventQueue->GetNextQueuedRequest(notificationDataOpt, outputBufferSizeRequired);
    if (!NT_SUCCESS(status)) {
        if (status == STATUS_NO_MORE_ENTRIES) {
            // Pend request
            WDFREQUEST request; // = TODO
            // alternatively, we could expect the handler to pend the request instead
            ioEventQueue->PendRequest(request, notificationDataSize);
            return STATUS_PENDING;
        }
    } else {
        notificationData = std::move(notificationDataOpt.value());
    }

    //// Acquire the notification lock to ensure the notification proimise can be safely inspected and updated.
    //std::unique_lock notificationLock{ m_notificationGate };
    //if (m_notificationPromise.has_value()) {
    //    // pre-existing promise, this should not happen.
    //    TraceLoggingWrite(
    //        UwbSimulatorTraceloggingProvider,
    //        "UwbNotification",
    //        TraceLoggingLevel(TRACE_LEVEL_WARNING),
    //        TraceLoggingString("Ignore", "Action"));
    //    return STATUS_ALREADY_REGISTERED;
    //}

    //// Create a new promise whose shared state will be updated when a notification is raised.
    //auto notificationFuture = m_notificationPromise.emplace().get_future();

    //// Release the lock since the shared data (promise) has been created and future obtained.
    //notificationLock.unlock();

    //TraceLoggingWrite(
    //    UwbSimulatorTraceloggingProvider,
    //    "UwbNotification",
    //    TraceLoggingLevel(TRACE_LEVEL_VERBOSE),
    //    TraceLoggingString("Wait", "Action"));

    //// Lock is now released; synchronously wait indefinitely for the shared state to be updated.
    //notificationFuture.wait();
    //notificationData = notificationFuture.get();

    //TraceLoggingWrite(
    //    UwbSimulatorTraceloggingProvider,
    //    "UwbNotification",
    //    TraceLoggingLevel(TRACE_LEVEL_VERBOSE),
    //    TraceLoggingString("WaitComplete", "Action"));

    return STATUS_SUCCESS;
}

UwbSimulatorCapabilities
UwbSimulatorDdiCallbacks::GetSimulatorCapabilities()
{
    TraceLoggingWrite(
        UwbSimulatorTraceloggingProvider,
        "GetSimulatorCapabilities",
        TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
        TraceLoggingHexUInt32(m_simulatorCapabilities.Version, "Version"));
    return m_simulatorCapabilities;
}

void
UwbSimulatorDdiCallbacks::TriggerSessionEvent(const UwbSimulatorTriggerSessionEventArgs &triggerSessionEventArgs)
{
    TraceLoggingWrite(
        UwbSimulatorTraceloggingProvider,
        "TriggerSessionEvent",
        TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
        TraceLoggingUInt32(triggerSessionEventArgs.SessionId, "Session Id"),
        TraceLoggingString(std::data(magic_enum::enum_name(triggerSessionEventArgs.Action)), "Action"));

    switch (triggerSessionEventArgs.Action) {
    case UwbSimulatorSessionEventAction::RandomRangingMeasurementGenerationStart: {
        SessionRandomMeasurementGenerationConfigure(triggerSessionEventArgs.SessionId, RandomMeasurementGeneration::Enable);
        break;
    }
    case UwbSimulatorSessionEventAction::RandomRangingMeasurementGenerationStop: {
        SessionRandomMeasurementGenerationConfigure(triggerSessionEventArgs.SessionId, RandomMeasurementGeneration::Disable);
        break;
    }
    case UwbSimulatorSessionEventAction::None: {
        break;
    }
    default: {
        break;
    }
    }
}

void
UwbSimulatorDdiCallbacks::SessionRandomMeasurementGenerationConfigure(uint32_t sessionId, RandomMeasurementGeneration action)
{
    std::unique_lock sessionsWriteLock{ m_sessionsGate };
    auto sessionIt = m_sessions.find(sessionId);
    if (sessionIt == std::cend(m_sessions)) {
        return;
    }

    auto &[_, session] = *sessionIt;

    switch (action) {
    case RandomMeasurementGeneration::Disable:
        session.RandomRangingMeasurementGenerationStop();
        break;
    case RandomMeasurementGeneration::Enable:
        session.RandomRangingMeasurementGenerationStart([&](UwbRangingData rangingData) {
            RaiseUwbNotification(std::move(rangingData));
        });
        break;
    }
}
