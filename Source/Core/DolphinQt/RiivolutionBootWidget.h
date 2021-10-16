// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <optional>
#include <string>

#include <QDialog>

#include "Common/CommonTypes.h"
#include "DiscIO/RiivolutionParser.h"

class QPushButton;
class QVBoxLayout;

class RiivolutionBootWidget : public QDialog
{
  Q_OBJECT
public:
  explicit RiivolutionBootWidget(std::string game_id, std::optional<u16> revision,
                                 std::optional<u8> disc, QWidget* parent = nullptr);
  ~RiivolutionBootWidget();

  bool ShouldBoot() const { return m_should_boot; }
  std::vector<DiscIO::Riivolution::Patch>& GetPatches() { return m_patches; }

private:
  void CreateWidgets();

  void LoadMatchingXMLs();
  void OpenXML();
  void MakeGUIForParsedFile(const std::string& path, std::string root,
                            DiscIO::Riivolution::Disc input_disc);
  std::optional<DiscIO::Riivolution::Config> LoadConfigXML(const std::string& root_directory);
  void ApplyConfigDefaults(DiscIO::Riivolution::Disc* disc,
                           const DiscIO::Riivolution::Config& config);
  void SaveConfigXMLs();
  void BootGame();

  std::string m_game_id;
  int m_revision;
  int m_disc_number;

  bool m_should_boot = false;
  struct DiscWithRoot
  {
    DiscIO::Riivolution::Disc m_disc;
    std::string m_root;
  };
  std::vector<DiscWithRoot> m_discs;
  std::vector<DiscIO::Riivolution::Patch> m_patches;

  QVBoxLayout* m_patch_section_layout;
};
