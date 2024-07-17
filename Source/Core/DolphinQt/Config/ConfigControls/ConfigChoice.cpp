// Copyright 2017 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinQt/Config/ConfigControls/ConfigChoice.h"

#include <QSignalBlocker>

#include "Common/Config/Config.h"

#include "DolphinQt/Settings.h"

ConfigChoice::ConfigChoice(const QStringList& options, const Config::Info<int>& setting)
    : m_setting(setting)
{
  addItems(options);
  connect(this, &QComboBox::currentIndexChanged, this, &ConfigChoice::Update);
  setCurrentIndex(Get(m_setting));

  connect(&Settings::Instance(), &Settings::ConfigChanged, this, [this] {
    QFont bf = font();
    bf.setBold(GetActiveLayerForConfig(m_setting) != Config::LayerType::Base);
    setFont(bf);

    const QSignalBlocker blocker(this);
    setCurrentIndex(Get(m_setting));
  });
}

void ConfigChoice::Update(int choice) const
{
  SetBaseOrCurrent(m_setting, choice);
}

ConfigStringChoice::ConfigStringChoice(const std::vector<std::string>& options,
                                       const Config::Info<std::string>& setting)
    : m_setting(setting), m_text_is_data(true)
{
  for (const auto& op : options)
    addItem(QString::fromStdString(op));

  Connect();
  Load();
}

ConfigStringChoice::ConfigStringChoice(const std::vector<std::pair<QString, QString>>& options,
                                       const Config::Info<std::string>& setting)
    : m_setting(setting), m_text_is_data(false)
{
  for (const auto& [option_text, option_data] : options)
    addItem(option_text, option_data);

  Connect();
  Load();
}

void ConfigStringChoice::Connect()
{
  const auto on_config_changed = [this]() {
    QFont bf = font();
    bf.setBold(GetActiveLayerForConfig(m_setting) != Config::LayerType::Base);
    setFont(bf);

    Load();
  };

  connect(&Settings::Instance(), &Settings::ConfigChanged, this, on_config_changed);
  connect(this, &QComboBox::currentIndexChanged, this, &ConfigStringChoice::Update);
}

void ConfigStringChoice::Update(int index) const
{
  if (m_text_is_data)
  {
    SetBaseOrCurrent(m_setting, itemText(index).toStdString());
  }
  else
  {
    SetBaseOrCurrent(m_setting, itemData(index).toString().toStdString());
  }
}

void ConfigStringChoice::Load()
{
  const QString setting_value = QString::fromStdString(Get(m_setting));

  const int index = m_text_is_data ? findText(setting_value) : findData(setting_value);
  const QSignalBlocker blocker(this);
  setCurrentIndex(index);
}
