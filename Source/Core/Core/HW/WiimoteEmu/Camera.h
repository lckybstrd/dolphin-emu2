// Copyright 2019 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Common/BitField.h"
#include "Common/BitField2.h"
#include "Common/ChunkFile.h"
#include "Common/CommonTypes.h"
#include "Core/HW/WiimoteEmu/Dynamics.h"
#include "Core/HW/WiimoteEmu/I2CBus.h"
#include "InputCommon/ControllerEmu/ControlGroup/Cursor.h"

namespace Common
{
class Matrix44;
}

namespace WiimoteEmu
{
// Four bytes for two objects. Filled with 0xFF if empty
struct IRBasic
{
  using IRObject = Common::TVec2<u16>;

  u8 x1;
  u8 y1;
  BitField2<u8> _bf;
  u8 x2;
  u8 y2;

  FIELD_IN(_bf, u8, 0, 2, x2hi);
  FIELD_IN(_bf, u8, 2, 2, y2hi);
  FIELD_IN(_bf, u8, 4, 2, x1hi);
  FIELD_IN(_bf, u8, 6, 2, y1hi);

  auto GetObject1() const { return IRObject(x1hi() << 8 | x1, y1hi() << 8 | y1); }
  auto GetObject2() const { return IRObject(x2hi() << 8 | x2, y2hi() << 8 | y2); }

  void SetObject1(const IRObject& obj)
  {
    x1 = obj.x;
    x1hi() = obj.x >> 8;
    y1 = obj.y;
    y1hi() = obj.y >> 8;
  }
  void SetObject2(const IRObject& obj)
  {
    x2 = obj.x;
    x2hi() = obj.x >> 8;
    y2 = obj.y;
    y2hi() = obj.y >> 8;
  }
};
static_assert(sizeof(IRBasic) == 5, "Wrong size");

// Three bytes for one object
struct IRExtended
{
  u8 x;
  u8 y;
  BitField2<u8> _bf;

  FIELD_IN(_bf, u8, 0, 4, size);
  FIELD_IN(_bf, u8, 4, 2, xhi);
  FIELD_IN(_bf, u8, 6, 2, yhi);

  auto GetPosition() const { return IRBasic::IRObject(xhi() << 8 | x, yhi() << 8 | y); }
  void SetPosition(const IRBasic::IRObject& obj)
  {
    x = obj.x;
    xhi() = obj.x >> 8;
    y = obj.y;
    yhi() = obj.y >> 8;
  }
};
static_assert(sizeof(IRExtended) == 3, "Wrong size");

// Nine bytes for one object
// first 3 bytes are the same as extended
struct IRFull : IRExtended
{
  BitField2<u8> _byte3;
  BitField2<u8> _byte4;
  BitField2<u8> _byte5;
  BitField2<u8> _byte6;
  u8 zero;
  u8 intensity;

  FIELD_IN(_byte3, u8, 0, 7, xmin);
  FIELD_IN(_byte4, u8, 0, 7, ymin);
  FIELD_IN(_byte5, u8, 0, 7, xmax);
  FIELD_IN(_byte6, u8, 0, 7, ymax);
};
static_assert(sizeof(IRFull) == 9, "Wrong size");

class CameraLogic : public I2CSlave
{
public:
  // OEM sensor bar distance between LED clusters in meters.
  static constexpr float SENSOR_BAR_LED_SEPARATION = 0.2f;

  static constexpr int CAMERA_RES_X = 1024;
  static constexpr int CAMERA_RES_Y = 768;

  // Jordan: I calculate the FOV at 42 degrees horizontally and having a 4:3 aspect ratio.
  // This is 31.5 degrees vertically.
  static constexpr float CAMERA_AR = 4.f / 3;
  static constexpr float CAMERA_FOV_X = 42 * float(MathUtil::TAU) / 360;
  static constexpr float CAMERA_FOV_Y = CAMERA_FOV_X / CAMERA_AR;

  enum : u8
  {
    IR_MODE_BASIC = 1,
    IR_MODE_EXTENDED = 3,
    IR_MODE_FULL = 5,
  };

  void Reset();
  void DoState(PointerWrap& p);
  void Update(const Common::Matrix44& transform, Common::Vec2 field_of_view);
  void SetEnabled(bool is_enabled);

  static constexpr u8 I2C_ADDR = 0x58;
  static constexpr int CAMERA_DATA_BYTES = 36;

private:
  // TODO: some of this memory is write-only and should return error 7.
#pragma pack(push, 1)
  struct Register
  {
    // Contains sensitivity and other unknown data
    // TODO: Does disabling the camera peripheral reset the mode or sensitivity?
    std::array<u8, 9> sensitivity_block1;
    std::array<u8, 17> unk_0x09;

    // addr: 0x1a
    std::array<u8, 2> sensitivity_block2;
    std::array<u8, 20> unk_0x1c;

    // addr: 0x30
    u8 enable_object_tracking;
    std::array<u8, 2> unk_0x31;

    // addr: 0x33
    u8 mode;
    std::array<u8, 3> unk_0x34;

    // addr: 0x37
    std::array<u8, CAMERA_DATA_BYTES> camera_data;
    std::array<u8, 165> unk_0x5b;
  };
#pragma pack(pop)

  static_assert(0x100 == sizeof(Register));

public:
  // The real wiimote reads camera data from the i2c bus at offset 0x37:
  static const u8 REPORT_DATA_OFFSET = offsetof(Register, camera_data);

private:
  int BusRead(u8 slave_addr, u8 addr, int count, u8* data_out) override;
  int BusWrite(u8 slave_addr, u8 addr, int count, const u8* data_in) override;

  Register m_reg_data{};

  // When disabled the camera does not respond on the bus.
  // Change is triggered by wiimote report 0x13.
  bool m_is_enabled = false;
};
}  // namespace WiimoteEmu
