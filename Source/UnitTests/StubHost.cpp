// Copyright 2014 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// Stub implementation of the Host_* callbacks for tests. These implementations
// do nothing except return default values when required.

#include <string>
#include <vector>

#include "Core/Host.h"

std::vector<std::string> Host_GetPreferredLocales()
{
  return {};
}
void Host_NotifyMapLoaded()
{
}
void Host_RefreshDSPDebuggerWindow()
{
}
void Host_Message(HostMessageID)
{
}
void Host_UpdateTitle(const std::string&)
{
}
void Host_UpdateDisasmDialog()
{
}
void Host_UpdateMainFrame()
{
}
void Host_RequestRenderWindowSize(int, int)
{
}
bool Host_UIBlocksControllerState()
{
  return false;
}
bool Host_RendererHasFocus()
{
  return false;
}
bool Host_RendererHasFullFocus()
{
  return false;
}
bool Host_RendererIsFullscreen()
{
  return false;
}
bool Host_MainWindowHasFocus()
{
  return false;
}
void Host_YieldToUI()
{
}
void Host_TitleChanged()
{
}
std::unique_ptr<GBAHostInterface> Host_CreateGBAHost(std::weak_ptr<HW::GBA::Core> core)
{
  return nullptr;
}
