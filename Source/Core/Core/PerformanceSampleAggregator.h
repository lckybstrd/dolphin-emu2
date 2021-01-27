// Copyright 2021 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <chrono>

class PerformanceSampleAggregator
{
public:
  PerformanceSampleAggregator() = default;

  std::chrono::microseconds GetInitialSamplingStartTimestamp();
  std::chrono::microseconds GetRepeatSamplingStartTimestamp();
  static std::chrono::microseconds GetCurrentMicroseconds();
};
