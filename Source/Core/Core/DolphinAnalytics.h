// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <mutex>
#include <string>
#include <string_view>

#include "Common/Analytics.h"

#include "Core/PerformanceSample.h"
#include "Core/PerformanceSampleAggregator.h"

#if defined(ANDROID)
#include <functional>
#endif
// Non generic part of the Dolphin Analytics framework. See Common/Analytics.h
// for the main documentation.

enum class GameQuirk
{
  // Sometimes code run from ICache is different from its mirror in RAM.
  ICacheMatters = 0,

  // The Wii remote hardware makes it possible to bypass normal data reporting and directly
  // "read" extension or IR data. This would break our current TAS/NetPlay implementation.
  DirectlyReadsWiimoteInput,

  // Several Wii DI commands that are rarely/never used and not implemented by Dolphin
  UsesDVDLowStopLaser,
  UsesDVDLowOffset,
  UsesDVDLowReadDiskBCA,  // NSMBW known to use this
  UsesDVDLowRequestDiscStatus,
  UsesDVDLowRequestRetryNumber,
  UsesDVDLowSerMeasControl,

  // Dolphin only implements the simple DVDLowOpenPartition, not any of the variants where some
  // already-read data is provided
  UsesDifferentPartitionCommand,

  // IOS has implementations for ioctls 0x85 and 0x89 and a stub for 0x87, but
  // DVDLowMaskCoverInterrupt/DVDLowUnmaskCoverInterrupt/DVDLowUnmaskStatusInterrupts
  // are all stubbed on the PPC side so they presumably will never be used.
  // (DVDLowClearCoverInterrupt is used, though)
  UsesDIInterruptMaskCommand,

  // Some games configure a mismatched number of texture coordinates or colors between the transform
  // and TEV/BP stages of the rendering pipeline. Currently, Dolphin just skips over these objects
  // as the hardware renderers are not equipped to handle the case where the registers between
  // stages are mismatched.
  MismatchedGPUTexgensBetweenXFAndBP,
  MismatchedGPUColorsBetweenXFAndBP,

  // The WD module can be configured to operate in six different modes.
  // In practice, only mode 1 (DS communications) and mode 3 (AOSS access point scanning)
  // are used by games and the system menu respectively.
  UsesUncommonWDMode,

  UsesWDUnimplementedIoctl,

  // Some games use invalid/unknown graphics commands (see e.g. bug 10931).
  // These are different from unknown opcodes: it is known that a BP/CP/XF command is being used,
  // but the command itself is not understood.
  UsesUnknownBPCommand,
  UsesUnknownCPCommand,
  UsesUnknownXFCommand,
  // YAGCD and Dolphin's implementation disagree about what is valid in some cases
  UsesMaybeInvalidCPCommand,
  // These commands are used by a few games (e.g. bug 12461), and seem to relate to perf queries.
  // Track them separately.
  UsesCPPerfCommand,

  Count,
};

class DolphinAnalytics
{
public:
  // Performs lazy-initialization of a singleton and returns the instance.
  static DolphinAnalytics& Instance();

#if defined(ANDROID)
  // Get value from java.
  static void AndroidSetGetValFunc(std::function<std::string(std::string)> function);
#endif
  // Resets and recreates the analytics system in order to reload
  // configuration.
  void ReloadConfig();
  // Rotates the unique identifier used for this instance of Dolphin and saves
  // it into the configuration.
  void GenerateNewIdentity();

  // Reports a Dolphin start event.
  void ReportDolphinStart(std::string_view ui_type);

  // Generates a base report for a "Game start" event. Also preseeds the
  // per-game base data.
  void ReportGameStart();

  // Generates a report for a special condition being hit by a game. This is automatically throttled
  // to once per game run.
  void ReportGameQuirk(GameQuirk quirk);

  // Reports performance information. This method performs its own throttling / aggregation --
  // calling it does not guarantee when a report will actually be sent.
  // Performance reports are generated using data from 100 consecutive frames.
  // Report starting times are randomized to obtain a wider range of sample data.
  // The first report begins 5-8 minutes after a game is launched.
  // Successive reports begin 30-33 minutes after the previous report finishes.
  //
  // This method is NOT thread-safe.
  void ReportPerformanceInfo(PerformanceSample&& sample);

  // Forward Send method calls to the reporter.
  template <typename T>
  void Send(T report)
  {
    std::lock_guard lk{m_reporter_mutex};
    m_reporter.Send(report);
  }

private:
  DolphinAnalytics();

  void MakeBaseBuilder();
  void MakePerGameBuilder();

  // Returns a unique ID derived on the global unique ID, hashed with some
  // report-specific data. This avoid correlation between different types of
  // events.
  std::string MakeUniqueId(std::string_view data) const;

  // Unique ID. This should never leave the application. Only used derived
  // values created by MakeUniqueId.
  std::string m_unique_id;

  PerformanceSampleAggregator m_sample_aggregator;

  // What quirks have already been reported about the current game.
  std::array<bool, static_cast<size_t>(GameQuirk::Count)> m_reported_quirks;

  // Builder that contains all non variable data that should be sent with all
  // reports.
  Common::AnalyticsReportBuilder m_base_builder;

  // Builder that contains per game data and is initialized when a game start
  // report is sent.
  Common::AnalyticsReportBuilder m_per_game_builder;

  std::mutex m_reporter_mutex;
  Common::AnalyticsReporter m_reporter;
};
