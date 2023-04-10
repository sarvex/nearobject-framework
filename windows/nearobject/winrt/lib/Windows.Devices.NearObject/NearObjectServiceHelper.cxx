
#include "pch.h"
#include "NearObjectServiceHelper.hxx"

using namespace windows::devices::nearobject::system;
using winrt::Windows::Devices::NearObject::System::NearObjectService;

NearObjectService
windows::devices::nearobject::system::InstantiateService()
{
	NearObjectService nearObjectService{};
	// TODO: this needs to account for the user identity and security
	// This is just here as a placeholder to activate the out-of-process
	// WinRT OOP server hosting the service objects.
	return nearObjectService;
}
