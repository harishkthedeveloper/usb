#ifndef FLUTTER_PLUGIN_USB_DETECT_PLUGIN_H_
#define FLUTTER_PLUGIN_USB_DETECT_PLUGIN_H_

#include <flutter/event_channel.h>
#include <flutter/event_stream_handler_functions.h>
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/event_sink.h>
#include <winuser.h>
#include <memory>
#include <flutter/flutter_view_controller.h>


namespace usb_detect {

class UsbDetectPlugin : public flutter::Plugin,
                        flutter::StreamHandler<flutter::EncodableValue> {
protected:
    /*std::optional<LRESULT> MessageHandler(HWND window, UINT const message, WPARAM const wparam,
                           LPARAM const lparam) noexcept override;*/
    flutter::EncodableValue getUSBDevicesWithDriveNames();
    LRESULT MessageHandler(HWND window, UINT const message, WPARAM const wparam,
                           LPARAM const lparam) ;
private:
    std::unique_ptr<flutter::FlutterViewController> flutter_controller_;
    std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> event_sink_;
    void SendBatteryStateEvent();
    HPOWERNOTIFY power_notification_handle_ = nullptr;
    HDEVNOTIFY device_notify;
    int32_t window_proc_id_ = -1;

    std::optional<LRESULT> HandleWindowProc(HWND hwnd,
                                            UINT message,
                                            WPARAM wparam,
                                            LPARAM lparam);


public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

  UsbDetectPlugin(flutter::PluginRegistrarWindows* registrar);

  virtual ~UsbDetectPlugin();

  // Disallow copy and assign.
  UsbDetectPlugin(const UsbDetectPlugin&) = delete;
  UsbDetectPlugin& operator=(const UsbDetectPlugin&) = delete;

  // Called when a method is called on this plugin's channel from Dart.
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

  std::unique_ptr<flutter::StreamHandlerError<>> OnListenInternal(
            const flutter::EncodableValue* arguments,
            std::unique_ptr<flutter::EventSink<>>&& events) override;

  std::unique_ptr<flutter::StreamHandlerError<>> OnCancelInternal(
            const flutter::EncodableValue* arguments) override;

};

}  // namespace usb_detect

#endif  // FLUTTER_PLUGIN_USB_DETECT_PLUGIN_H_
