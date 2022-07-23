
#ifndef __OOB_CHANNEL_BLUETOOTH_HXX__
#define __OOB_CHANNEL_BLUETOOTH_HXX__

#include "OobChannel.hxx"

namespace nearobject
{
class OobChannelBluetooth :
    public OobChannel
{
public:
    std::unique_ptr<OobConnection>
    EstablishConnectionImpl() override;
};

} // namespace nearobject

#endif // __OOB_CHANNEL_HXX__
