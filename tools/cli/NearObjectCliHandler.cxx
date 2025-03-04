
#include <nearobject/cli/NearObjectCli.hxx>
#include <nearobject/cli/NearObjectCliHandler.hxx>
#include <uwb/UwbDevice.hxx>
#include <uwb/UwbSession.hxx>
#include <uwb/protocols/fira/UwbException.hxx>
#include <uwb/protocols/fira/UwbOobConversions.hxx>

#include <magic_enum.hpp>
#include <plog/Log.h>

using namespace nearobject::cli;
using namespace uwb::protocol::fira;

void
NearObjectCliHandler::SetParent(NearObjectCli* parent)
{
    m_parent = parent;
    m_sessionEventCallbacks = std::make_shared<NearObjectCliUwbSessionEventCallbacks>([this]() {
        auto controlFlowContext = (m_parent != nullptr) ? m_parent->GetControlFlowContext() : nullptr;
        if (controlFlowContext != nullptr) {
            controlFlowContext->OperationSignalComplete();
        }
        m_activeSession.reset();
    });
}

std::shared_ptr<uwb::UwbDevice>
NearObjectCliHandler::ResolveUwbDevice(const NearObjectCliData& /*cliData */) noexcept
{
    // Default implementation does not know how to resolve a device. OS-specific
    // implementations are expected to sub-class and override this function.
    return nullptr;
}

void
NearObjectCliHandler::HandleDriverStartRanging(std::shared_ptr<uwb::UwbDevice> uwbDevice, const UwbRangingParameters& rangingParameters) noexcept
try {
    auto controlFlowContext = (m_parent != nullptr) ? m_parent->GetControlFlowContext() : nullptr;
    auto session = uwbDevice->CreateSession(rangingParameters.SessionId, m_sessionEventCallbacks);
    session->Configure(rangingParameters.ApplicationConfigurationParameters);
    auto applicationConfigurationParameters = session->GetApplicationConfigurationParameters({});
    PLOG_DEBUG << "Session Application Configuration Parameters: ";
    for (const auto& applicationConfigurationParameter : applicationConfigurationParameters) {
        PLOG_DEBUG << " > " << applicationConfigurationParameter.ToString();
    }
    session->StartRanging();
    bool destroySessionOnClose = true; // TODO: allow overriding this

    // Register a stop callback such that
    if (controlFlowContext != nullptr) {
        controlFlowContext->RegisterStopCallback([=]() {
            session->StopRanging();
            if (destroySessionOnClose) {
                session->Destroy();
            }
        });
    }

    // Save the session reference so it stays alive while the session is active.
    m_activeDevice = uwbDevice;
    m_activeSession = std::move(session);
} catch (...) {
    PLOG_ERROR << "failed to start ranging";
}

void
NearObjectCliHandler::HandleStartRanging(std::shared_ptr<uwb::UwbDevice> uwbDevice, uwb::protocol::fira::UwbSessionData& sessionData) noexcept
try {
    // Create a new session.
    auto session = uwbDevice->CreateSession(sessionData.sessionId, m_sessionEventCallbacks);

    // Convert configuration from OOB (UWB_SESSION_DATA) to UCI application
    // configuration parameters and configure the session with them.
    auto applicationConfigurationParameters = GetUciConfigParams(sessionData.uwbConfiguration, session->GetDeviceType());
    session->Configure(applicationConfigurationParameters);

    // Obtain the configured session application configuration parameters.
    auto applicationConfigurationParametersSet = session->GetApplicationConfigurationParameters({});
    PLOG_DEBUG << "Session Application Configuration Parameters: ";
    for (const auto& applicationConfigurationParameter : applicationConfigurationParametersSet) {
        PLOG_DEBUG << " > " << applicationConfigurationParameter.ToString();
    }

    // Start ranging.
    session->StartRanging();

    // Save the session reference so it stays alive while the session is active.
    m_activeSession = std::move(session);
} catch (...) {
    PLOG_ERROR << "failed to start ranging";
}

void
NearObjectCliHandler::HandleStopRanging() noexcept
try {
    // TODO
} catch (...) {
    PLOG_ERROR << "failed to stop ranging";
}

void
NearObjectCliHandler::HandleMonitorMode() noexcept
try {
    // TODO
} catch (...) {
    PLOG_ERROR << "failed to initiate monitor mode";
}

void
NearObjectCliHandler::HandleDeviceReset(std::shared_ptr<uwb::UwbDevice> uwbDevice) noexcept
try {
    uwbDevice->Reset();
} catch (...) {
    PLOG_ERROR << "failed to reset uwb device";
}

void
NearObjectCliHandler::HandleGetDeviceInfo(std::shared_ptr<uwb::UwbDevice> uwbDevice) noexcept
try {
    auto deviceInfo = uwbDevice->GetDeviceInformation();
    std::cout << deviceInfo.ToString() << std::endl;
} catch (...) {
    PLOG_ERROR << "failed to obtain device information";
}

void
NearObjectCliHandler::HandleSessionDeinitialize(std::shared_ptr<::uwb::UwbDevice> uwbDevice, uint32_t sessionId) noexcept
try {
    auto session = uwbDevice->GetSession(sessionId);
    if (session == nullptr) {
        PLOG_WARNING << "no session found with id " << sessionId;
        return;
    }

    session->Destroy();
} catch (...) {
    PLOG_ERROR << "failed to deinitialize session";
}

void
NearObjectCliHandler::HandleGetSessionCount(std::shared_ptr<::uwb::UwbDevice> uwbDevice) noexcept
try {
    auto sessionCount = uwbDevice->GetSessionCount();
    std::cout << "Session count: " << sessionCount << std::endl;
} catch (...) {
    PLOG_ERROR << "failed to get session count";
}

void
NearObjectCliHandler::HandleGetSessionState(std::shared_ptr<::uwb::UwbDevice> uwbDevice, uint32_t sessionId) noexcept
try {
    auto session = uwbDevice->GetSession(sessionId);
    if (session == nullptr) {
        PLOG_WARNING << "no session found with id " << sessionId;
        return;
    }

    auto sessionState = session->GetSessionState();
    std::cout << "Session state: " << magic_enum::enum_name(sessionState) << std::endl;

} catch (...) {
    PLOG_ERROR << "failed to get session state";
}
