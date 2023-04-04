
add_library(
  nearobject-winrt
  NearObject.cpp NearObject.h NearObject.idl
)

set_property(
    SOURCE NearObject.idl
    PROPERTY VS_SETTINGS
    "SubType=Code"
)
set_property(
    SOURCE NearObject.cpp NearObject.h
    PROPERTY VS_SETTINGS
    "DependentUpon=NearObject.idl"
)
set_property(
  TARGET nearobject-winrt
  PROPERTY VS_PACKAGE_REFERENCES
  "Microsoft.Windows.CppWinRT.2.0.220531.1"
)
target_include_directories(
  nearobject-winrt
  PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
)
set_target_properties(
  nearobject-winrt
  PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  VS_GLOBAL_RootNamespace Windows.Devices.NearObject
  VS_GLOBAL_AppContainerApplication true
  VS_GLOBAL_AppxPackage false
  VS_GLOBAL_CppWinRTOptimized true
  VS_GLOBAL_CppWinRTRootNamespaceAutoMerge true
  VS_GLOBAL_CppWinRTGenerateWindowsMetadata true
  VS_GLOBAL_MinimalCoreWin true
  VS_GLOBAL_DefaultLanguage en-US
  VS_GLOBAL_MinimumVisualStudioVersion 14.0
  VS_GLOBAL_ApplicationType "Windows Store"
  VS_GLOBAL_ApplicationTypeRevision 10.0
  VS_GLOBAL_WindowsPackageType None
  VS_GLOBAL_WindowsPackageType None
  VS_GLOBAL_WindowsTargetPlatformMinVersion 10.0.17134.0
# <WindowsTargetPlatformVersion Condition=" '$(WindowsTargetPlatformVersion)' == '' ">10.0.22621.0</WindowsTargetPlatformVersion>
# <WindowsTargetPlatformMinVersion>10.0.17134.0</WindowsTargetPlatformMinVersion>
)
