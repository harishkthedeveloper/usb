#include "flutter_window.h"
#include <flutter/event_channel.h>
#include <flutter/event_sink.h>
#include <flutter/event_stream_handler_functions.h>
#include <flutter/method_channel.h>
#include <flutter/standard_method_codec.h>
#include <windows.h>
#include <initguid.h>
#include <Usbiodef.h>
#include <flutter/event_channel.h>
#include <flutter/event_sink.h>
#include <flutter/event_stream_handler_functions.h>
#include <flutter/method_channel.h>
#include <flutter/standard_method_codec.h>
#include <dbt.h>
#include <memory>
#include <optional>
#include <iostream>
#include <fstream>
#include "flutter/generated_plugin_registrant.h"
#include <errno.h>
#include <Windows.h>
#include <SetupAPI.h>
#include <vector>
#include <chrono>
#include <ctime> 
#include <future>
#include <string>

#include <locale>

//#pragma comment(lib, "Setupapi.lib")
/*
// stream overload for encodable value with string, (unhandled exception if string is not the embedded value)
std::ostream& operator <<(std::ostream& stream, const EncodableValue& value) {
    std::cout << std::get<std::string>(value) << "\n";
    return stream;
}
*/
using flutter::EncodableMap;
using flutter::EncodableValue;
using flutter::EncodableList;

static std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> usb_event;

constexpr const wchar_t* kUsbEventChannelName = L"usb_event_channel";
void MonitorUsbInsertion(flutter::EventSink<flutter::EncodableValue>* sink) {
    // Send an event to Flutter
    sink->Success(flutter::EncodableValue(true));
}

//// Function to convert wstring to UTF-8 string
//std::string wstring_to_utf8(const std::wstring& str)
//{
//    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
//    return myconv.to_bytes(str);
//}

FlutterWindow::FlutterWindow(const flutter::DartProject& project)
        : project_(project) {}

FlutterWindow::~FlutterWindow() {}


bool FlutterWindow::OnCreate() {
  if (!Win32Window::OnCreate()) {
    return false;
  }

  RECT frame = GetClientArea();

  // The size here must match the window dimensions to avoid unnecessary surface
  // creation / destruction in the startup path.
  flutter_controller_ = std::make_unique<flutter::FlutterViewController>(
      frame.right - frame.left, frame.bottom - frame.top, project_);
  // Ensure that basic setup of the controller was successful.
  if (!flutter_controller_->engine() || !flutter_controller_->view()) {
    return false;
  }
  RegisterPlugins(flutter_controller_->engine());
    flutter::MethodChannel<> channel(
            flutter_controller_->engine()->messenger(), "read-file-channel",
            &flutter::StandardMethodCodec::GetInstance());
    channel.SetMethodCallHandler(
            [this](const flutter::MethodCall<>& call,
               std::unique_ptr<flutter::MethodResult<>> result) {
                if (call.method_name() == "readFile") {
                    std::string filePath;
                    const auto* arguments = std::get_if<EncodableMap>(call.arguments());
                    if (arguments) {
                        auto path = arguments->find(EncodableValue("filePath"));
                        if (path != arguments->end()) {
                            filePath = std::get<std::string>(path->second);
                        }
                    }

                    std::ifstream file(filePath);
                    std::cerr << filePath << "\n";
                    if (!file.is_open()) {
                        std::cerr<< "file opening error code : " << errno << std::endl;
                        return;
                    }
                    std::string fileContent;
                    std::string line;
                    while (std::getline(file, line)) {
                        fileContent += line + "\n";
                        //std::cout << line << std::endl;
                    }
                    file.close();
                    result->Success(flutter::EncodableValue(fileContent));
                } else if (call.method_name() == "getUsbList"){
                    flutter::EncodableValue res = getUSBDevicesWithDriveNames();
                    result->Success(res);
                } else {
                    result->NotImplemented();
                }
            });
    flutter::EventChannel<> charging_channel(
            flutter_controller_->engine()->messenger(), "usb-event",
            &flutter::StandardMethodCodec::GetInstance());
    charging_channel.SetStreamHandler(
            std::make_unique<flutter::StreamHandlerFunctions<>>(
                    [this](auto arguments, auto events) {
                        this->OnStreamListen(std::move(events));
                        return nullptr;
                    },
                    [this](auto arguments) {
                        this->OnStreamCancel();
                        return nullptr;
                    }));

  SetChildContent(flutter_controller_->view()->GetNativeWindow());

  flutter_controller_->engine()->SetNextFrameCallback([&]() {
    this->Show();
  });

  // Flutter can complete the first frame before the "show window" callback is
  // registered. The following call ensures a frame is pending to ensure the
  // window is shown. It is a no-op if the first frame hasn't completed yet.
  flutter_controller_->ForceRedraw();

  return true;
}

void FlutterWindow::OnDestroy() {
  if (flutter_controller_) {
    flutter_controller_ = nullptr;
  }

  Win32Window::OnDestroy();
}

LRESULT
FlutterWindow::MessageHandler(HWND hwnd, UINT const message,
                              WPARAM const wparam,
                              LPARAM const lparam) noexcept {
  // Give Flutter, including plugins, an opportunity to handle window messages.
  if (flutter_controller_) {
    std::optional<LRESULT> result =
        flutter_controller_->HandleTopLevelWindowProc(hwnd, message, wparam,
                                                      lparam);
    if (result) {
      return *result;
    }
  }
    switch (message) {
    case WM_CREATE: {
        DEV_BROADCAST_DEVICEINTERFACE devInf = {0};
        devInf.dbcc_size = sizeof(devInf);
        devInf.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        devInf.dbcc_classguid = GUID_DEVINTERFACE_USB_DEVICE;
        device_notify = RegisterDeviceNotification(hwnd, &devInf, DEVICE_NOTIFY_WINDOW_HANDLE);
            if(NULL == device_notify){
                std::cout << "Notification registration failure \n";
            }
        }break;
    case WM_DEVICECHANGE: {
            switch(wparam) {
            case DBT_DEVICEARRIVAL: {
                    PDEV_BROADCAST_DEVICEINTERFACE_W devInterface = (PDEV_BROADCAST_DEVICEINTERFACE_W)lparam;
                        if (devInterface->dbcc_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
                            SendBatteryStateEvent();
                            std::wcout << "Device Arrived:" << std::endl;
                            }
            }break;
            case DBT_DEVICEREMOVECOMPLETE: {
                    PDEV_BROADCAST_DEVICEINTERFACE_W devInterface = (PDEV_BROADCAST_DEVICEINTERFACE_W)lparam;
                    if (devInterface->dbcc_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
                    SendBatteryStateEvent();
                    std::cout << "Device unplugged..!!";
                    }

            }break;
            }
    }break;
    case WM_FONTCHANGE:
        flutter_controller_->engine()->ReloadSystemFonts();
    break;
}


        switch (message) {
        case WM_FONTCHANGE:
        flutter_controller_->engine()->ReloadSystemFonts();
        break;
        }

  return Win32Window::MessageHandler(hwnd, message, wparam, lparam);
}
flutter::EncodableValue FlutterWindow::getUSBDevicesWithDriveNames()
{
    //std::cout << __FUNCTION__ << "\n";
    std::vector<std::string> removableDrives;
    DWORD drives = GetLogicalDrives(); // Get bit mask representing all drives
    std::wcout << "get logical drives:" << std::endl;
   
    for (int i = 0; i < 26; ++i) {
        if (drives & (1 << i)) { // Check if drive letter is valid
            wchar_t driveLetter[] = { static_cast<wchar_t>(L'A' + i), L':', L'\\', L'\0' };
            UINT driveType = GetDriveType(driveLetter); // Get the type of drive
            if (driveType == DRIVE_REMOVABLE) { // Check if the drive is removable (USB)
                std::wstring driveString(driveLetter);
                std::string Jsting = "";
                
                for(wchar_t wc : driveString) {
                    Jsting.push_back(static_cast<char>(wc)); 

                }
                Jsting.append("\\");
                //std::cout << "<<<<<" << Jsting << std::endl;
                removableDrives.push_back(Jsting);
            }
        }
    }
  
 


    // Build the JSON string manually
    std::string jsonStr = "[";
    for (size_t i = 0; i < removableDrives.size(); ++i) {
        jsonStr += "\"" + removableDrives[i] + "\"";
        if (i != removableDrives.size() - 1) {
            jsonStr += ", ";
        }
    }
    jsonStr += "]";
    // return an EncodableValue from the JSON string
   // std::cout << ">>>>>" << jsonStr << std::endl;
    return flutter::EncodableValue(jsonStr);
}
void FlutterWindow::OnStreamListen(
        std::unique_ptr<flutter::EventSink<>>&& events) {
        event_sink_ = std::move(events);
}

void FlutterWindow::OnStreamCancel() { event_sink_ = nullptr; }

void FlutterWindow::SendBatteryStateEvent() {
    event_sink_->Success(flutter::EncodableValue("USB Detected"));
}
