
#include <cerrno>
#include <stdexcept>
#include <string>

#include <unistd.h>

#include <nearobject/service/ServiceRuntime.hxx>
#include <nearobject/service/NearObjectService.hxx>
#include <nearobject/service/NearObjectServiceInjector.hxx>
#include <nearobject/service/NearObjectServiceConfiguration.hxx>

int
main(int argc, char *argv[])
{
    auto configuration = nearobject::service::NearObjectServiceConfiguration::FromCommandLineArguments(argc, argv);

    // Daemonize, if requested.
    if (configuration.RunInBackground) {
        constexpr int nochdir = 0;  // Change current working directory to /
        constexpr int noclose = 0;  // Don't redirect stdin, stdout to /dev/null
        if (daemon(nochdir, noclose) != 0) {
            int error = errno;
            const std::string what = "failed to daemonize (error=" + std::to_string(error) + ")";
            throw std::runtime_error(what);
        }
    }

    // TODO: create and fill injector components
    nearobject::service::NearObjectServiceInjector injector{};
    auto service = nearobject::service::NearObjectService::Create(injector);

    nearobject::service::ServiceRuntime nearObjectServiceRuntime{};
    nearObjectServiceRuntime.SetServiceInstance(service).Start();
    return 0;
}