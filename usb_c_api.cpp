#include "include/usb_detect/usb_detect_plugin_c_api.h"

#include <flutter/plugin_registrar_windows.h>

#include "usb_detect_plugin.h"

void UsbDetectPluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  usb_detect::UsbDetectPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}

