
#ifndef __SERVICE_RUNTIME_HXX__
#define __SERVICE_RUNTIME_HXX__

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace nearobject
{
namespace service
{
class ServiceRuntime
{
public:
    void
    Start();

    void
    Stop();

    void
    HandleEvent();

    void
    Run();

private:
    bool m_running = false;
    std::thread m_threadMain;
    std::mutex m_runEventGate;
    std::condition_variable m_runEvent;
};

} // namespace service
} // namespace nearobject

#endif // __SERVICE_RUNTIME_HXX__