// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <unordered_set>
#include <stdlib.h>
#include <string>
#include <utility>

#include "Common/CommonFuncs.h"
#include "Common/CommonPaths.h"
#include "Common/CommonTypes.h"
#include "Common/FileUtil.h"
#include "Common/Logging/Log.h"
#include "Common/NandPaths.h"
#include "Common/StringUtil.h"

namespace Common
{
static std::string s_temp_wii_root;

void InitializeWiiRoot(bool use_dummy)
{
  ShutdownWiiRoot();
  if (use_dummy)
  {
    s_temp_wii_root = File::CreateTempDir();
    if (s_temp_wii_root.empty())
    {
      ERROR_LOG(WII_IPC_FILEIO, "Could not create temporary directory");
      return;
    }
    File::CopyDir(File::GetSysDirectory() + WII_USER_DIR, s_temp_wii_root);
    WARN_LOG(WII_IPC_FILEIO, "Using temporary directory %s for minimal Wii FS",
             s_temp_wii_root.c_str());
    static bool s_registered;
    if (!s_registered)
    {
      s_registered = true;
      atexit(ShutdownWiiRoot);
    }
    File::SetUserPath(D_SESSION_WIIROOT_IDX, s_temp_wii_root);
  }
  else
  {
    File::SetUserPath(D_SESSION_WIIROOT_IDX, File::GetUserPath(D_WIIROOT_IDX));
  }
}

void ShutdownWiiRoot()
{
  if (!s_temp_wii_root.empty())
  {
    File::DeleteDirRecursively(s_temp_wii_root);
    s_temp_wii_root.clear();
  }
}

static std::string RootUserPath(FromWhichRoot from)
{
  int idx = from == FROM_CONFIGURED_ROOT ? D_WIIROOT_IDX : D_SESSION_WIIROOT_IDX;
  return File::GetUserPath(idx);
}

std::string GetTicketFileName(u64 _titleID, FromWhichRoot from)
{
  return StringFromFormat("%s/ticket/%08x/%08x.tik", RootUserPath(from).c_str(),
                          (u32)(_titleID >> 32), (u32)_titleID);
}

std::string GetTitleDataPath(u64 _titleID, FromWhichRoot from)
{
  return StringFromFormat("%s/title/%08x/%08x/data/", RootUserPath(from).c_str(),
                          (u32)(_titleID >> 32), (u32)_titleID);
}

std::string GetTMDFileName(u64 _titleID, FromWhichRoot from)
{
  return GetTitleContentPath(_titleID, from) + "title.tmd";
}
std::string GetTitleContentPath(u64 _titleID, FromWhichRoot from)
{
  return StringFromFormat("%s/title/%08x/%08x/content/", RootUserPath(from).c_str(),
                          (u32)(_titleID >> 32), (u32)_titleID);
}

bool CheckTitleTMD(u64 _titleID, FromWhichRoot from)
{
  const std::string TitlePath = GetTMDFileName(_titleID, from);
  if (File::Exists(TitlePath))
  {
    File::IOFile pTMDFile(TitlePath, "rb");
    u64 TitleID = 0;
    pTMDFile.Seek(0x18C, SEEK_SET);
    if (pTMDFile.ReadArray(&TitleID, 1) && _titleID == Common::swap64(TitleID))
      return true;
  }
  INFO_LOG(DISCIO, "Invalid or no tmd for title %08x %08x", (u32)(_titleID >> 32),
           (u32)(_titleID & 0xFFFFFFFF));
  return false;
}

bool CheckTitleTIK(u64 _titleID, FromWhichRoot from)
{
  const std::string ticketFileName = Common::GetTicketFileName(_titleID, from);
  if (File::Exists(ticketFileName))
  {
    File::IOFile pTIKFile(ticketFileName, "rb");
    u64 TitleID = 0;
    pTIKFile.Seek(0x1dC, SEEK_SET);
    if (pTIKFile.ReadArray(&TitleID, 1) && _titleID == Common::swap64(TitleID))
      return true;
  }
  INFO_LOG(DISCIO, "Invalid or no tik for title %08x %08x", (u32)(_titleID >> 32),
           (u32)(_titleID & 0xFFFFFFFF));
  return false;
}

std::string EscapeFileName(const std::string& filename)
{
  // Prevent paths from containing ./, ../, .../, ..../, and so on
  if (std::all_of(filename.begin(), filename.end(), [](char c) { return c == '.'; }))
    return ReplaceAll(filename, ".", "__2e__");

  // Escape all double underscores since we will use double underscores for our escape sequences
  std::string filename_with_escaped_double_underscores = ReplaceAll(filename, "__", "__5f__");

  // Escape all other characters that need to be escaped
  static const std::unordered_set<char> chars_to_replace = {'\"', '*', '/',  ':', '<',
                                                            '>',  '?', '\\', '|', '\x7f'};
  std::string result;
  result.reserve(filename_with_escaped_double_underscores.size());
  for (char character : filename_with_escaped_double_underscores)
  {
    if (character < 0x20 || chars_to_replace.find(character) != chars_to_replace.end())
      result.append(StringFromFormat("__%02x__", character));
    else
      result.push_back(character);
  }

  return result;
}

std::string EscapePath(const std::string& path)
{
  std::vector<std::string> split_strings;
  SplitString(path, '/', split_strings);

  std::vector<std::string> escaped_split_strings;
  escaped_split_strings.reserve(split_strings.size());
  for (const std::string& split_string : split_strings)
    escaped_split_strings.push_back(EscapeFileName(split_string));

  return UnsplitString(split_strings, '/');
}

std::string UnescapeFileName(const std::string& filename)
{
  std::string result = filename;
  size_t pos = 0;

  // Replace escape sequences of the format "__3f__" with the ASCII
  // character defined by the escape sequence's two hex digits.
  while ((pos = result.find("__", pos)) != std::string::npos)
  {
    u32 character;
    if (pos + 6 <= result.size() && result[pos + 4] == '_' && result[pos + 5] == '_')
      if (AsciiToHex(result.substr(pos + 2, 2), character))
        result.replace(pos, 6, {static_cast<char>(character)});

    ++pos;
  }

  return result;
}
}
