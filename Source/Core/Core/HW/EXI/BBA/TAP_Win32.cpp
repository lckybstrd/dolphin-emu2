// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "Core/HW/EXI/BBA/TAP_Win32.h"
#include "Common/Assert.h"
#include "Common/Logging/Log.h"
#include "Common/MsgHandler.h"
#include "Common/StringUtil.h"
#include "Core/HW/EXI/EXI_Device.h"
#include "Core/HW/EXI/EXI_DeviceEthernet.h"

namespace Win32TAPHelper
{
bool IsTAPDevice(const TCHAR* guid)
{
  HKEY netcard_key;
  LONG status;
  DWORD len;
  int i = 0;

  status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, ADAPTER_KEY, 0, KEY_READ, &netcard_key);

  if (status != ERROR_SUCCESS)
    return false;

  for (;;)
  {
    TCHAR enum_name[256];
    TCHAR unit_string[256];
    HKEY unit_key;
    TCHAR component_id_string[] = _T("ComponentId");
    TCHAR component_id[256];
    TCHAR net_cfg_instance_id_string[] = _T("NetCfgInstanceId");
    TCHAR net_cfg_instance_id[256];
    DWORD data_type;

    len = sizeof(enum_name);
    status = RegEnumKeyEx(netcard_key, i, enum_name, &len, nullptr, nullptr, nullptr, nullptr);

    if (status == ERROR_NO_MORE_ITEMS)
      break;
    else if (status != ERROR_SUCCESS)
      return false;

    _sntprintf(unit_string, sizeof(unit_string), _T("%s\\%s"), ADAPTER_KEY, enum_name);

    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, unit_string, 0, KEY_READ, &unit_key);

    if (status != ERROR_SUCCESS)
    {
      return false;
    }
    else
    {
      len = sizeof(component_id);
      status = RegQueryValueEx(unit_key, component_id_string, nullptr, &data_type,
                               (LPBYTE)component_id, &len);

      if (!(status != ERROR_SUCCESS || data_type != REG_SZ))
      {
        len = sizeof(net_cfg_instance_id);
        status = RegQueryValueEx(unit_key, net_cfg_instance_id_string, nullptr, &data_type,
                                 (LPBYTE)net_cfg_instance_id, &len);

        if (status == ERROR_SUCCESS && data_type == REG_SZ)
        {
          if (!_tcscmp(component_id, TAP_COMPONENT_ID) && !_tcscmp(net_cfg_instance_id, guid))
          {
            RegCloseKey(unit_key);
            RegCloseKey(netcard_key);
            return true;
          }
        }
      }
      RegCloseKey(unit_key);
    }
    ++i;
  }

  RegCloseKey(netcard_key);
  return false;
}

bool GetGUIDs(std::vector<std::basic_string<TCHAR>>& guids)
{
  LONG status;
  HKEY control_net_key;
  DWORD len;
  DWORD cSubKeys = 0;

  status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, NETWORK_CONNECTIONS_KEY, 0, KEY_READ | KEY_QUERY_VALUE,
                        &control_net_key);

  if (status != ERROR_SUCCESS)
    return false;

  status = RegQueryInfoKey(control_net_key, nullptr, nullptr, nullptr, &cSubKeys, nullptr, nullptr,
                           nullptr, nullptr, nullptr, nullptr, nullptr);

  if (status != ERROR_SUCCESS)
    return false;

  for (DWORD i = 0; i < cSubKeys; i++)
  {
    TCHAR enum_name[256];
    TCHAR connection_string[256];
    HKEY connection_key;
    TCHAR name_data[256];
    DWORD name_type;
    const TCHAR name_string[] = _T("Name");

    len = sizeof(enum_name);
    status = RegEnumKeyEx(control_net_key, i, enum_name, &len, nullptr, nullptr, nullptr, nullptr);

    if (status != ERROR_SUCCESS)
      continue;

    _sntprintf(connection_string, sizeof(connection_string), _T("%s\\%s\\Connection"),
               NETWORK_CONNECTIONS_KEY, enum_name);

    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, connection_string, 0, KEY_READ, &connection_key);

    if (status == ERROR_SUCCESS)
    {
      len = sizeof(name_data);
      status = RegQueryValueEx(connection_key, name_string, nullptr, &name_type, (LPBYTE)name_data,
                               &len);

      if (status != ERROR_SUCCESS || name_type != REG_SZ)
      {
        continue;
      }
      else
      {
        if (IsTAPDevice(enum_name))
        {
          guids.push_back(enum_name);
        }
      }

      RegCloseKey(connection_key);
    }
  }

  RegCloseKey(control_net_key);

  return !guids.empty();
}

bool OpenTAP(HANDLE& adapter, const std::basic_string<TCHAR>& device_guid)
{
  auto const device_path = USERMODEDEVICEDIR + device_guid + TAPSUFFIX;

  adapter = CreateFile(device_path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                       FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED, nullptr);

  if (adapter == INVALID_HANDLE_VALUE)
  {
    INFO_LOG(SP1, "Failed to open TAP at %s", device_path.c_str());
    return false;
  }
  return true;
}

}  // namespace Win32TAPHelper

namespace ExpansionInterface
{
bool CEXIETHERNET::TAPPhysicalNetworkInterface::Activate()
{
  if (IsActivated())
    return true;

  DWORD len;
  std::vector<std::basic_string<TCHAR>> device_guids;

  if (!Win32TAPHelper::GetGUIDs(device_guids))
  {
    ERROR_LOG(SP1, "Failed to find a TAP GUID");
    return false;
  }

  for (size_t i = 0; i < device_guids.size(); i++)
  {
    if (Win32TAPHelper::OpenTAP(mHAdapter, device_guids.at(i)))
    {
      INFO_LOG(SP1, "OPENED %s", device_guids.at(i).c_str());
      break;
    }
  }
  if (mHAdapter == INVALID_HANDLE_VALUE)
  {
    PanicAlert("Failed to open any TAP");
    return false;
  }

  /* get driver version info */
  ULONG info[3];
  if (DeviceIoControl(mHAdapter, TAP_IOCTL_GET_VERSION, &info, sizeof(info), &info, sizeof(info),
                      &len, nullptr))
  {
    INFO_LOG(SP1, "TAP-Win32 Driver Version %d.%d %s", info[0], info[1], info[2] ? "(DEBUG)" : "");
  }
  if (!(info[0] > TAP_WIN32_MIN_MAJOR ||
        (info[0] == TAP_WIN32_MIN_MAJOR && info[1] >= TAP_WIN32_MIN_MINOR)))
  {
    PanicAlertT("ERROR: This version of Dolphin requires a TAP-Win32 driver"
                " that is at least version %d.%d -- If you recently upgraded your Dolphin"
                " distribution, a reboot is probably required at this point to get"
                " Windows to see the new driver.",
                TAP_WIN32_MIN_MAJOR, TAP_WIN32_MIN_MINOR);
    return false;
  }

  /* set driver media status to 'connected' */
  ULONG status = TRUE;
  if (!DeviceIoControl(mHAdapter, TAP_IOCTL_SET_MEDIA_STATUS, &status, sizeof(status), &status,
                       sizeof(status), &len, nullptr))
  {
    ERROR_LOG(SP1, "WARNING: The TAP-Win32 driver rejected a"
                   "TAP_IOCTL_SET_MEDIA_STATUS DeviceIoControl call.");
    return false;
  }

  /* initialize read/write events */
  mReadOverlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
  mWriteOverlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
  if (mReadOverlapped.hEvent == nullptr || mWriteOverlapped.hEvent == nullptr)
    return false;

  mWriteBuffer.reserve(1518);
  return RecvInit();
}

void CEXIETHERNET::TAPPhysicalNetworkInterface::Deactivate()
{
  if (!IsActivated())
    return;

  // Signal read thread to exit.
  readEnabled.Clear();
  readThreadShutdown.Set();

  // Cancel any outstanding requests from both this thread (writes), and the read thread.
  CancelIoEx(mHAdapter, nullptr);

  // Wait for read thread to exit.
  if (readThread.joinable())
    readThread.join();

  // Clean-up handles
  CloseHandle(mReadOverlapped.hEvent);
  CloseHandle(mWriteOverlapped.hEvent);
  CloseHandle(mHAdapter);
  mHAdapter = INVALID_HANDLE_VALUE;
  memset(&mReadOverlapped, 0, sizeof(mReadOverlapped));
  memset(&mWriteOverlapped, 0, sizeof(mWriteOverlapped));
}

bool CEXIETHERNET::TAPPhysicalNetworkInterface::IsActivated()
{
  return mHAdapter != INVALID_HANDLE_VALUE;
}

void CEXIETHERNET::TAPPhysicalNetworkInterface::ReadThreadHandler(TAPPhysicalNetworkInterface* self)
{
  while (!self->readThreadShutdown.IsSet())
  {
    DWORD transferred;

    // Read from TAP into internal buffer.
    if (ReadFile(self->mHAdapter, self->m_eth_ref->mRecvBuffer.get(), BBA_RECV_SIZE, &transferred,
                 &self->mReadOverlapped))
    {
      // Returning immediately is not likely to happen, but if so, reset the event state manually.
      ResetEvent(self->mReadOverlapped.hEvent);
    }
    else
    {
      // IO should be pending.
      if (GetLastError() != ERROR_IO_PENDING)
      {
        ERROR_LOG(SP1, "ReadFile failed (err=0x%X)", GetLastError());
        continue;
      }

      // Block until the read completes.
      if (!GetOverlappedResult(self->mHAdapter, &self->mReadOverlapped, &transferred, TRUE))
      {
        // If CancelIO was called, we should exit (the flag will be set).
        if (GetLastError() == ERROR_OPERATION_ABORTED)
          continue;

        // Something else went wrong.
        ERROR_LOG(SP1, "GetOverlappedResult failed (err=0x%X)", GetLastError());
        continue;
      }
    }

    // Copy to BBA buffer, and fire interrupt if enabled.
    DEBUG_LOG(SP1, "Received %u bytes:\n %s", transferred,
              ArrayToString(self->mRecvBuffer.get(), transferred, 0x10).c_str());
    if (self->readEnabled.IsSet())
    {
      self->m_eth_ref->mRecvBufferLength = transferred;
      self->m_eth_ref->RecvHandlePacket();
    }
  }
}

bool CEXIETHERNET::TAPPhysicalNetworkInterface::SendFrame(const u8* frame, u32 size)
{
  DEBUG_LOG(SP1, "SendFrame %u bytes:\n%s", size, ArrayToString(frame, size, 0x10).c_str());

  // Check for a background write. We can't issue another one until this one has completed.
  DWORD transferred;
  if (mWritePending)
  {
    // Wait for previous write to complete.
    if (!GetOverlappedResult(mHAdapter, &mWriteOverlapped, &transferred, TRUE))
      ERROR_LOG(SP1, "GetOverlappedResult failed (err=0x%X)", GetLastError());
  }

  // Copy to write buffer.
  mWriteBuffer.assign(frame, frame + size);
  mWritePending = true;

  // Queue async write.
  if (WriteFile(mHAdapter, mWriteBuffer.data(), size, &transferred, &mWriteOverlapped))
  {
    // Returning immediately is not likely to happen, but if so, reset the event state manually.
    ResetEvent(mWriteOverlapped.hEvent);
  }
  else
  {
    // IO should be pending.
    if (GetLastError() != ERROR_IO_PENDING)
    {
      ERROR_LOG(SP1, "WriteFile failed (err=0x%X)", GetLastError());
      ResetEvent(mWriteOverlapped.hEvent);
      mWritePending = false;
      return false;
    }
  }

  // Always report the packet as being sent successfully, even though it might be a lie
  m_eth_ref->SendComplete();
  return true;
}

bool CEXIETHERNET::TAPPhysicalNetworkInterface::RecvInit()
{
  readThread = std::thread(ReadThreadHandler, this);
  return true;
}

void CEXIETHERNET::TAPPhysicalNetworkInterface::RecvStart()
{
  readEnabled.Set();
}

void CEXIETHERNET::TAPPhysicalNetworkInterface::RecvStop()
{
  readEnabled.Clear();
}
}  // namespace ExpansionInterface
