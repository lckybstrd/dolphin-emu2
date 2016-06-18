#include <cinttypes>
#include <memory>
#include <mutex>
#include <random>
#include <string>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <CoreServices/CoreServices.h>
#endif

#include "Common/Analytics.h"
#include "Common/Common.h"
#include "Common/CommonTypes.h"
#include "Common/CPUDetect.h"
#include "Common/StringUtil.h"
#include "Core/Analytics.h"
#include "Core/ConfigManager.h"
#include "VideoCommon/VideoConfig.h"

namespace
{
const char* ANALYTICS_ENDPOINT = "https://analytics.dolphin-emu.org/report";
}  // namespace

std::mutex DolphinAnalytics::s_instance_mutex;
std::shared_ptr<DolphinAnalytics> DolphinAnalytics::s_instance;

DolphinAnalytics::DolphinAnalytics()
{
	ReloadConfig();
	MakeBaseBuilder();
}

std::shared_ptr<DolphinAnalytics> DolphinAnalytics::Instance()
{
	std::lock_guard<std::mutex> lk(s_instance_mutex);
	if (!s_instance)
	{
		s_instance.reset(new DolphinAnalytics());
	}
	return s_instance;
}

void DolphinAnalytics::ReloadConfig()
{
	std::lock_guard<std::mutex> lk(m_reporter_mutex);

	// Install the HTTP backend if analytics support is enabled.
	std::unique_ptr<Common::AnalyticsReportingBackend> new_backend;
	if (SConfig::GetInstance().m_analytics_enabled)
	{
		new_backend.reset(new Common::HttpAnalyticsBackend(ANALYTICS_ENDPOINT));
	}
	m_reporter.SetBackend(std::move(new_backend));

	// Load the unique ID or generate it if needed.
	m_unique_id = SConfig::GetInstance().m_analytics_id;
	if (m_unique_id.empty())
	{
		GenerateNewIdentity();
	}
}

void DolphinAnalytics::GenerateNewIdentity()
{
	std::random_device rd;
	u64 id = (static_cast<u64>(rd()) << 32) | rd();
	m_unique_id = StringFromFormat("%016" PRIx64, id);

	// Save the new id in the configuration.
	SConfig::GetInstance().m_analytics_id = m_unique_id;
	SConfig::GetInstance().SaveSettings();
}

void DolphinAnalytics::ReportDolphinStart(const std::string& ui_type)
{
	Common::AnalyticsReportBuilder builder(m_base_builder);
	builder.AddData("type", "dolphin-start");
	builder.AddData("ui-type", ui_type);
	Send(builder);
}

void DolphinAnalytics::ReportGameStart()
{
	MakePerGameBuilder();

	Common::AnalyticsReportBuilder builder(m_per_game_builder);
	builder.AddData("type", "game-start");
	Send(builder);
}

void DolphinAnalytics::MakeBaseBuilder()
{
	Common::AnalyticsReportBuilder builder;

	// Unique ID.
	builder.AddData("id", m_unique_id);

	// Version information.
	builder.AddData("version-desc", scm_desc_str);
	builder.AddData("version-hash", scm_rev_git_str);
	builder.AddData("version-branch", scm_branch_str);
	builder.AddData("version-dist", scm_distributor_str);

	// CPU information.
	builder.AddData("cpu-summary", cpu_info.Summarize());

	// OS information.
#if defined(_WIN32)
	builder.AddData("os-type", "windows");

	OSVERSIONEX winver;
	winver.dwOSVersionInfoSize = sizeof(winver);
	if (GetVersionEx(&winver))
	{
		builder.AddData("win-ver-major", winver.dwMajorVersion);
		builder.AddData("win-ver-minor", winver.dwMinorVersion);
		builder.AddData("win-ver-build", winver.dwBuildNumber);
		builder.AddData("win-ver-spmajor", winver.wServicePackMajor);
		builder.AddData("win-ver-spminor", winver.wServicePackMinor);
	}
#elif defined(ANDROID)
	builder.AddData("os-type", "android");
#elif defined(__APPLE__)
	builder.AddData("os-type", "osx");

	SInt32 osxmajor, osxminor, osxbugfix;
	Gestalt(gestaltSystemVersionMajor, &osxmajor);
	Gestalt(gestaltSystemVersionMinor, &osxminor);
	Gestalt(gestaltSystemVersionBugFix, &osxbugfix);

	builder.AddData("osx-ver-major", osxmajor);
	builder.AddData("osx-ver-minor", osxminor);
	builder.AddData("osx-ver-bugfix", osxbugfix);
#elif defined(__linux__)
	builder.AddData("os-type", "linux");
#elif defined(__FreeBSD__)
	builder.AddData("os-type", "freebsd");
#else
	builder.AddData("os-type", "unknown");
#endif

	m_base_builder = builder;
}

void DolphinAnalytics::MakePerGameBuilder()
{
	Common::AnalyticsReportBuilder builder(m_base_builder);

	// Gameid.
	builder.AddData("gameid", SConfig::GetInstance().GetUniqueID());

	// Configuration.
	builder.AddData("cfg-dsp-hle", SConfig::GetInstance().bDSPHLE);
	builder.AddData("cfg-dsp-jit", SConfig::GetInstance().m_DSPEnableJIT);
	builder.AddData("cfg-dsp-thread", SConfig::GetInstance().bDSPThread);
	builder.AddData("cfg-cpu-thread", SConfig::GetInstance().bCPUThread);
	builder.AddData("cfg-idle-skip", SConfig::GetInstance().bSkipIdle);
	builder.AddData("cfg-fastmem", SConfig::GetInstance().bFastmem);
	builder.AddData("cfg-syncgpu", SConfig::GetInstance().bSyncGPU);
	builder.AddData("cfg-audio-backend", SConfig::GetInstance().sBackend);
	builder.AddData("cfg-video-backend", SConfig::GetInstance().m_strVideoBackend);
	builder.AddData("cfg-oc-enable", SConfig::GetInstance().m_OCEnable);
	builder.AddData("cfg-oc-factor", SConfig::GetInstance().m_OCFactor);
	builder.AddData("cfg-render-to-main", SConfig::GetInstance().bRenderToMain);

	// Video configuration.
	builder.AddData("cfg-gfx-multisamples", g_Config.iMultisamples);
	builder.AddData("cfg-gfx-ssaa", g_Config.bSSAA);
	builder.AddData("cfg-gfx-efb-scale", g_Config.iEFBScale);
	builder.AddData("cfg-gfx-anisotropy", g_Config.iMaxAnisotropy);
	builder.AddData("cfg-gfx-realxfb", g_Config.RealXFBEnabled());
	builder.AddData("cfg-gfx-virtualxfb", g_Config.VirtualXFBEnabled());

	m_per_game_builder = builder;
}
