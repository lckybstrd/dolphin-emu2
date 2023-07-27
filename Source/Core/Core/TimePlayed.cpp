#pragma once

#include <string>

#include "Common/FileUtil.h"
#include "Common/IniFile.h"
#include "TimePlayed.h"

TimePlayed::TimePlayed(std::string game_id)
{
  m_game_id = game_id;
}

void TimePlayed::AddTime(std::chrono::milliseconds time_emulated)
{
  Common::IniFile ini;
  std::string ini_path = File::GetUserPath(D_CONFIG_IDX) + "TimePlayed.ini";
  ini.Load(ini_path);

  auto time_list = ini.GetOrCreateSection("Time Played");
  int previous_time;

  time_list->Get(m_game_id, &previous_time);
  // convert to int for easier readability in TimePlayed.ini
  time_list->Set(m_game_id, previous_time + int(time_emulated.count()));
  ini.Save(ini_path);
}

u64 TimePlayed::GetTimePlayed()
{
  Common::IniFile ini;
  std::string ini_path = File::GetUserPath(D_CONFIG_IDX) + "TimePlayed.ini";
  ini.Load(ini_path);

  auto time_list = ini.GetOrCreateSection("Time Played");
  u64 previous_time;
  time_list->Get(m_game_id, &previous_time);
  return previous_time;
}
