#pragma once
#include "Common/CommonTypes.h"
#include "Common/IniFile.h"

class TimePlayed
{
public:
  TimePlayed();
  TimePlayed(std::string game_id);

  void AddTime(std::chrono::milliseconds time_emulated);

  u64 GetTimePlayed();
  u64 GetTimePlayed(std::string game_id);

private:
  std::string m_game_id;
  Common::IniFile ini;
  std::string ini_path;

  Common::IniFile::Section* time_list;
};
