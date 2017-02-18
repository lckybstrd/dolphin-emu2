// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <cstddef>

// clang-format off
#include <wx/bitmap.h>
#include <wx/panel.h>
// clang-format on

#include "Common/FileUtil.h"
#include "Common/Config.h"

#include "Core/ConfigManager.h"
#include "Core/ConfigLoaders/GameConfigLoader.h"
#include "Core/PowerPC/PowerPC.h"

#include "DolphinWX/Debugger/WatchView.h"
#include "DolphinWX/Debugger/WatchWindow.h"
#include "DolphinWX/AuiToolBar.h"
#include "DolphinWX/WxUtils.h"

class CWatchToolbar : public DolphinAuiToolBar
{
public:
  CWatchToolbar(CWatchWindow* parent, const wxWindowID id)
      : DolphinAuiToolBar(parent, id, wxDefaultPosition, wxDefaultSize,
                          wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_TEXT)
  {
    wxSize bitmap_size = FromDIP(wxSize(16, 16));
    SetToolBitmapSize(bitmap_size);

    m_Bitmaps[Toolbar_File] = WxUtils::LoadScaledResourceBitmap(
        "toolbar_debugger_delete", this, bitmap_size, wxDefaultSize,
        WxUtils::LSI_SCALE_DOWN | WxUtils::LSI_ALIGN_CENTER);

    AddTool(ID_LOAD, _("Load"), m_Bitmaps[Toolbar_File]);
    Bind(wxEVT_TOOL, &CWatchWindow::Event_LoadAll, parent, ID_LOAD);

    AddTool(ID_SAVE, _("Save"), m_Bitmaps[Toolbar_File]);
    Bind(wxEVT_TOOL, &CWatchWindow::Event_SaveAll, parent, ID_SAVE);
  }

private:
  enum
  {
    Toolbar_File,
    Num_Bitmaps
  };

  enum
  {
    ID_LOAD,
    ID_SAVE
  };

  wxBitmap m_Bitmaps[Num_Bitmaps];
};

CWatchWindow::CWatchWindow(wxWindow* parent, wxWindowID id, const wxPoint& position,
                           const wxSize& size, long style, const wxString& name)
    : wxPanel(parent, id, position, size, style, name), m_GPRGridView(nullptr)
{
  m_mgr.SetManagedWindow(this);
  m_mgr.SetFlags(wxAUI_MGR_DEFAULT | wxAUI_MGR_LIVE_RESIZE);

  m_GPRGridView = new CWatchView(this);

  m_mgr.AddPane(new CWatchToolbar(this, wxID_ANY), wxAuiPaneInfo()
                                                       .ToolbarPane()
                                                       .Top()
                                                       .LeftDockable(true)
                                                       .RightDockable(true)
                                                       .BottomDockable(false)
                                                       .Floatable(false));
  m_mgr.AddPane(m_GPRGridView, wxAuiPaneInfo().CenterPane());
  m_mgr.Update();
}

CWatchWindow::~CWatchWindow()
{
  m_mgr.UnInit();
}

void CWatchWindow::NotifyUpdate()
{
  if (m_GPRGridView != nullptr)
    m_GPRGridView->Repopulate();
}

void CWatchWindow::Event_SaveAll(wxCommandEvent& WXUNUSED(event))
{
  SaveAll();
}

void CWatchWindow::SaveAll()
{
  auto game_layer = std::make_unique<Config::Layer>(std::unique_ptr<Config::ConfigLayerLoader>(
      GenerateLocalGameConfigLoader(SConfig::GetInstance().GetGameID(), 0)));

  Config::Section* watches = game_layer->GetOrCreateSection(Config::System::Debugger, "Watches");

  watches->SetLines(PowerPC::watches.GetStrings());
  game_layer->Save();
}

void CWatchWindow::Event_LoadAll(wxCommandEvent& WXUNUSED(event))
{
  LoadAll();
}

void CWatchWindow::LoadAll()
{
  Watches::TWatchesStr watches;
  Config::Section* watches_config = Config::GetOrCreateSection(Config::System::Debugger, "Watches");

  if (watches_config->GetLines(&watches, false))
  {
    PowerPC::watches.Clear();
    PowerPC::watches.AddFromStrings(watches);
  }

  NotifyUpdate();
}
