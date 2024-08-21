// Copyright 2009 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Core/IOS/USB/USB_KBD.h"

#include <array>
#include <optional>
#include <queue>
#include <string>

#include "Common/FileUtil.h"
#include "Common/IniFile.h"
#include "Common/Logging/Log.h"
#include "Common/Swap.h"
#include "Core/Config/MainSettings.h"
#include "Core/Core.h"  // Local core functions
#include "Core/HW/Memmap.h"
#include "Core/System.h"
#include "InputCommon/ControlReference/ControlReference.h"  // For background input check

#ifdef _WIN32
#include <windows.h>
#endif

namespace IOS::HLE
{
namespace
{
// Crazy ugly
#ifdef _WIN32
constexpr std::array<u8, 256> s_key_codes_qwerty{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x2A,  // Backspace
    0x2B,  // Tab
    0x00, 0x00,
    0x00,  // Clear
    0x28,  // Return
    0x00, 0x00,
    0x00,  // Shift
    0x00,  // Control
    0x00,  // ALT
    0x48,  // Pause
    0x39,  // Capital
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x29,  // Escape
    0x00, 0x00, 0x00, 0x00,
    0x2C,  // Space
    0x4B,  // Prior
    0x4E,  // Next
    0x4D,  // End
    0x4A,  // Home
    0x50,  // Left
    0x52,  // Up
    0x4F,  // Right
    0x51,  // Down
    0x00, 0x00, 0x00,
    0x46,  // Print screen
    0x49,  // Insert
    0x4C,  // Delete
    0x00,
    // 0 -> 9
    0x27, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00,
    // A -> Z
    0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13,
    0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x00, 0x00, 0x00, 0x00, 0x00,
    // Numpad 0 -> 9
    0x62, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61,
    0x55,  // Multiply
    0x57,  // Add
    0x00,  // Separator
    0x56,  // Subtract
    0x63,  // Decimal
    0x54,  // Divide
    // F1 -> F12
    0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45,
    // F13 -> F24
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x53,  // Numlock
    0x47,  // Scroll lock
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // Modifier keys
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x33,  // ';'
    0x2E,  // Plus
    0x36,  // Comma
    0x2D,  // Minus
    0x37,  // Period
    0x38,  // '/'
    0x35,  // '~'
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x2F,  // '['
    0x32,  // '\'
    0x30,  // ']'
    0x34,  // '''
    0x00,  //
    0x00,  // Nothing interesting past this point.
};

constexpr std::array<u8, 256> s_key_codes_azerty{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x2A,  // Backspace
    0x2B,  // Tab
    0x00, 0x00,
    0x00,  // Clear
    0x28,  // Return
    0x00, 0x00,
    0x00,  // Shift
    0x00,  // Control
    0x00,  // ALT
    0x48,  // Pause
    0x39,  // Capital
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x29,  // Escape
    0x00, 0x00, 0x00, 0x00,
    0x2C,  // Space
    0x4B,  // Prior
    0x4E,  // Next
    0x4D,  // End
    0x4A,  // Home
    0x50,  // Left
    0x52,  // Up
    0x4F,  // Right
    0x51,  // Down
    0x00, 0x00, 0x00,
    0x46,  // Print screen
    0x49,  // Insert
    0x4C,  // Delete
    0x00,
    // 0 -> 9
    0x27, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00,
    // A -> Z
    0x14, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x33, 0x11, 0x12, 0x13,
    0x04, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1D, 0x1B, 0x1C, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x00,
    // Numpad 0 -> 9
    0x62, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61,
    0x55,  // Multiply
    0x57,  // Add
    0x00,  // Separator
    0x56,  // Substract
    0x63,  // Decimal
    0x54,  // Divide
    // F1 -> F12
    0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45,
    // F13 -> F24
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x53,  // Numlock
    0x47,  // Scroll lock
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // Modifier keys
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x30,  // '$'
    0x2E,  // Plus
    0x10,  // Comma
    0x00,  // Minus
    0x36,  // Period
    0x37,  // '/'
    0x34,  // ' '
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x2D,  // ')'
    0x32,  // '\'
    0x2F,  // '^'
    0x00,  // ' '
    0x38,  // '!'
    0x00,  // Nothing interesting past this point.
};
#else
constexpr std::array<u8, 256> s_key_codes_qwerty{};

constexpr std::array<u8, 256> s_key_codes_azerty{};
#endif
}  // Anonymous namespace

USB_KBD::MessageData::MessageData(MessageType type, u8 modifiers_, PressedKeyData pressed_keys_)
    : msg_type{Common::swap32(static_cast<u32>(type))}, modifiers{modifiers_}, pressed_keys{
                                                                                   pressed_keys_}
{
}

// TODO: support in netplay/movies.

USB_KBD::USB_KBD(EmulationKernel& ios, const std::string& device_name)
    : EmulationDevice(ios, device_name)
{
}

std::optional<IPCReply> USB_KBD::Open(const OpenRequest& request)
{
  INFO_LOG_FMT(IOS, "USB_KBD: Open");
  Common::IniFile ini;
  ini.Load(File::GetUserPath(F_DOLPHINCONFIG_IDX));
  ini.GetOrCreateSection("USB Keyboard")->Get("Layout", &m_keyboard_layout, KBD_LAYOUT_QWERTY);

  m_message_queue = {};
  m_old_key_buffer.fill(false);
  m_old_modifiers = 0x00;

  // m_message_queue.emplace(MessageType::KeyboardConnect, 0, PressedKeyData{});
  return Device::Open(request);
}

std::optional<IPCReply> USB_KBD::Write(const ReadWriteRequest& request)
{
  // Stubbed.
  return IPCReply(IPC_SUCCESS);
}

std::optional<IPCReply> USB_KBD::IOCtl(const IOCtlRequest& request)
{
  if (request.request != 0)
    return IPCReply(IPC_EINVAL);

  if (m_pending_request.has_value())
    return IPCReply(IPC_EEXIST);

  // Blocks until a new event is available
  m_pending_request = request.address;
  return std::nullopt;
}

void USB_KBD::DoState(PointerWrap& p)
{
  Device::DoState(p);
  p.Do(m_pending_request);
}

bool USB_KBD::IsKeyPressed(int key) const
{
#ifdef _WIN32
  return (GetAsyncKeyState(key) & 0x8000) != 0;
#else
  // TODO: do it for non-Windows platforms
  return false;
#endif
}

void USB_KBD::Update()
{
  if (!Config::Get(Config::MAIN_WII_KEYBOARD) || Core::WantsDeterminism() || !m_is_active)
    return;

  u8 modifiers = 0x00;
  PressedKeyData pressed_keys{0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  bool got_event = false;
  size_t num_pressed_keys = 0;
  for (size_t i = 0; i < m_old_key_buffer.size(); i++)
  {
    const bool key_pressed_now = IsKeyPressed(static_cast<int>(i));
    const bool key_pressed_before = m_old_key_buffer[i];
    u8 key_code = 0;

    if (key_pressed_now ^ key_pressed_before)
    {
      if (key_pressed_now)
      {
        switch (m_keyboard_layout)
        {
        case KBD_LAYOUT_QWERTY:
          key_code = s_key_codes_qwerty[i];
          break;

        case KBD_LAYOUT_AZERTY:
          key_code = s_key_codes_azerty[i];
          break;
        }

        if (key_code == 0x00)
          continue;

        pressed_keys[num_pressed_keys] = key_code;

        num_pressed_keys++;
        if (num_pressed_keys == pressed_keys.size())
          break;
      }

      got_event = true;
    }

    m_old_key_buffer[i] = key_pressed_now;
  }

#ifdef _WIN32
  if (GetAsyncKeyState(VK_LCONTROL) & 0x8000)
    modifiers |= 0x01;
  if (GetAsyncKeyState(VK_LSHIFT) & 0x8000)
    modifiers |= 0x02;
  if (GetAsyncKeyState(VK_MENU) & 0x8000)
    modifiers |= 0x04;
  if (GetAsyncKeyState(VK_LWIN) & 0x8000)
    modifiers |= 0x08;
  if (GetAsyncKeyState(VK_RCONTROL) & 0x8000)
    modifiers |= 0x10;
  if (GetAsyncKeyState(VK_RSHIFT) & 0x8000)
    modifiers |= 0x20;
  // TODO: VK_MENU is for ALT, not for ALT GR (ALT GR seems to work though...)
  if (GetAsyncKeyState(VK_MENU) & 0x8000)
    modifiers |= 0x40;
  if (GetAsyncKeyState(VK_RWIN) & 0x8000)
    modifiers |= 0x80;
#else
// TODO: modifiers for non-Windows platforms
#endif

  if (modifiers ^ m_old_modifiers)
  {
    got_event = true;
    m_old_modifiers = modifiers;
  }

  if (got_event)
    m_message_queue.emplace(MessageType::Event, modifiers, pressed_keys);

  if (ControlReference::GetInputGate() && !m_message_queue.empty() && m_pending_request.has_value())
  {
    auto& system = GetSystem();
    auto& memory = system.GetMemory();
    IOCtlRequest pending_request{GetSystem(), m_pending_request.value()};
    memory.CopyToEmu(pending_request.buffer_out, &m_message_queue.front(), sizeof(MessageData));
    m_message_queue.pop();
    GetEmulationKernel().EnqueueIPCReply(pending_request, IPC_SUCCESS);
    m_pending_request.reset();
  }
}
}  // namespace IOS::HLE
