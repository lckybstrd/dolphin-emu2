// Copyright 2022 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <string_view>

#include <picojson.h>

#include "VideoCommon/GraphicsModSystem/Runtime/GraphicsModAction.h"

class MoveAction final : public GraphicsModAction
{
public:
  static constexpr std::string_view factory_name = "move";
  static std::unique_ptr<MoveAction> Create(const picojson::value& json_data);
  static std::unique_ptr<MoveAction> Create();

  MoveAction() = default;
  explicit MoveAction(Common::Vec3 position_offset);

  void DrawImGui() override;

  void OnProjection(GraphicsModActionData::Projection* projection) override;
  void OnProjectionAndTexture(GraphicsModActionData::Projection* projection) override;

  void SerializeToConfig(picojson::object* obj) override;
  std::string GetFactoryName() const override;

private:
  Common::Vec3 m_position_offset;
};
