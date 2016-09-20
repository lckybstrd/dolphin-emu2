// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <array>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <utility>
#include <wx/app.h>
#include <wx/buffer.h>
#include <wx/cmdline.h>
#include <wx/image.h>
#include <wx/imagpng.h>
#include <wx/intl.h>
#include <wx/language.h>
#include <wx/msgdlg.h>
#include <wx/thread.h>
#include <wx/timer.h>
#include <wx/utils.h>
#include <wx/window.h>

#include "Common/CPUDetect.h"
#include "Common/CommonPaths.h"
#include "Common/CommonTypes.h"
#include "Common/FileUtil.h"
#include "Common/IniFile.h"
#include "Common/Logging/LogManager.h"
#include "Common/Thread.h"

#include "Core/Analytics.h"
#include "Core/ConfigManager.h"
#include "Core/Core.h"
#include "Core/HW/Wiimote.h"
#include "Core/Host.h"
#include "Core/Movie.h"

#include "DolphinWX/Debugger/CodeWindow.h"
#include "DolphinWX/Debugger/JitWindow.h"
#include "DolphinWX/Frame.h"
#include "DolphinWX/Globals.h"
#include "DolphinWX/Main.h"
#include "DolphinWX/NetPlay/NetWindow.h"
#include "DolphinWX/SoftwareVideoConfigDialog.h"
#include "DolphinWX/VideoConfigDiag.h"
#include "DolphinWX/WxUtils.h"

#include "UICommon/UICommon.h"

#include "VideoCommon/VideoBackendBase.h"

#if defined HAVE_X11 && HAVE_X11
#include <X11/Xlib.h>
#endif

#ifdef __APPLE__
#import <AppKit/AppKit.h>
#endif

// ------------
//  Main window

IMPLEMENT_APP(DolphinApp)

bool wxMsgAlert(const char*, const char*, bool, int);
std::string wxStringTranslator(const char*);

CFrame* main_frame = nullptr;

bool DolphinApp::Initialize(int& c, wxChar** v)
{
#if defined HAVE_X11 && HAVE_X11
  XInitThreads();
#endif
  return wxApp::Initialize(c, v);
}

// The 'main program' equivalent that creates the main window and return the main frame

bool DolphinApp::OnInit()
{
  if (!wxApp::OnInit())
    return false;

  Bind(wxEVT_ACTIVATE_APP, &DolphinApp::OnApplicationFocusChanged, this);
  Bind(wxEVT_QUERY_END_SESSION, &DolphinApp::OnEndSession, this);
  Bind(wxEVT_END_SESSION, &DolphinApp::OnEndSession, this);
  Bind(wxEVT_IDLE, &DolphinApp::OnIdle, this);

  // Register message box and translation handlers
  RegisterMsgAlertHandler(&wxMsgAlert);
  RegisterStringTranslator(&wxStringTranslator);

#if wxUSE_ON_FATAL_EXCEPTION
  wxHandleFatalExceptions(true);
#endif

  UICommon::SetUserDirectory(m_user_path.ToStdString());
  UICommon::CreateDirectories();
  InitLanguageSupport();  // The language setting is loaded from the user directory
  UICommon::Init();

  if (m_select_video_backend && !m_video_backend_name.empty())
    SConfig::GetInstance().m_strVideoBackend = WxStrToStr(m_video_backend_name);

  if (m_select_audio_emulation)
    SConfig::GetInstance().bDSPHLE = (m_audio_emulation_name.Upper() == "HLE");

  VideoBackendBase::ActivateBackend(SConfig::GetInstance().m_strVideoBackend);

  DolphinAnalytics::Instance()->ReportDolphinStart("wx");

  // Enable the PNG image handler for screenshots
  wxImage::AddHandler(new wxPNGHandler);

  // We have to copy the size and position out of SConfig now because CFrame's OnMove
  // handler will corrupt them during window creation (various APIs like SetMenuBar cause
  // event dispatch including WM_MOVE/WM_SIZE)
  wxRect window_geometry(SConfig::GetInstance().iPosX, SConfig::GetInstance().iPosY,
                         SConfig::GetInstance().iWidth, SConfig::GetInstance().iHeight);
  main_frame = new CFrame(nullptr, wxID_ANY, StrToWxStr(scm_rev_str), window_geometry,
                          m_use_debugger, m_batch_mode, m_use_logger);
  SetTopWindow(main_frame);

  AfterInit();

  return true;
}

void DolphinApp::OnInitCmdLine(wxCmdLineParser& parser)
{
  static const wxCmdLineEntryDesc desc[] = {
      {wxCMD_LINE_SWITCH, "h", "help", "Show this help message", wxCMD_LINE_VAL_NONE,
       wxCMD_LINE_OPTION_HELP},
      {wxCMD_LINE_SWITCH, "d", "debugger", "Opens the debugger", wxCMD_LINE_VAL_NONE,
       wxCMD_LINE_PARAM_OPTIONAL},
      {wxCMD_LINE_SWITCH, "l", "logger", "Opens the logger", wxCMD_LINE_VAL_NONE,
       wxCMD_LINE_PARAM_OPTIONAL},
      {wxCMD_LINE_OPTION, "e", "exec",
       "Loads the specified file (ELF, DOL, GCM, ISO, WBFS, CISO, GCZ, WAD)", wxCMD_LINE_VAL_STRING,
       wxCMD_LINE_PARAM_OPTIONAL},
      {wxCMD_LINE_SWITCH, "b", "batch", "Exit Dolphin with emulator", wxCMD_LINE_VAL_NONE,
       wxCMD_LINE_PARAM_OPTIONAL},
      {wxCMD_LINE_OPTION, "c", "confirm", "Set Confirm on Stop", wxCMD_LINE_VAL_STRING,
       wxCMD_LINE_PARAM_OPTIONAL},
      {wxCMD_LINE_OPTION, "v", "video_backend", "Specify a video backend", wxCMD_LINE_VAL_STRING,
       wxCMD_LINE_PARAM_OPTIONAL},
      {wxCMD_LINE_OPTION, "a", "audio_emulation", "Low level (LLE) or high level (HLE) audio",
       wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL},
      {wxCMD_LINE_OPTION, "m", "movie", "Play a movie file", wxCMD_LINE_VAL_STRING,
       wxCMD_LINE_PARAM_OPTIONAL},
      {wxCMD_LINE_OPTION, "u", "user", "User folder path", wxCMD_LINE_VAL_STRING,
       wxCMD_LINE_PARAM_OPTIONAL},
      {wxCMD_LINE_NONE, nullptr, nullptr, nullptr, wxCMD_LINE_VAL_NONE, 0}};

  parser.SetDesc(desc);
}

bool DolphinApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
  if (argc == 2 && File::Exists(argv[1].ToUTF8().data()))
  {
    m_load_file = true;
    m_file_to_load = argv[1];
  }
  else if (parser.Parse() != 0)
  {
    return false;
  }

  if (!m_load_file)
    m_load_file = parser.Found("exec", &m_file_to_load);

  m_use_debugger = parser.Found("debugger");
  m_use_logger = parser.Found("logger");
  m_batch_mode = parser.Found("batch");
  m_confirm_stop = parser.Found("confirm", &m_confirm_setting);
  m_select_video_backend = parser.Found("video_backend", &m_video_backend_name);
  m_select_audio_emulation = parser.Found("audio_emulation", &m_audio_emulation_name);
  m_play_movie = parser.Found("movie", &m_movie_file);
  parser.Found("user", &m_user_path);

  return true;
}

#ifdef __APPLE__
void DolphinApp::MacOpenFile(const wxString& fileName)
{
  m_file_to_load = fileName;
  m_load_file = true;
  main_frame->BootGame(WxStrToStr(m_file_to_load));
}
#endif

void DolphinApp::AfterInit()
{
  if (!m_batch_mode)
    main_frame->UpdateGameList();

  if (!SConfig::GetInstance().m_analytics_permission_asked)
  {
    int answer =
        wxMessageBox(_("If authorized, Dolphin can collect data on its performance, "
                       "feature usage, and configuration, as well as data on your system's "
                       "hardware and operating system.\n\n"
                       "No private data is ever collected. This data helps us understand "
                       "how people and emulated games use Dolphin and prioritize our "
                       "efforts. It also helps us identify rare configurations that are "
                       "causing bugs, performance and stability issues.\n"
                       "This authorization can be revoked at any time through Dolphin's "
                       "settings.\n\n"
                       "Do you authorize Dolphin to report this information to Dolphin's "
                       "developers?"),
                     _("Usage statistics reporting"), wxYES_NO, main_frame);

    SConfig::GetInstance().m_analytics_permission_asked = true;
    SConfig::GetInstance().m_analytics_enabled = (answer == wxYES);
    SConfig::GetInstance().SaveSettings();

    DolphinAnalytics::Instance()->ReloadConfig();
  }

  if (m_confirm_stop)
  {
    if (m_confirm_setting.Upper() == "TRUE")
      SConfig::GetInstance().bConfirmStop = true;
    else if (m_confirm_setting.Upper() == "FALSE")
      SConfig::GetInstance().bConfirmStop = false;
  }

  if (m_play_movie && !m_movie_file.empty())
  {
    if (Movie::PlayInput(WxStrToStr(m_movie_file)))
    {
      if (m_load_file && !m_file_to_load.empty())
      {
        main_frame->BootGame(WxStrToStr(m_file_to_load));
      }
      else
      {
        main_frame->BootGame("");
      }
    }
  }
  // First check if we have an exec command line.
  else if (m_load_file && !m_file_to_load.empty())
  {
    main_frame->BootGame(WxStrToStr(m_file_to_load));
  }
  // If we have selected Automatic Start, start the default ISO,
  // or if no default ISO exists, start the last loaded ISO
  else if (main_frame->g_pCodeWindow)
  {
    if (main_frame->g_pCodeWindow->AutomaticStart())
    {
      main_frame->BootGame("");
    }
  }
}

void DolphinApp::InitLanguageSupport()
{
  unsigned int language = 0;

  IniFile ini;
  ini.Load(File::GetUserPath(F_DOLPHINCONFIG_IDX));
  ini.GetOrCreateSection("Interface")->Get("Language", &language, wxLANGUAGE_DEFAULT);

  // Load language if possible, fall back to system default otherwise
  if (wxLocale::IsAvailable(language))
  {
    m_locale.reset(new wxLocale(language));

// Specify where dolphins *.gmo files are located on each operating system
#ifdef _WIN32
    m_locale->AddCatalogLookupPathPrefix(StrToWxStr(File::GetExeDirectory() + DIR_SEP "Languages"));
#elif defined(__LINUX__)
    m_locale->AddCatalogLookupPathPrefix(StrToWxStr(DATA_DIR "../locale"));
#elif defined(__APPLE__)
    m_locale->AddCatalogLookupPathPrefix(
        StrToWxStr(File::GetBundleDirectory() + "Contents/Resources"));
#endif

    m_locale->AddCatalog("dolphin-emu");

    if (!m_locale->IsOk())
    {
      wxMessageBox(_("Error loading selected language. Falling back to system default."),
                   _("Error"));
      m_locale.reset(new wxLocale(wxLANGUAGE_DEFAULT));
    }
  }
  else
  {
    wxMessageBox(
        _("The selected language is not supported by your system. Falling back to system default."),
        _("Error"));
    m_locale.reset(new wxLocale(wxLANGUAGE_DEFAULT));
  }
}

void DolphinApp::OnApplicationFocusChanged(wxActivateEvent& ev)
{
  m_is_active_threadsafe.store(ev.GetActive(), std::memory_order_relaxed);
  ev.Skip();
}

void DolphinApp::OnEndSession(wxCloseEvent& event)
{
  // Close if we've received wxEVT_END_SESSION (ignore wxEVT_QUERY_END_SESSION)
  if (!event.CanVeto())
  {
    main_frame->Close(true);
  }
}

int DolphinApp::OnExit()
{
  Core::Shutdown();
  UICommon::Shutdown();

  return wxApp::OnExit();
}

void DolphinApp::OnFatalException()
{
  WiimoteReal::Shutdown();
}

void DolphinApp::OnIdle(wxIdleEvent& ev)
{
  ev.Skip();
  Core::HostDispatchJobs();
}

// ------------
// Talk to GUI

std::string wxStringTranslator(const char* text)
{
  return WxStrToStr(wxGetTranslation(wxString::FromUTF8(text)));
}

// Accessor for the main window class
CFrame* DolphinApp::GetCFrame()
{
  return main_frame;
}

bool DolphinApp::IsActiveThreadsafe() const
{
  return m_is_active_threadsafe.load(std::memory_order_relaxed);
}

void* Host_GetRenderHandle()
{
  return main_frame->GetRenderHandle();
}

// Posting messages across thread boundaries has several caveats:
//   - Use QueueEvent, only wxThreadEvent can be used with AddPendingEvent. Never use
//     ProcessEvent.
//   - GUI objects can only be created on the main thread.
//   - GUI objects can only have member functions called while holding the GUI lock
//     (wxMutexGuiEnter) but if the object tries to create a window behind the scenes
//     then it will break anyway even with the lock held.
//   - Many wx objects, including wxString, use non-threadsafe reference counting
//     implementations which will data race when sent across thread boundaries. These
//     races are hard to avoid since the destructor on the stack local object will set
//     it off. You must always be mindful of object scope and make sure local objects
//     that are included in the message are destructed BEFORE the QueueEvent() call.
//     (UnShare() / Clone() / etc needs to be used on objects that came from somewhere
//     else since assigning copies just twiddles the refcount)
static void PostHostMessage(wxEvtHandler* target, int id, int val = 0,
                            const wxString& str = wxEmptyString)
{
  wxThreadEvent* event = new wxThreadEvent(wxEVT_HOST_COMMAND, id);
  event->SetInt(val);
  event->SetString(str.Clone());  // DEEP copy string (instead of incrementing refcount)
  // NOTE: Local stack copy of cloned string is destroyed on this line. assert(refcount==1)
  target->QueueEvent(event);  // Takes ownership of the object
}
static void PostHostMessage(int id, int val = 0, const wxString& str = wxEmptyString)
{
  PostHostMessage(main_frame->GetEventHandler(), id, val, str);
}
static void PostDebugHostMessage(int id, int val = 0, const wxString& str = wxEmptyString)
{
  PostHostMessage(id, val, str);
  if (main_frame->g_pCodeWindow)
    PostHostMessage(main_frame->g_pCodeWindow->GetEventHandler(), id, val, str);
}

bool wxMsgAlert(const char* caption, const char* text, bool yes_no, int /*Style*/)
{
  // Rules:
  //  - On Windows, any thread can create a window. That window then belongs to that
  //    thread and it must run the message pump for it (window messages are sent to
  //    the owner thread's queue). This also means that modal windows on Thread B will
  //    not prevent windows on Thread A from continuing to process events.
  //      - Caveat: If a window created on Thread B has a parent that is owned by Thread A
  //        then that causes both threads to AttachInputQueue(...) with each other. They
  //        share a single internal input queue which means that if either thread stops
  //        running its message pump then both threads deadlock since the queue will refuse
  //        to give messages while the head of the queue is intended for the other thread.
  //        This problem scales exponentially as you can attach as many threads as you want
  //        which will all collectively deadlock with each other as soon as one fails.
  //        [This is a pain to deal with and very dangerous so it should be avoided]
  //  - On GTK, only the main thread can create windows or receive messages
  //  - On Mac OS X, only the main thread can create windows or receive messages
  NetPlayDialog* npd = NetPlayDialog::GetInstance();
  if (npd && npd->IsShown())
  {
    // FIXME: This is a race because NetPlay uses a FifoQueue to transfer messages from the client
    // thread to the GUI. FifoQueue is only dual-thread safe, not N-threads safe.
    npd->AppendChat("/!\\ CORE PANIC: " + std::string{text});
    return true;
  }
  return main_frame->CreatePanicWindowAndWait(text, caption, yes_no);
}

void Host_Message(int id)
{
  if (id == WM_USER_JOB_DISPATCH)
  {
    // Trigger a wxEVT_IDLE
    wxWakeUpIdle();
    return;
  }
  PostHostMessage(id);
}

void Host_NotifyMapLoaded()
{
  PostDebugHostMessage(IDM_NOTIFY_MAP_LOADED);
}

void Host_UpdateDisasmDialog()
{
  PostDebugHostMessage(IDM_UPDATE_DISASM_DIALOG);
}

void Host_UpdateMainFrame()
{
  PostDebugHostMessage(IDM_UPDATE_GUI);
}

void Host_RefreshDSPDebuggerWindow()
{
  PostDebugHostMessage(IDM_UPDATE_DSP_DEBUGGER);
}

void Host_UpdateTitle(const std::string& title)
{
  PostHostMessage(IDM_UPDATE_TITLE, 0, StrToWxStr(title));
}

void Host_RequestRenderWindowSize(int width, int height)
{
  wxThreadEvent* ev = new wxThreadEvent(wxEVT_HOST_COMMAND, IDM_WINDOW_SIZE_REQUEST);
  ev->SetPayload(std::pair<int, int>(width, height));
  main_frame->GetEventHandler()->QueueEvent(ev);
}

void Host_RequestFullscreen(bool enable_fullscreen)
{
  PostHostMessage(IDM_FULLSCREEN_REQUEST, enable_fullscreen);
}

void Host_SetStartupDebuggingParameters()
{
  SConfig& StartUp = SConfig::GetInstance();
  if (main_frame->g_pCodeWindow)
  {
    StartUp.bBootToPause = main_frame->g_pCodeWindow->BootToPause();
    StartUp.bAutomaticStart = main_frame->g_pCodeWindow->AutomaticStart();
    StartUp.bJITNoBlockCache = main_frame->g_pCodeWindow->JITNoBlockCache();
    StartUp.bJITNoBlockLinking = main_frame->g_pCodeWindow->JITNoBlockLinking();
  }
  else
  {
    StartUp.bBootToPause = false;
  }
  StartUp.bEnableDebugging = main_frame->g_pCodeWindow ? true : false;  // RUNNING_DEBUG
}

void Host_SetWiiMoteConnectionState(int state)
{
  static std::atomic<int> old_state(-1);
  if (old_state.exchange(state) == state)
    return;

  static constexpr std::array<const char* const, 3> s_message_list{
      {wxTRANSLATE("Not connected"), wxTRANSLATE("Connecting..."),
       wxTRANSLATE("Wiimote Connected")}};
  const char* message = "Unknown State";  // Doesn't need to be translated
  if (static_cast<u32>(state) < s_message_list.size())
    message = s_message_list[state];

  NOTICE_LOG(WIIMOTE, "%s", message);

  // Update field 0 or 1
  int field = 1;

  PostHostMessage(IDM_UPDATE_STATUS_BAR, field, wxGetTranslation(message));
}

// This is called from the UI and CPU Threads
bool Host_UIHasFocus()
{
  return wxGetApp().IsActiveThreadsafe();
}

// This is called from the UI, CPU and GPU threads
bool Host_RendererHasFocus()
{
  return main_frame->RendererHasFocus();
}

// This is not called from anywhere
bool Host_RendererIsFullscreen()
{
  return main_frame->RendererIsFullscreen();
}

void Host_ConnectWiimote(int wm_idx, bool connect)
{
  int id_base = connect ? IDM_FORCE_CONNECT_WIIMOTE1 : IDM_FORCE_DISCONNECT_WIIMOTE1;
  PostHostMessage(id_base + wm_idx);
}

void Host_ShowVideoConfig(void* parent, const std::string& backend_name)
{
  wxASSERT(wxIsMainThread());

  if (backend_name == "Software Renderer")
  {
    SoftwareVideoConfigDialog diag(static_cast<wxWindow*>(parent), backend_name);
    diag.ShowModal();
  }
  else
  {
    VideoConfigDiag diag(static_cast<wxWindow*>(parent), backend_name);
    diag.ShowModal();
  }
}
