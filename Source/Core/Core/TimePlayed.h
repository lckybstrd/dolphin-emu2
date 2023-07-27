#pragma once
#include "Common/CommonTypes.h"
#include "Common/IniFile.h"

class TimePlayed
{
public:
  TimePlayed(std::string game_id);

  void AddTime(std::chrono::milliseconds time_emulated);

  u64 GetTimePlayed();

private:
  std::string m_game_id;
};
