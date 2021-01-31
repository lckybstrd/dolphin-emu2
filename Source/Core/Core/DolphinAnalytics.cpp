#include "Core/DolphinAnalytics.h"

#include <array>
#include <memory>
#include <mutex>
#include <string>

#include <fmt/format.h>
#include <mbedtls/sha1.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <objc/message.h>
#elif defined(ANDROID)
#include "Common/AndroidAnalytics.h"
#endif

#include "Common/Analytics.h"
#include "Common/CPUDetect.h"
#include "Common/CommonTypes.h"
#include "Common/Config/Config.h"
#include "Common/Random.h"
#include "Common/Version.h"

#include "Core/Config/MainSettings.h"
#include "Core/ConfigManager.h"
#include "Core/HW/GCPad.h"
#include "Core/Movie.h"
#include "Core/NetPlayProto.h"

#include "InputCommon/GCAdapter.h"
#include "InputCommon/InputConfig.h"

#include "VideoCommon/VideoBackendBase.h"
#include "VideoCommon/VideoConfig.h"

#if defined(ANDROID)
static std::function<std::string(std::string)> s_get_val_func;
void DolphinAnalytics::AndroidSetGetValFunc(std::function<std::string(std::string)> func)
{
  s_get_val_func = std::move(func);
}
#endif

namespace
{
void AddVersionInformationToReportBuilder(Common::AnalyticsReportBuilder* builder)
{
  builder->AddData("version-desc", Common::scm_desc_str);
  builder->AddData("version-hash", Common::scm_rev_git_str);
  builder->AddData("version-branch", Common::scm_branch_str);
  builder->AddData("version-dist", Common::scm_distributor_str);
}

void AddAutoUpdateInformationToReportBuilder(Common::AnalyticsReportBuilder* builder)
{
  builder->AddData("update-track", SConfig::GetInstance().m_auto_update_track);
}

void AddCPUInformationToReportBuilder(Common::AnalyticsReportBuilder* builder)
{
  builder->AddData("cpu-summary", cpu_info.Summarize());
}

#if defined(_WIN32)
void AddWindowsInformationToReportBuilder(Common::AnalyticsReportBuilder* builder)
{
  // Windows 8 removes support for GetVersionEx and such. Stupid.
  const DWORD(WINAPI * RtlGetVersion)(LPOSVERSIONINFOEXW);
  *(FARPROC*)&RtlGetVersion = GetProcAddress(GetModuleHandle(TEXT("ntdll")), "RtlGetVersion");

  OSVERSIONINFOEXW winver;
  winver.dwOSVersionInfoSize = sizeof(winver);
  if (RtlGetVersion != nullptr)
  {
    RtlGetVersion(&winver);
    builder->AddData("win-ver-major", static_cast<u32>(winver.dwMajorVersion));
    builder->AddData("win-ver-minor", static_cast<u32>(winver.dwMinorVersion));
    builder->AddData("win-ver-build", static_cast<u32>(winver.dwBuildNumber));
    builder->AddData("win-ver-spmajor", static_cast<u32>(winver.wServicePackMajor));
    builder->AddData("win-ver-spminor", static_cast<u32>(winver.wServicePackMinor));
  }
}
#elif defined(ANDROID)
void AddAndroidInformationToReportBuilder(Common::AnalyticsReportBuilder* builder)
{
  builder->AddData("android-manufacturer", s_get_val_func("DEVICE_MANUFACTURER"));
  builder->AddData("android-model", s_get_val_func("DEVICE_MODEL"));
  builder->AddData("android-version", s_get_val_func("DEVICE_OS"));
}
#elif defined(__APPLE__)
void AddMacOSInformationToReportBuilder(Common::AnalyticsReportBuilder* builder)
{
  // id processInfo = [NSProcessInfo processInfo]
  id processInfo = reinterpret_cast<id (*)(Class, SEL)>(objc_msgSend)(
      objc_getClass("NSProcessInfo"), sel_getUid("processInfo"));
  if (processInfo)
  {
    struct OSVersion  // NSOperatingSystemVersion
    {
      s64 major_version;  // NSInteger majorVersion
      s64 minor_version;  // NSInteger minorVersion
      s64 patch_version;  // NSInteger patchVersion
    };

    // NSOperatingSystemVersion version = [processInfo operatingSystemVersion]
    OSVersion version = reinterpret_cast<OSVersion (*)(id, SEL)>(objc_msgSend_stret)(
        processInfo, sel_getUid("operatingSystemVersion"));

    builder->AddData("osx-ver-major", version.major_version);
    builder->AddData("osx-ver-minor", version.minor_version);
    builder->AddData("osx-ver-bugfix", version.patch_version);
  }
}
#endif

void AddPlatformInformationToReportBuilder(Common::AnalyticsReportBuilder* builder)
{
#if defined(_WIN32)
  builder->AddData("os-type", "windows");
  AddWindowsInformationToReportBuilder(builder);
#elif defined(ANDROID)
  builder->AddData("os-type", "android");
  AddAndroidInformationToReportBuilder(builder);
#elif defined(__APPLE__)
  builder->AddData("os-type", "osx");
  AddMacOSInformationToReportBuilder(builder);
#elif defined(__linux__)
  builder->AddData("os-type", "linux");
#elif defined(__FreeBSD__)
  builder->AddData("os-type", "freebsd");
#elif defined(__OpenBSD__)
  builder->AddData("os-type", "openbsd");
#elif defined(__NetBSD__)
  builder->AddData("os-type", "netbsd");
#elif defined(__HAIKU__)
  builder->AddData("os-type", "haiku");
#else
  builder->AddData("os-type", "unknown");
#endif
}
}  // namespace

DolphinAnalytics::DolphinAnalytics()
{
  ReloadConfig();
  MakeBaseBuilder();
}

DolphinAnalytics& DolphinAnalytics::Instance()
{
  static DolphinAnalytics instance;
  return instance;
}

void DolphinAnalytics::ReloadConfig()
{
  const std::lock_guard lk{m_reporter_mutex};

  // Install the HTTP backend if analytics support is enabled.
  std::unique_ptr<Common::AnalyticsReportingBackend> new_backend;
  if (Config::Get(Config::MAIN_ANALYTICS_ENABLED))
  {
    constexpr char analytics_endpoint[] = "https://analytics.dolphin-emu.org/report";
#if defined(ANDROID)
    new_backend = std::make_unique<Common::AndroidAnalyticsBackend>(analytics_endpoint);
#else
    new_backend = std::make_unique<Common::HttpAnalyticsBackend>(analytics_endpoint);
#endif
  }
  m_reporter.SetBackend(std::move(new_backend));

  // Load the unique ID or generate it if needed.
  m_unique_id = Config::Get(Config::MAIN_ANALYTICS_ID);
  if (m_unique_id.empty())
  {
    GenerateNewIdentity();
  }
}

void DolphinAnalytics::GenerateNewIdentity()
{
  const u64 id_high = Common::Random::GenerateValue<u64>();
  const u64 id_low = Common::Random::GenerateValue<u64>();
  m_unique_id = fmt::format("{:016x}{:016x}", id_high, id_low);

  // Save the new id in the configuration.
  Config::SetBase(Config::MAIN_ANALYTICS_ID, m_unique_id);
  Config::Save();
}

std::string DolphinAnalytics::MakeUniqueId(std::string_view data) const
{
  std::array<u8, 20> digest;
  const auto input = std::string{m_unique_id}.append(data);
  mbedtls_sha1_ret(reinterpret_cast<const u8*>(input.c_str()), input.size(), digest.data());

  // Convert to hex string and truncate to 64 bits.
  std::string out;
  for (int i = 0; i < 8; ++i)
  {
    out += fmt::format("{:02x}", digest[i]);
  }
  return out;
}

void DolphinAnalytics::ReportDolphinStart(std::string_view ui_type)
{
  Common::AnalyticsReportBuilder builder(m_base_builder);
  builder.AddData("type", "dolphin-start");
  builder.AddData("ui-type", ui_type);
  builder.AddData("id", MakeUniqueId("dolphin-start"));
  Send(builder);
}

void DolphinAnalytics::ReportGameStart()
{
  MakePerGameBuilder();

  Common::AnalyticsReportBuilder builder(m_per_game_builder);
  builder.AddData("type", "game-start");
  Send(builder);

  // Reset per-game state.
  m_reported_quirks.fill(false);
  m_sample_aggregator.InitializePerformanceSampling();
}

// Keep in sync with enum class GameQuirk definition.
constexpr std::array<const char*, 19> GAME_QUIRKS_NAMES{
    "icache-matters",
    "directly-reads-wiimote-input",
    "uses-DVDLowStopLaser",
    "uses-DVDLowOffset",
    "uses-DVDLowReadDiskBca",
    "uses-DVDLowRequestDiscStatus",
    "uses-DVDLowRequestRetryNumber",
    "uses-DVDLowSerMeasControl",
    "uses-different-partition-command",
    "uses-di-interrupt-command",
    "mismatched-gpu-texgens-between-xf-and-bp",
    "mismatched-gpu-colors-between-xf-and-bp",
    "uses-uncommon-wd-mode",
    "uses-wd-unimplemented-ioctl",
    "uses-unknown-bp-command",
    "uses-unknown-cp-command",
    "uses-unknown-xf-command",
    "uses-maybe-invalid-cp-command",
    "uses-cp-perf-command",
};
static_assert(GAME_QUIRKS_NAMES.size() == static_cast<u32>(GameQuirk::Count),
              "Game quirks names and enum definition are out of sync.");

void DolphinAnalytics::ReportGameQuirk(GameQuirk quirk)
{
  const u32 quirk_idx = static_cast<u32>(quirk);

  // Only report once per run.
  if (m_reported_quirks[quirk_idx])
    return;
  m_reported_quirks[quirk_idx] = true;

  Common::AnalyticsReportBuilder builder(m_per_game_builder);
  builder.AddData("type", "quirk");
  builder.AddData("quirk", GAME_QUIRKS_NAMES[quirk_idx]);
  Send(builder);
}

void DolphinAnalytics::ReportPerformanceInfo(PerformanceSample&& sample)
{
  m_sample_aggregator.AddSampleIfSamplingInProgress(std::move(sample));
  const std::optional<PerformanceSampleAggregator::CompletedReport> report_optional =
      m_sample_aggregator.PopReportIfComplete();
  if (!report_optional)
  {
    return;
  }
  const PerformanceSampleAggregator::CompletedReport& report = *report_optional;

  // The per game builder should already exist -- there is no way we can be reporting performance
  // info without a game start event having been generated.
  Common::AnalyticsReportBuilder builder(m_per_game_builder);
  builder.AddData("type", "performance");
  builder.AddData("speed", report.speed);
  builder.AddData("prims", report.primitives);
  builder.AddData("draw-calls", report.draw_calls);

  Send(builder);
}

void DolphinAnalytics::MakeBaseBuilder()
{
  m_base_builder = Common::AnalyticsReportBuilder();

  AddVersionInformationToReportBuilder(&m_base_builder);
  AddAutoUpdateInformationToReportBuilder(&m_base_builder);
  AddCPUInformationToReportBuilder(&m_base_builder);
  AddPlatformInformationToReportBuilder(&m_base_builder);
}

static const char* GetShaderCompilationMode(const VideoConfig& video_config)
{
  switch (video_config.iShaderCompilationMode)
  {
  case ShaderCompilationMode::AsynchronousUberShaders:
    return "async-ubershaders";
  case ShaderCompilationMode::AsynchronousSkipRendering:
    return "async-skip-rendering";
  case ShaderCompilationMode::SynchronousUberShaders:
    return "sync-ubershaders";
  case ShaderCompilationMode::Synchronous:
  default:
    return "sync";
  }
}

void DolphinAnalytics::MakePerGameBuilder()
{
  Common::AnalyticsReportBuilder builder(m_base_builder);
  const SConfig& config = SConfig::GetInstance();

  // Gameid.
  builder.AddData("gameid", config.GetGameID());

  // Unique id bound to the gameid.
  builder.AddData("id", MakeUniqueId(config.GetGameID()));

  // Configuration.
  builder.AddData("cfg-dsp-hle", config.bDSPHLE);
  builder.AddData("cfg-dsp-jit", config.m_DSPEnableJIT);
  builder.AddData("cfg-dsp-thread", config.bDSPThread);
  builder.AddData("cfg-cpu-thread", config.bCPUThread);
  builder.AddData("cfg-fastmem", config.bFastmem);
  builder.AddData("cfg-syncgpu", config.bSyncGPU);
  builder.AddData("cfg-audio-backend", config.sBackend);
  builder.AddData("cfg-oc-enable", config.m_OCEnable);
  builder.AddData("cfg-oc-factor", config.m_OCFactor);
  builder.AddData("cfg-render-to-main", Config::Get(Config::MAIN_RENDER_TO_MAIN));
  if (g_video_backend)
  {
    builder.AddData("cfg-video-backend", g_video_backend->GetName());
  }

  // Video configuration.
  builder.AddData("cfg-gfx-multisamples", g_Config.iMultisamples);
  builder.AddData("cfg-gfx-ssaa", g_Config.bSSAA);
  builder.AddData("cfg-gfx-anisotropy", g_Config.iMaxAnisotropy);
  builder.AddData("cfg-gfx-vsync", g_Config.bVSync);
  builder.AddData("cfg-gfx-aspect-ratio", static_cast<int>(g_Config.aspect_mode));
  builder.AddData("cfg-gfx-efb-access", g_Config.bEFBAccessEnable);
  builder.AddData("cfg-gfx-efb-copy-format-changes", g_Config.bEFBEmulateFormatChanges);
  builder.AddData("cfg-gfx-efb-copy-ram", !g_Config.bSkipEFBCopyToRam);
  builder.AddData("cfg-gfx-xfb-copy-ram", !g_Config.bSkipXFBCopyToRam);
  builder.AddData("cfg-gfx-defer-efb-copies", g_Config.bDeferEFBCopies);
  builder.AddData("cfg-gfx-immediate-xfb", !g_Config.bImmediateXFB);
  builder.AddData("cfg-gfx-efb-copy-scaled", g_Config.bCopyEFBScaled);
  builder.AddData("cfg-gfx-internal-resolution", g_Config.iEFBScale);
  builder.AddData("cfg-gfx-tc-samples", g_Config.iSafeTextureCache_ColorSamples);
  builder.AddData("cfg-gfx-stereo-mode", static_cast<int>(g_Config.stereo_mode));
  builder.AddData("cfg-gfx-per-pixel-lighting", g_Config.bEnablePixelLighting);
  builder.AddData("cfg-gfx-shader-compilation-mode", GetShaderCompilationMode(g_Config));
  builder.AddData("cfg-gfx-wait-for-shaders", g_Config.bWaitForShadersBeforeStarting);
  builder.AddData("cfg-gfx-fast-depth", g_Config.bFastDepthCalc);
  builder.AddData("cfg-gfx-vertex-rounding", g_Config.UseVertexRounding());

  const auto& video_backend = g_Config.backend_info;

  // GPU features.
  if (g_Config.iAdapter < static_cast<int>(video_backend.Adapters.size()))
  {
    builder.AddData("gpu-adapter", video_backend.Adapters[g_Config.iAdapter]);
  }
  else if (!video_backend.AdapterName.empty())
  {
    builder.AddData("gpu-adapter", video_backend.AdapterName);
  }
  builder.AddData("gpu-has-exclusive-fullscreen", video_backend.bSupportsExclusiveFullscreen);
  builder.AddData("gpu-has-dual-source-blend", video_backend.bSupportsDualSourceBlend);
  builder.AddData("gpu-has-primitive-restart", video_backend.bSupportsPrimitiveRestart);
  builder.AddData("gpu-has-oversized-viewports", video_backend.bSupportsOversizedViewports);
  builder.AddData("gpu-has-geometry-shaders", video_backend.bSupportsGeometryShaders);
  builder.AddData("gpu-has-3d-vision", video_backend.bSupports3DVision);
  builder.AddData("gpu-has-early-z", video_backend.bSupportsEarlyZ);
  builder.AddData("gpu-has-binding-layout", video_backend.bSupportsBindingLayout);
  builder.AddData("gpu-has-bbox", video_backend.bSupportsBBox);
  builder.AddData("gpu-has-fragment-stores-and-atomics",
                  video_backend.bSupportsFragmentStoresAndAtomics);
  builder.AddData("gpu-has-gs-instancing", video_backend.bSupportsGSInstancing);
  builder.AddData("gpu-has-post-processing", video_backend.bSupportsPostProcessing);
  builder.AddData("gpu-has-palette-conversion", video_backend.bSupportsPaletteConversion);
  builder.AddData("gpu-has-clip-control", video_backend.bSupportsClipControl);
  builder.AddData("gpu-has-ssaa", video_backend.bSupportsSSAA);

  // NetPlay / recording.
  builder.AddData("netplay", NetPlay::IsNetPlayRunning());
  builder.AddData("movie", Movie::IsMovieActive());

  // Controller information
  // We grab enough to tell what percentage of our users are playing with keyboard/mouse, some kind
  // of gamepad, or the official gamecube adapter.
  builder.AddData("gcadapter-detected", GCAdapter::IsDetected(nullptr));
  builder.AddData("has-controller", Pad::GetConfig()->IsControllerControlledByGamepadDevice(0) ||
                                        GCAdapter::IsDetected(nullptr));

  m_per_game_builder = builder;
}
