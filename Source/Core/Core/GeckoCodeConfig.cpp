// Copyright 2010 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <sstream>
#include <string>
#include <vector>

#include "Common/OnionConfig.h"
#include "Common/StringUtil.h"
#include "Core/GeckoCodeConfig.h"

namespace Gecko
{
void LoadCodes(OnionConfig::Layer* global_config, OnionConfig::Layer* local_config,
               std::vector<GeckoCode>& gcodes)
{
  OnionConfig::Layer* configs[] = {global_config, local_config};
  for (auto config : configs)
  {
    OnionConfig::Section* codes =
        config->GetOrCreateSection(OnionConfig::System::SYSTEM_MAIN, "Gecko");
    OnionConfig::Section* codes_enabled =
        config->GetOrCreateSection(OnionConfig::System::SYSTEM_MAIN, "Gecko_Enabled");

    std::vector<std::string> lines;
    codes->GetLines(&lines, false);

    GeckoCode gcode;

    for (auto& line : lines)
    {
      if (line.empty())
        continue;

      std::istringstream ss(line);

      switch ((line)[0])
      {
      // enabled or disabled code
      case '+':
        ss.seekg(1);
      case '$':
        if (gcode.name.size())
          gcodes.push_back(gcode);
        gcode = GeckoCode();
        gcode.enabled = (1 == ss.tellg());  // silly

        gcode.user_defined = config == local_config;
        ss.seekg(1, std::ios_base::cur);
        // read the code name
        std::getline(ss, gcode.name, '[');  // stop at [ character (beginning of contributor name)
        gcode.name = StripSpaces(gcode.name);
        // read the code creator name
        std::getline(ss, gcode.creator, ']');
        break;

      // notes
      case '*':
        gcode.notes.push_back(std::string(++line.begin(), line.end()));
        break;

      // either part of the code, or an option choice
      default:
      {
        GeckoCode::Code new_code;
        // TODO: support options
        new_code.original_line = line;
        ss >> std::hex >> new_code.address >> new_code.data;
        gcode.codes.push_back(new_code);
      }
      break;
      }
    }

    // add the last code
    if (gcode.name.size())
    {
      gcodes.push_back(gcode);
    }

    codes_enabled->GetLines(&lines, false);

    for (const std::string& line : lines)
    {
      if (line.size() == 0 || line[0] != '$')
      {
        continue;
      }
      std::string name = line.substr(1);
      for (GeckoCode& ogcode : gcodes)
      {
        if (ogcode.name == name)
        {
          ogcode.enabled = true;
        }
      }
    }
  }
}

// used by the SaveGeckoCodes function
static void SaveGeckoCode(std::vector<std::string>& lines, std::vector<std::string>& enabledLines,
                          const GeckoCode& gcode)
{
  if (gcode.enabled)
    enabledLines.push_back("$" + gcode.name);

  if (!gcode.user_defined)
    return;

  std::string name;

  // save the name
  name += '$';
  name += gcode.name;

  // save the creator name
  if (gcode.creator.size())
  {
    name += " [";
    name += gcode.creator;
    name += ']';
  }

  lines.push_back(name);

  // save all the code lines
  for (const GeckoCode::Code& code : gcode.codes)
  {
    // ss << std::hex << codes_iter->address << ' ' << codes_iter->data;
    // lines.push_back(StringFromFormat("%08X %08X", codes_iter->address,
    // codes_iter->data));
    lines.push_back(code.original_line);
    // ss.clear();
  }

  // save the notes
  for (const std::string& note : gcode.notes)
    lines.push_back(std::string("*") + note);
}

void SaveCodes(OnionConfig::Layer* config, const std::vector<GeckoCode>& gcodes)
{
  std::vector<std::string> lines;
  std::vector<std::string> enabledLines;

  for (const GeckoCode& geckoCode : gcodes)
  {
    SaveGeckoCode(lines, enabledLines, geckoCode);
  }

  OnionConfig::Section* gecko =
      config->GetOrCreateSection(OnionConfig::System::SYSTEM_MAIN, "Gecko");
  OnionConfig::Section* gecko_enabled =
      config->GetOrCreateSection(OnionConfig::System::SYSTEM_MAIN, "Gecko_Enabled");

  gecko->SetLines(lines);
  gecko_enabled->SetLines(enabledLines);
}
}
