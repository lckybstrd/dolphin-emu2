#include "Core/Scripting/InternalAPIFunctions/RegistersAPI.h"

#include <algorithm>
#include <fmt/format.h>
#include <memory>
#include <string>

#include "Common/CommonTypes.h"
#include "Core/PowerPC/PowerPC.h"
#include "Core/Scripting/HelperClasses/ArgHolder.h"
#include "Core/Scripting/HelperClasses/ScriptCallLocations.h"
#include "Core/Scripting/HelperClasses/ClassMetadata.h"
#include "Core/Scripting/HelperClasses/ArgTypeEnum.h"
#include "Core/Scripting/HelperClasses/VersionResolver.h"
#include <unordered_map>

namespace Scripting::RegistersAPI
{
const char* class_name = "RegistersAPI";

static std::array all_registers_functions_metadata_list = {
  FunctionMetadata("getU8FromRegister", "1.0", "getU8FromRegister(\"R5\", 3)", GetU8FromRegister, ArgTypeEnum::U8, {ArgTypeEnum::String, ArgTypeEnum::LongLong}),
  FunctionMetadata("getU16FromRegister", "1.0", "getU16FromRegister(\"R5\", 2)", GetU16FromRegister, ArgTypeEnum::U16, {ArgTypeEnum::String, ArgTypeEnum::LongLong}),
  FunctionMetadata("getU32FromRegister", "1.0", "getU32FromRegister(\"R5\", 0)", GetU32FromRegister, ArgTypeEnum::U32, {ArgTypeEnum::String, ArgTypeEnum::LongLong}),
  FunctionMetadata("getU64FromRegister", "1.0", "getU64FromRegister(\"F5\", 0)", GetU64FromRegister, ArgTypeEnum::U64, {ArgTypeEnum::String, ArgTypeEnum::LongLong}),
  FunctionMetadata("getS8FromRegister", "1.0", "getS8FromRegister(\"R5\", 3)", GetS8FromRegister, ArgTypeEnum::S8, {ArgTypeEnum::String, ArgTypeEnum::LongLong}),
  FunctionMetadata("getS16FromRegister", "1.0", "getS16FromRegister(\"R5\", 2)", GetS16FromRegister, ArgTypeEnum::S16, {ArgTypeEnum::String, ArgTypeEnum::LongLong}),
  FunctionMetadata("getS32FromRegister", "1.0", "getS32FromRegister(\"R5\", 0)", GetS32FromRegister, ArgTypeEnum::Integer, {ArgTypeEnum::String, ArgTypeEnum::LongLong}),
  FunctionMetadata("getS64FromRegister", "1.0", "getS64FromRegister(\"F5\", 0)", GetS64FromRegister, ArgTypeEnum::LongLong, {ArgTypeEnum::String, ArgTypeEnum::LongLong}),
  FunctionMetadata("getFloatFromRegister", "1.0", "getFloatFromRegister(\"F5\", 4)", GetFloatFromRegister, ArgTypeEnum::Float, {ArgTypeEnum::String, ArgTypeEnum::LongLong}),
  FunctionMetadata("getDoubleFromRegister", "1.0", "getDoubleFromRegister(\"F5\", 0)", GetDoubleFromRegister, ArgTypeEnum::Double, {ArgTypeEnum::String, ArgTypeEnum::LongLong}),
  FunctionMetadata("getUnsignedBytesFromRegister", "1.0", "getUnsignedBytesFromRegister(\"R5\", 3, 1)", GetUnsignedBytesFromRegister, ArgTypeEnum::AddressToUnsignedByteMap, {ArgTypeEnum::String, ArgTypeEnum::LongLong, ArgTypeEnum::LongLong}),
  FunctionMetadata("getSignedBytesFromRegister", "1.0", "getSignedBytesFromRegister(\"R5\", 3, 1)", GetSignedBytesFromRegister, ArgTypeEnum::AddressToSignedByteMap, {ArgTypeEnum::String, ArgTypeEnum::LongLong, ArgTypeEnum::LongLong}),

  FunctionMetadata("writeU8ToRegister", "1.0", "writeU8ToRegister(\"R5\", 41, 3)", WriteU8ToRegister, ArgTypeEnum::VoidType, {ArgTypeEnum::String, ArgTypeEnum::U8, ArgTypeEnum::LongLong}),
  FunctionMetadata("writeU16ToRegister", "1.0", "writeU16ToRegister(\"R5\", 410, 2)", WriteU16ToRegister, ArgTypeEnum::VoidType, {ArgTypeEnum::String, ArgTypeEnum::U16, ArgTypeEnum::LongLong}),
  FunctionMetadata("writeU32ToRegister", "1.0", "writeU32ToRegister(\"R5\", 500300, 0)", WriteU32ToRegister, ArgTypeEnum::VoidType, {ArgTypeEnum::String, ArgTypeEnum::U32, ArgTypeEnum::LongLong}),
  FunctionMetadata("writeU64ToRegister", "1.0", "writeU64ToRegister(\"F5\", 700000, 0)", WriteU64ToRegister, ArgTypeEnum::VoidType, {ArgTypeEnum::String, ArgTypeEnum::U64, ArgTypeEnum::LongLong}),
  FunctionMetadata("writeS8ToRegister", "1.0", "writeS8ToRegister(\"R5\", -41, 3)", WriteS8ToRegister, ArgTypeEnum::VoidType, {ArgTypeEnum::String, ArgTypeEnum::S8, ArgTypeEnum::LongLong}),
  FunctionMetadata("writeS16ToRegister", "1.0", "writeS16ToRegister(\"R5\", -9850, 2)", WriteS16ToRegister, ArgTypeEnum::VoidType, {ArgTypeEnum::String, ArgTypeEnum::S16, ArgTypeEnum::LongLong}),
  FunctionMetadata("writeS32ToRegister", "1.0", "writeS32ToRegister(\"R5\", -800567, 0)", WriteS32ToRegister, ArgTypeEnum::VoidType, {ArgTypeEnum::String, ArgTypeEnum::Integer, ArgTypeEnum::LongLong}),
  FunctionMetadata("writeS64ToRegister", "1.0", "writeS64ToRegister(\"F5\", -1123456, 0)", WriteS64ToRegister, ArgTypeEnum::VoidType, {ArgTypeEnum::String, ArgTypeEnum::LongLong, ArgTypeEnum::LongLong}),
  FunctionMetadata("writeFloatToRegister", "1.0", "writeFloatToRegister(\"F5\", 41.23, 4)", WriteFloatToRegister, ArgTypeEnum::VoidType, {ArgTypeEnum::String, ArgTypeEnum::Float, ArgTypeEnum::LongLong}),
  FunctionMetadata("writeDoubleToRegister", "1.0", "writeDoubleToRegister(\"R5\", 78.32, 0)", WriteDoubleToRegister, ArgTypeEnum::VoidType, {ArgTypeEnum::String, ArgTypeEnum::Double, ArgTypeEnum::LongLong}),
  FunctionMetadata("writeBytesToRegister", "1.0", "writeBytesToRegister(\"R5\", indexToByteMap, 1)", WriteBytesToRegister, ArgTypeEnum::VoidType, {ArgTypeEnum::String, ArgTypeEnum::AddressToByteMap, ArgTypeEnum::LongLong})
};

 ClassMetadata GetRegistersApiClassData(const std::string& api_version)
{
  std::unordered_map<std::string, std::string> deprecated_functions_map;
  return {class_name, GetLatestFunctionsForVersion(all_registers_functions_metadata_list, api_version, deprecated_functions_map)};
}

// Currently supported registers are:
// r0 - r31, f0 - f31, PC, and LR (the register which stores the return address to jump to when a
// function call ends)
class RegisterObject
{
public:
  enum class RegisterType
  {
    GeneralPurposeRegister,
    FloatingPointRegister,
    PcRegister,
    ReturnRegister,  // LR register
    Undefined
  };

  RegisterObject(RegisterType new_register_type, u8 new_register_number)
  {
    register_type = new_register_type;
    register_number = new_register_number;
  }

  u8 register_number;
  RegisterType register_type;
};

RegisterObject ParseRegister(const std::string& register_string)
{
  if (register_string.length() == 0 || register_string[0] == '\0')
    return RegisterObject(RegisterObject::RegisterType::Undefined, 0);

  s64 register_number = 0;
  switch (register_string[0])
  {
  case 'r':
  case 'R':

    register_number = std::stoi(std::string(register_string).substr(1), nullptr);
    if (register_number < 0 || register_number > 31)
      return RegisterObject(RegisterObject::RegisterType::Undefined, 0);
    return RegisterObject(RegisterObject::RegisterType::GeneralPurposeRegister, register_number);
    break;

  case 'f':
  case 'F':
    register_number = std::stoi(std::string(register_string).substr(1), nullptr);
    if (register_number < 0 || register_number > 31)
      return RegisterObject(RegisterObject::RegisterType::Undefined, 0);
    return RegisterObject(RegisterObject::RegisterType::FloatingPointRegister, register_number);
    break;

  case 'p':
  case 'P':
    if (register_string[1] != 'c' && register_string[1] != 'C')
      return RegisterObject(RegisterObject::RegisterType::Undefined, 0);
    return RegisterObject(RegisterObject::RegisterType::PcRegister, 0);
    break;

  case 'l':
  case 'L':
    if (register_string[1] != 'r' && register_string[1] != 'R')
      return RegisterObject(RegisterObject::RegisterType::Undefined, 0);
    return RegisterObject(RegisterObject::RegisterType::ReturnRegister, 0);
    break;

  default:
    return RegisterObject(RegisterObject::RegisterType::Undefined, 0);
  }
}

u8* GetAddressForRegister(RegisterObject register_object)
{
  u8 register_number = 0;
  u8* address = nullptr;
  switch (register_object.register_type)
  {
  case RegisterObject::RegisterType::GeneralPurposeRegister:
    register_number = register_object.register_number;
    address = reinterpret_cast<u8*>(PowerPC::ppcState.gpr + register_number);
    return address;
  case RegisterObject::RegisterType::PcRegister:
    address = reinterpret_cast<u8*>(&PowerPC::ppcState.pc);
    return address;
  case RegisterObject::RegisterType::ReturnRegister:
    address = reinterpret_cast<u8*>(&PowerPC::ppcState.spr[SPR_LR]);
  case RegisterObject::RegisterType::FloatingPointRegister:
    address = reinterpret_cast<u8*>(PowerPC::ppcState.ps + register_number);
    return address;

  default:
    return nullptr;
  }
}

ArgHolder ReturnInvalidRegisterNameArgHolder(const std::string& register_name)
{
  return CreateErrorStringArgHolder(
      fmt::format("Invalid value of {} was passed in for register string. Supported register names "
                  "include R0-R31, F0-F31, PC and LR (case-insensitive)",
                  register_name));
}

bool IsOperationOutOfBounds(RegisterObject register_val, long long offset,
                            long long return_value_size)
{
  long long register_size_in_bytes = 4;
  if (register_val.register_type == RegisterObject::RegisterType::FloatingPointRegister)
    register_size_in_bytes = 16;

  if (offset < 0 || offset + return_value_size > register_size_in_bytes)
    return true;
  return false;
}

ArgHolder ReturnOperationOutOfBoundsError(std::string read_or_write, std::string return_type,
                                          std::string register_string, long long offset)
{
  return CreateErrorStringArgHolder(
      fmt::format("Attempt to {} {} with offset of {} at register {} failed. Attempted to {} past "
                  "the end of the register!",
                  read_or_write, return_type, offset, register_string, read_or_write));
}

bool IsRegisterObjectUndefined(const RegisterObject& register_object)
{
  return register_object.register_type == RegisterObject::RegisterType::Undefined;
}

ArgHolder GetU8FromRegister(ScriptCallLocations call_location, std::vector<ArgHolder>& args_list)
{
  const std::string register_string = args_list[0].string_val;
  long long offset = args_list[1].long_long_val;
  RegisterObject register_val = ParseRegister(register_string);
  if (IsRegisterObjectUndefined(register_val))
    return ReturnInvalidRegisterNameArgHolder(register_string);
  if (IsOperationOutOfBounds(register_val, offset, 1))
    return ReturnOperationOutOfBoundsError("read", "u8", register_string, offset);
 
  u8* address_pointer = GetAddressForRegister(register_val);
  if (address_pointer == nullptr)
    return ReturnInvalidRegisterNameArgHolder(register_string);

  u8 u8_return_val = 0;
  memcpy(&u8_return_val, address_pointer, sizeof(u8));
  return CreateU8ArgHolder(u8_return_val);
}

ArgHolder GetU16FromRegister(ScriptCallLocations call_location, std::vector<ArgHolder>& args_list)
{
  const std::string register_string = args_list[0].string_val;
  long long offset = args_list[1].long_long_val;
  RegisterObject register_val = ParseRegister(register_string);
  if (IsRegisterObjectUndefined(register_val))
    return ReturnInvalidRegisterNameArgHolder(register_string);
  if (IsOperationOutOfBounds(register_val, offset, 2))
    return ReturnOperationOutOfBoundsError("read", "u16", register_string, offset);

  u8* address_pointer = GetAddressForRegister(register_val);
  if (address_pointer == nullptr)
    return ReturnInvalidRegisterNameArgHolder(register_string);

  u16 u16_return_val = 0;
  memcpy(&u16_return_val, address_pointer, sizeof(u16));
  return CreateU16ArgHolder(u16_return_val);
}

ArgHolder GetU32FromRegister(ScriptCallLocations call_location, std::vector<ArgHolder>& args_list)
{
  const std::string register_string = args_list[0].string_val;
  long long offset = args_list[1].long_long_val;
  RegisterObject register_val = ParseRegister(register_string);
  if (IsRegisterObjectUndefined(register_val))
    return ReturnInvalidRegisterNameArgHolder(register_string);
  if (IsOperationOutOfBounds(register_val, offset, 4))
    return ReturnOperationOutOfBoundsError("read", "u32", register_string, offset);

  u8* address_pointer = GetAddressForRegister(register_val);
  if (address_pointer == nullptr)
    return ReturnInvalidRegisterNameArgHolder(register_string);

  u32 u32_return_val = 0;
  memcpy(&u32_return_val, address_pointer, sizeof(u32));
  return CreateU32ArgHolder(u32_return_val);
}

ArgHolder GetU64FromRegister(ScriptCallLocations call_location, std::vector<ArgHolder>& args_list)
{
  const std::string register_string = args_list[0].string_val;
  long long offset = args_list[1].long_long_val;
  RegisterObject register_val = ParseRegister(register_string);
  if (IsRegisterObjectUndefined(register_val))
    return ReturnInvalidRegisterNameArgHolder(register_string);
  if (IsOperationOutOfBounds(register_val, offset, 8))
    return ReturnOperationOutOfBoundsError("read", "u64", register_string, offset);

  u8* address_pointer = GetAddressForRegister(register_val);
  if (address_pointer == nullptr)
    return ReturnInvalidRegisterNameArgHolder(register_string);

  u64 u64_return_val = 0;
  memcpy(&u64_return_val, address_pointer, sizeof(u64));
  return CreateU64ArgHolder(u64_return_val);
}

ArgHolder GetS8FromRegister(ScriptCallLocations call_location, std::vector<ArgHolder>& args_list)
{
  const std::string register_string = args_list[0].string_val;
  long long offset = args_list[1].long_long_val;
  RegisterObject register_val = ParseRegister(register_string);
  if (IsRegisterObjectUndefined(register_val))
    return ReturnInvalidRegisterNameArgHolder(register_string);
  if (IsOperationOutOfBounds(register_val, offset, 1))
    return ReturnOperationOutOfBoundsError("read", "s8", register_string, offset);

  u8* address_pointer = GetAddressForRegister(register_val);
  if (address_pointer == nullptr)
    return ReturnInvalidRegisterNameArgHolder(register_string);

  s8 s8_return_val = 0;
  memcpy(&s8_return_val, address_pointer, sizeof(s8));
  return CreateS8ArgHolder(s8_return_val);
}

ArgHolder GetS16FromRegister(ScriptCallLocations call_location, std::vector<ArgHolder>& args_list)
{
  const std::string register_string = args_list[0].string_val;
  long long offset = args_list[1].long_long_val;
  RegisterObject register_val = ParseRegister(register_string);
  if (IsRegisterObjectUndefined(register_val))
    return ReturnInvalidRegisterNameArgHolder(register_string);
  if (IsOperationOutOfBounds(register_val, offset, 2))
    return ReturnOperationOutOfBoundsError("read", "s16", register_string, offset);

  u8* address_pointer = GetAddressForRegister(register_val);
  if (address_pointer == nullptr)
    return ReturnInvalidRegisterNameArgHolder(register_string);
  s16 s16_return_val = 0;
  memcpy(&s16_return_val, address_pointer, sizeof(s16));
  return CreateS16ArgHolder(s16_return_val);
}

ArgHolder GetS32FromRegister(ScriptCallLocations call_location, std::vector<ArgHolder>& args_list)
{
  const std::string register_string = args_list[0].string_val;
  long long offset = args_list[1].long_long_val;
  RegisterObject register_val = ParseRegister(register_string);
  if (IsRegisterObjectUndefined(register_val))
    return ReturnInvalidRegisterNameArgHolder(register_string);
  if (IsOperationOutOfBounds(register_val, offset, 4))
    return ReturnOperationOutOfBoundsError("read", "s32", register_string, offset);

  u8* address_pointer = GetAddressForRegister(register_val);
  if (address_pointer == nullptr)
    return ReturnInvalidRegisterNameArgHolder(register_string);

  s32 s32_return_val = 0;
  memcpy(&s32_return_val, address_pointer, sizeof(s32));
  return CreateIntArgHolder(s32_return_val);
}

ArgHolder GetS64FromRegister(ScriptCallLocations call_location, std::vector<ArgHolder>& args_list)
{
  const std::string register_string = args_list[0].string_val;
  long long offset = args_list[1].long_long_val;
  RegisterObject register_val = ParseRegister(register_string);
  if (IsRegisterObjectUndefined(register_val))
    return ReturnInvalidRegisterNameArgHolder(register_string);
  if (IsOperationOutOfBounds(register_val, offset, 8))
    return ReturnOperationOutOfBoundsError("read", "s64", register_string, offset);

  u8* address_pointer = GetAddressForRegister(register_val);
  if (address_pointer == nullptr)
    return ReturnInvalidRegisterNameArgHolder(register_string);

  s64 s64_return_val = 0;
  memcpy(&s64_return_val, address_pointer, sizeof(s64));
  return CreateLongLongArgHolder(s64_return_val);
}

ArgHolder GetFloatFromRegister(ScriptCallLocations call_location, std::vector<ArgHolder>& args_list)
{
  const std::string register_string = args_list[0].string_val;
  long long offset = args_list[1].long_long_val;
  RegisterObject register_val = ParseRegister(register_string);
  if (IsRegisterObjectUndefined(register_val))
    return ReturnInvalidRegisterNameArgHolder(register_string);
  if (IsOperationOutOfBounds(register_val, offset, 4))
    return ReturnOperationOutOfBoundsError("read", "float", register_string, offset);

  u8* address_pointer = GetAddressForRegister(register_val);
  if (address_pointer == nullptr)
    return ReturnInvalidRegisterNameArgHolder(register_string);

  float float_return_val = 0;
  memcpy(&float_return_val, address_pointer, sizeof(float));
  return CreateFloatArgHolder(float_return_val);
}

ArgHolder GetDoubleFromRegister(ScriptCallLocations call_location, std::vector<ArgHolder>& args_list)
{
  const std::string register_string = args_list[0].string_val;
  long long offset = args_list[1].long_long_val;
  RegisterObject register_val = ParseRegister(register_string);
  if (IsRegisterObjectUndefined(register_val))
    return ReturnInvalidRegisterNameArgHolder(register_string);
  if (IsOperationOutOfBounds(register_val, offset, 8))
    return ReturnOperationOutOfBoundsError("read", "double", register_string, offset);

  u8* address_pointer = GetAddressForRegister(register_val);
  if (address_pointer == nullptr)
    return ReturnInvalidRegisterNameArgHolder(register_string);

  double double_return_val = 0;
  memcpy(&double_return_val, address_pointer, sizeof(double));
  return CreateDoubleArgHolder(double_return_val);
}

ArgHolder GetUnsignedBytesFromRegister(ScriptCallLocations call_location,
                                       std::vector<ArgHolder>& args_list)
{
  const std::string register_string = args_list[0].string_val;
  long long num_bytes_to_read = args_list[1].long_long_val;
  long long offset = args_list[2].long_long_val;

  RegisterObject register_val = ParseRegister(register_string);
  if (IsRegisterObjectUndefined(register_val))
    return ReturnInvalidRegisterNameArgHolder(register_string);
  if (IsOperationOutOfBounds(register_val, offset, num_bytes_to_read))
    return CreateErrorStringArgHolder(
        fmt::format("Attempt to read {} UnsignedBytes from register {} with a starting offset of {} "
                    "failed. Attempted to read past the end of the register!",
                    num_bytes_to_read, register_string, offset));

  u8* address_pointer = GetAddressForRegister(register_val);
  if (address_pointer == nullptr)
    return ReturnInvalidRegisterNameArgHolder(register_string);

  std::map<long long, u8> index_to_unsigned_byte_map = std::map<long long, u8>();
  for (u8 i = 0; i < num_bytes_to_read; ++i)
  {
    u8 next_byte = 0;
    memcpy(&next_byte, address_pointer + offset + i, sizeof(u8));
    index_to_unsigned_byte_map[i + 1] = next_byte;
  }

  return CreateAddressToUnsignedByteMapArgHolder(index_to_unsigned_byte_map);
}

ArgHolder GetSignedBytesFromRegister(ScriptCallLocations call_location,
                                       std::vector<ArgHolder>& args_list)
{
  const std::string register_string = args_list[0].string_val;
  long long num_bytes_to_read = args_list[1].long_long_val;
  long long offset = args_list[2].long_long_val;

  RegisterObject register_val = ParseRegister(register_string);
  if (IsRegisterObjectUndefined(register_val))
    return ReturnInvalidRegisterNameArgHolder(register_string);
  if (IsOperationOutOfBounds(register_val, offset, num_bytes_to_read))
    return CreateErrorStringArgHolder(fmt::format(
        "Attempt to read {} SignedBytes from register {} with a starting offset of {} "
        "failed. Attempted to read past the end of the register!",
        num_bytes_to_read, register_string, offset));

  u8* address_pointer = GetAddressForRegister(register_val);
  if (address_pointer == nullptr)
    return ReturnInvalidRegisterNameArgHolder(register_string);

  std::map<long long, s8> index_to_signed_byte_map = std::map<long long, s8>();
  for (u8 i = 0; i < num_bytes_to_read; ++i)
  {
    s8 next_byte = 0;
    memcpy(&next_byte, address_pointer + offset + i, sizeof(s8));
    index_to_signed_byte_map[i + 1] = next_byte;
  }

  return CreateAddressToSignedByteMapArgHolder(index_to_signed_byte_map);
}

ArgHolder WriteU8ToRegister(ScriptCallLocations call_location, std::vector<ArgHolder>& args_list)
{
  const std::string register_string = args_list[0].string_val;
  u8 u8_val = args_list[1].u8_val;
  long long offset = args_list[2].long_long_val;

  RegisterObject register_val = ParseRegister(register_string);
  if (IsRegisterObjectUndefined(register_val))
    return ReturnInvalidRegisterNameArgHolder(register_string);
  if (IsOperationOutOfBounds(register_val, offset, 1))
    return ReturnOperationOutOfBoundsError("write", "u8", register_string, offset);

  u8* address_pointer = GetAddressForRegister(register_val);
  if (address_pointer == nullptr)
    return ReturnInvalidRegisterNameArgHolder(register_string);

  memcpy(address_pointer, &u8_val, sizeof(u8));
  return CreateVoidTypeArgHolder();
}

ArgHolder WriteU16ToRegister(ScriptCallLocations call_location, std::vector<ArgHolder>& args_list)
{
  const std::string register_string = args_list[0].string_val;
  u16 u16_val = args_list[1].u16_val;
  long long offset = args_list[2].long_long_val;

  RegisterObject register_val = ParseRegister(register_string);
  if (IsRegisterObjectUndefined(register_val))
    return ReturnInvalidRegisterNameArgHolder(register_string);
  if (IsOperationOutOfBounds(register_val, offset, 2))
    return ReturnOperationOutOfBoundsError("write", "u16", register_string, offset);

  u8* address_pointer = GetAddressForRegister(register_val);
  if (address_pointer == nullptr)
    return ReturnInvalidRegisterNameArgHolder(register_string);

  memcpy(address_pointer, &u16_val, sizeof(u16));
  return CreateVoidTypeArgHolder();
}

ArgHolder WriteU32ToRegister(ScriptCallLocations call_location, std::vector<ArgHolder>& args_list)
{
  const std::string register_string = args_list[0].string_val;
  u32 u32_val = args_list[1].u32_val;
  long long offset = args_list[2].long_long_val;

  RegisterObject register_val = ParseRegister(register_string);
  if (IsRegisterObjectUndefined(register_val))
    return ReturnInvalidRegisterNameArgHolder(register_string);
  if (IsOperationOutOfBounds(register_val, offset, 4))
    return ReturnOperationOutOfBoundsError("write", "u32", register_string, offset);

  u8* address_pointer = GetAddressForRegister(register_val);
  if (address_pointer == nullptr)
    return ReturnInvalidRegisterNameArgHolder(register_string);

  memcpy(address_pointer, &u32_val, sizeof(u32));
  return CreateVoidTypeArgHolder();
}

ArgHolder WriteU64ToRegister(ScriptCallLocations call_location, std::vector<ArgHolder>& args_list)
{
  const std::string register_string = args_list[0].string_val;
  u64 u64_val = args_list[1].u64_val;
  long long offset = args_list[2].long_long_val;

  RegisterObject register_val = ParseRegister(register_string);
  if (IsRegisterObjectUndefined(register_val))
    return ReturnInvalidRegisterNameArgHolder(register_string);
  if (IsOperationOutOfBounds(register_val, offset, 8))
    return ReturnOperationOutOfBoundsError("write", "u64", register_string, offset);

  u8* address_pointer = GetAddressForRegister(register_val);
  if (address_pointer == nullptr)
    return ReturnInvalidRegisterNameArgHolder(register_string);

  memcpy(address_pointer, &u64_val, sizeof(u64));
  return CreateVoidTypeArgHolder();
}

ArgHolder WriteS8ToRegister(ScriptCallLocations call_location, std::vector<ArgHolder>& args_list)
{
  const std::string register_string = args_list[0].string_val;
  s8 s8_val = args_list[1].s8_val;
  long long offset = args_list[2].long_long_val;

  RegisterObject register_val = ParseRegister(register_string);
  if (IsRegisterObjectUndefined(register_val))
    return ReturnInvalidRegisterNameArgHolder(register_string);
  if (IsOperationOutOfBounds(register_val, offset, 1))
    return ReturnOperationOutOfBoundsError("write", "s8", register_string, offset);

  u8* address_pointer = GetAddressForRegister(register_val);
  if (address_pointer == nullptr)
    return ReturnInvalidRegisterNameArgHolder(register_string);

  memcpy(address_pointer, &s8_val, sizeof(s8));
  return CreateVoidTypeArgHolder();
}

ArgHolder WriteS16ToRegister(ScriptCallLocations call_location, std::vector<ArgHolder>& args_list)
{
  const std::string register_string = args_list[0].string_val;
  s16 s16_val = args_list[1].s16_val;
  long long offset = args_list[2].long_long_val;

  RegisterObject register_val = ParseRegister(register_string);
  if (IsRegisterObjectUndefined(register_val))
    return ReturnInvalidRegisterNameArgHolder(register_string);
  if (IsOperationOutOfBounds(register_val, offset, 2))
    return ReturnOperationOutOfBoundsError("write", "s16", register_string, offset);

  u8* address_pointer = GetAddressForRegister(register_val);
  if (address_pointer == nullptr)
    return ReturnInvalidRegisterNameArgHolder(register_string);

  memcpy(address_pointer, &s16_val, sizeof(s16));
  return CreateVoidTypeArgHolder();
}

ArgHolder WriteS32ToRegister(ScriptCallLocations call_location, std::vector<ArgHolder>& args_list)
{
  const std::string register_string = args_list[0].string_val;
  s32 s32_val = args_list[1].int_val;
  long long offset = args_list[2].long_long_val;

  RegisterObject register_val = ParseRegister(register_string);
  if (IsRegisterObjectUndefined(register_val))
    return ReturnInvalidRegisterNameArgHolder(register_string);
  if (IsOperationOutOfBounds(register_val, offset, 4))
    return ReturnOperationOutOfBoundsError("write", "s32", register_string, offset);

  u8* address_pointer = GetAddressForRegister(register_val);
  if (address_pointer == nullptr)
    return ReturnInvalidRegisterNameArgHolder(register_string);

  memcpy(address_pointer, &s32_val, sizeof(s32));
  return CreateVoidTypeArgHolder();
}

ArgHolder WriteS64ToRegister(ScriptCallLocations call_location, std::vector<ArgHolder>& args_list)
{
  const std::string register_string = args_list[0].string_val;
  s64 s64_val = args_list[1].long_long_val;
  long long offset = args_list[2].long_long_val;

  RegisterObject register_val = ParseRegister(register_string);
  if (IsRegisterObjectUndefined(register_val))
    return ReturnInvalidRegisterNameArgHolder(register_string);
  if (IsOperationOutOfBounds(register_val, offset, 8))
    return ReturnOperationOutOfBoundsError("write", "s64", register_string, offset);

  u8* address_pointer = GetAddressForRegister(register_val);
  if (address_pointer == nullptr)
    return ReturnInvalidRegisterNameArgHolder(register_string);

  memcpy(address_pointer, &s64_val, sizeof(s64));
  return CreateVoidTypeArgHolder();
}

ArgHolder WriteFloatToRegister(ScriptCallLocations call_location, std::vector<ArgHolder>& args_list)
{
  const std::string register_string = args_list[0].string_val;
  float float_val = args_list[1].float_val;
  long long offset = args_list[2].long_long_val;

  RegisterObject register_val = ParseRegister(register_string);
  if (IsRegisterObjectUndefined(register_val))
    return ReturnInvalidRegisterNameArgHolder(register_string);
  if (IsOperationOutOfBounds(register_val, offset, 4))
    return ReturnOperationOutOfBoundsError("write", "float", register_string, offset);

  u8* address_pointer = GetAddressForRegister(register_val);
  if (address_pointer == nullptr)
    return ReturnInvalidRegisterNameArgHolder(register_string);

  memcpy(address_pointer, &float_val, sizeof(float));
  return CreateVoidTypeArgHolder();
}

ArgHolder WriteDoubleToRegister(ScriptCallLocations call_location, std::vector<ArgHolder>& args_list)
{
  const std::string register_string = args_list[0].string_val;
  double double_val = args_list[1].double_val;
  long long offset = args_list[2].long_long_val;

  RegisterObject register_val = ParseRegister(register_string);
  if (IsRegisterObjectUndefined(register_val))
    return ReturnInvalidRegisterNameArgHolder(register_string);
  if (IsOperationOutOfBounds(register_val, offset, 8))
    return ReturnOperationOutOfBoundsError("write", "double", register_string, offset);

  u8* address_pointer = GetAddressForRegister(register_val);
  if (address_pointer == nullptr)
    return ReturnInvalidRegisterNameArgHolder(register_string);

  memcpy(address_pointer, &double_val, sizeof(double));
  return CreateVoidTypeArgHolder();
}

ArgHolder WriteBytesToRegister(ScriptCallLocations call_location, std::vector<ArgHolder>& args_list)
{
  const std::string register_string = args_list[0].string_val;
  std::map<long long, s16> index_to_byte_map = args_list[1].address_to_byte_map;
  long long offset = args_list[2].long_long_val;

  RegisterObject register_val = ParseRegister(register_string);
  if (IsRegisterObjectUndefined(register_val))
    return ReturnInvalidRegisterNameArgHolder(register_string);
  if (IsOperationOutOfBounds(register_val, offset, index_to_byte_map.size()))
    return ReturnOperationOutOfBoundsError("write", "Bytes", register_string, offset);

  u8* address_pointer = GetAddressForRegister(register_val);
  if (address_pointer == nullptr)
    return ReturnInvalidRegisterNameArgHolder(register_string);

  int i = 0;
  for (auto& it : index_to_byte_map)
  {
    s16 curr_byte = it.second;
    if (curr_byte < -128 || curr_byte > 255)
      return CreateErrorStringArgHolder(
          fmt::format("Byte at offset of {} for register {} was outside the valid range of what can be represented by 1 byte "
          "(it was outside the range of -128-255)", i + offset, register_string));

    if (curr_byte < 0)
    {
      s8 curr_s8 = static_cast<s8>(curr_byte);
      memcpy(address_pointer + offset + i, &curr_s8, sizeof(s8));
    }
    else
    {
      u8 curr_u8 = static_cast<u8>(curr_byte);
      memcpy(address_pointer + offset + i, &curr_u8, sizeof(u8));
    }
    i++;
  }

  return CreateVoidTypeArgHolder();
}

}  // namespace Scripting::RegistersAPI
