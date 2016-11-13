// Copyright 2017 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "Common/CommonTypes.h"
#include "Common/Flag.h"
#include "Core/IOS/Device.h"
#include "Core/IOS/IPC.h"
#include "Core/IOS/USB/Common.h"

class PointerWrap;
struct libusb_context;

namespace IOS
{
namespace HLE
{
namespace Device
{
// Common base class for USB host devices (such as /dev/usb/oh0 and /dev/usb/ven).
class USBHost : public Device
{
public:
  USBHost(u32 device_id, const std::string& device_name);
  virtual ~USBHost();

  ReturnCode Open(const OpenRequest& request) override;

  void UpdateWantDeterminism(bool new_want_determinism) override;
  void DoState(PointerWrap& p) override;

protected:
  enum class ChangeEvent
  {
    Inserted,
    Removed,
  };
  using DeviceChangeHooks = std::map<std::shared_ptr<USB::Device>, ChangeEvent>;

  std::map<u64, std::shared_ptr<USB::Device>> m_devices;
  mutable std::mutex m_devices_mutex;

  bool UpdateDevices(bool always_add_hooks = false);

  bool AddDevice(std::unique_ptr<USB::Device> device);
  std::shared_ptr<USB::Device> GetDeviceById(u64 device_id) const;

  virtual void OnDeviceChange(ChangeEvent event, std::shared_ptr<USB::Device> changed_device) = 0;
  virtual void OnDeviceChangeEnd() {}
  virtual bool ShouldAddDevice(const USB::Device& device) const { return true; }
  void StartThreads();
  void StopThreads();

private:
  bool AddNewDevices(std::set<u64>* plugged_devices, DeviceChangeHooks* hooks,
                     bool always_add_hooks);
  void DetectRemovedDevices(const std::set<u64>& plugged_devices, DeviceChangeHooks* hooks);
  void DispatchHooks(const DeviceChangeHooks& hooks);

#ifdef __LIBUSB__
  libusb_context* m_libusb_context = nullptr;
#endif
  // Event thread for libusb
  Common::Flag m_thread_running;
  std::thread m_thread;
  // Device scanning thread
  Common::Flag m_scan_thread_running;
  std::thread m_scan_thread;
};
}  // namespace Device
}  // namespace HLE
}  // namespace IOS
