#ifndef RUNNER_FLUTTER_WINDOW_H_
#define RUNNER_FLUTTER_WINDOW_H_

#include <flutter/dart_project.h>
#include <flutter/event_sink.h>
#include <flutter/flutter_view_controller.h>
#include <winuser.h>
#include <memory>
#include "win32_window.h"
using namespace flutter;
// A window that does nothing but host a Flutter view.
class FlutterWindow : public Win32Window {
 public:
  // Creates a new FlutterWindow hosting a Flutter view running |project|.
  explicit FlutterWindow(const flutter::DartProject& project);
  virtual ~FlutterWindow();
  

 protected:
  // Win32Window:
  bool OnCreate() override;
  void OnDestroy() override;
  LRESULT MessageHandler(HWND window, UINT const message, WPARAM const wparam,
                         LPARAM const lparam) noexcept override;

  flutter::EncodableValue getUSBDevicesWithDriveNames();

private:
    // The project to run.
    flutter::DartProject project_;

    // The Flutter instance hosted by this window.
    std::unique_ptr<flutter::FlutterViewController> flutter_controller_;
    // EventStream handlers:
    void OnStreamListen(std::unique_ptr<flutter::EventSink<>>&& events);
    void OnStreamCancel();

    // Sends a state event to |event_sink_| with the current charging status.
    void SendBatteryStateEvent();
   

    std::unique_ptr<flutter::EventSink<>> event_sink_;
    HPOWERNOTIFY power_notification_handle_ = nullptr;
    HDEVNOTIFY device_notify;
};


#endif  // RUNNER_FLUTTER_WINDOW_H_
