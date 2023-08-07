#pragma once

#include <string>

#include "Common/FileUtil.h"
#include "Common/IniFile.h"
#include "TimePlayed.h"

// used for QT interface - general access to time played for games
TimePlayed::TimePlayed()
{
  m_game_id = "None";
  ini_path = File::GetUserPath(D_CONFIG_IDX) + "TimePlayed.ini";
  ini.Load(ini_path);
  time_list = ini.GetOrCreateSection("Time Played");
}

void FilterUnsafeCharacters(std::string& game_id)
{
  const std::string forbiddenChars = "\\/:?\"<>|";
  for (auto& chr : game_id)
  {
    if (forbiddenChars.find(chr) != std::string::npos)
    {
      chr = '_';
    }
  }
}

TimePlayed::TimePlayed(std::string game_id)
{
  // filter for unsafe characters
  FilterUnsafeCharacters(game_id);

  m_game_id = game_id;
  ini_path = File::GetUserPath(D_CONFIG_IDX) + "TimePlayed.ini";
  ini.Load(ini_path);
  time_list = ini.GetOrCreateSection("Time Played");
}

void TimePlayed::AddTime(std::chrono::milliseconds time_emulated)
{
  if (m_game_id == "None")
  {
    return;
  }

  u64 previous_time;
  time_list->Get(m_game_id, &previous_time);
  time_list->Set(m_game_id, previous_time + u64(time_emulated.count()));
  ini.Save(ini_path);
}

u64 TimePlayed::GetTimePlayed()
{
  if (m_game_id == "None")
  {
    return 0;
  }

  u64 previous_time;
  time_list->Get(m_game_id, &previous_time);
  return previous_time;
}

u64 TimePlayed::GetTimePlayed(std::string game_id)
{
  FilterUnsafeCharacters(game_id);
  u64 previous_time;
  time_list->Get(game_id, &previous_time);
  return previous_time;
}
