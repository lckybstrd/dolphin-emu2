#pragma once
#include "ByteWrapper.h"
#include <stdexcept>
#include "common/CommonTypes.h"
#include <type_traits>


  ByteWrapper::ByteWrapper()
  {
    byteType = UNDEFINED;
    numBytesUsedByType = -1;
    totalBytesAllocated = 0;
    bytes = 0ULL;
  }

  ByteWrapper* ByteWrapper::CreateByteWrapperFromU8(u8 value, bool fromMemoryAddr)
  {
    ByteWrapper* returnResult = new ByteWrapper();
    returnResult->bytes = 0;
    returnResult->bytes = static_cast<u64>(value);
    returnResult->totalBytesAllocated = 1;
    returnResult->numBytesUsedByType = -1;
    returnResult->byteType = UNDEFINED;
    returnResult->createdFromMemoryAddress = fromMemoryAddr;
    return returnResult;
  }

  ByteWrapper* ByteWrapper::CreateByteWrapperFromU16(u16 value, bool fromMemoryAddr)
  {
    ByteWrapper* returnResult = new ByteWrapper();
    returnResult->bytes = 0;
    returnResult->bytes = static_cast<u64>(value);
    returnResult->totalBytesAllocated = 2;
    returnResult->numBytesUsedByType = -1;
    returnResult->byteType = UNDEFINED;
    returnResult->createdFromMemoryAddress = fromMemoryAddr;
    return returnResult;
  }

  ByteWrapper* ByteWrapper::CreateByteWrapperFromU32(u32 value, bool fromMemoryAddr)
  {
    ByteWrapper* returnResult = new ByteWrapper();
    returnResult->bytes = 0ULL;
    returnResult->bytes = static_cast<u64>(value);
    returnResult->totalBytesAllocated = 4;
    returnResult->numBytesUsedByType = -1;
    returnResult->byteType = UNDEFINED;
    returnResult->createdFromMemoryAddress = fromMemoryAddr;
    return returnResult;
  }

  ByteWrapper* ByteWrapper::CreateByteWrapperFromU64(u64 value, bool fromMemoryAddr)
  {
    ByteWrapper* returnResult = new ByteWrapper();
    returnResult->bytes = 0ULL;
    returnResult->bytes = value;
    returnResult->totalBytesAllocated = 8;
    returnResult->numBytesUsedByType = -1;
    returnResult->byteType = UNDEFINED;
    returnResult->createdFromMemoryAddress = fromMemoryAddr;
    return returnResult;
  }

  ByteWrapper* ByteWrapper::CreateByteWrapperFromLongLong(long long value, bool fromMemoryAddr)
  {
    ByteWrapper* returnResult = new ByteWrapper();
    returnResult->bytes = 0ULL;
    memcpy(&returnResult->bytes, &value, sizeof(returnResult->bytes));
    returnResult->totalBytesAllocated = 8;
    returnResult->numBytesUsedByType = -1;
    returnResult->byteType = UNDEFINED;
    returnResult->createdFromMemoryAddress = fromMemoryAddr;
    return returnResult;
  }

  ByteWrapper* ByteWrapper::CreateBytewrapperFromDouble(double value, bool fromMemoryAddr)
  {
    ByteWrapper* returnResult = new ByteWrapper();
    returnResult->bytes = 0ULL;
    memcpy(&returnResult->bytes, &value, sizeof(returnResult->bytes));
    returnResult->totalBytesAllocated = 8;
    returnResult->numBytesUsedByType = 1;
    returnResult->byteType = UNDEFINED;
    returnResult->createdFromMemoryAddress = fromMemoryAddr;
    return returnResult;
  }

  ByteWrapper* ByteWrapper::CreateByteWrapperFromCopy(ByteWrapper* value)
  {
    ByteWrapper* returnResult = new ByteWrapper();
    returnResult->bytes = value->bytes;
    returnResult->totalBytesAllocated = value->totalBytesAllocated;
    returnResult->numBytesUsedByType = value->numBytesUsedByType;
    returnResult->byteType = value->byteType;
    returnResult->createdFromMemoryAddress = value->createdFromMemoryAddress;
    return returnResult;
  }

  std::string ByteWrapper::getByteTypeAsString(ByteType inputType)
  {
    switch (inputType)
    {
    case UNSIGNED_8:
      return "UNSIGNED_8";
    case UNSIGNED_16:
      return "UNSIGNED_16";
    case UNSIGNED_32:
      return "UNSIGNED_32";
    case UNSIGNED_64:
      return "UNSIGNED_64";
    case SIGNED_8:
      return "SIGNED_8";
    case SIGNED_16:
      return "SIGNED_16";
    case SIGNED_32:
      return "SIGNED_32";
    case SIGNED_64:
      return "SIGNED_64";
    case FLOAT:
      return "FLOAT";
    case DOUBLE:
      return "DOUBLE";
    default:
      return "UNDEFINED";
    }
  }

  ByteWrapper::ByteType ByteWrapper::parseType(const char* typeString)
  {
    if (typeString == NULL)
      return ByteWrapper::ByteType::UNDEFINED;

    size_t typeStringLength = strlen(typeString);

    if (typeStringLength < 2)
      return ByteWrapper::ByteType::UNDEFINED;

    switch (typeString[0])
    {
    case 'u':
    case 'U':
      switch (typeString[1])
      {
      case '8':
        return ByteWrapper::ByteType::UNSIGNED_8;

      case '1':
        if (typeStringLength < 3 || typeString[2] != '6')
        {
          return ByteWrapper::ByteType::UNDEFINED;
        }
        return ByteWrapper::ByteType::UNSIGNED_16;

      case '3':
        if (typeStringLength < 3 || typeString[2] != '2')
        {
          return ByteWrapper::ByteType::UNDEFINED;
        }
        return ByteWrapper::ByteType::UNSIGNED_32;

      case '6':
        if (typeStringLength < 3 || typeString[2] != '4')
        {
          return ByteWrapper::ByteType::UNDEFINED;
        }
        return ByteWrapper::ByteType::UNSIGNED_64;

      default:
        return ByteWrapper::ByteType::UNDEFINED;
      }

    case 's':
    case 'S':
      switch (typeString[1])
      {
      case '8':
        return ByteWrapper::ByteType::SIGNED_8;
      case '1':
        if (typeStringLength < 3 || typeString[2] != '6')
        {
          return ByteWrapper::ByteType::UNDEFINED;
        }
        return ByteWrapper::ByteType::SIGNED_16;
      case '3':
        if (typeStringLength < 3 || typeString[2] != '2')
        {
          return ByteWrapper::ByteType::UNDEFINED;
        }
        return ByteWrapper::ByteType::SIGNED_32;
      case '6':
        if (typeStringLength < 3 || typeString[2] != '4')
        {
          return ByteWrapper::ByteType::UNDEFINED;
        }
        return ByteWrapper::ByteType::SIGNED_64;
      default:
        return ByteWrapper::ByteType::UNDEFINED;
      }

    case 'f':
    case 'F':
      if (typeStringLength < 5 || !((typeString[1] == 'l' || typeString[1] == 'L') &&
                                    (typeString[2] == 'o' || typeString[2] == 'O') &&
                                    (typeString[3] == 'a' || typeString[3] == 'A') &&
                                    (typeString[4] == 't' || typeString[4] == 'T')))
      {
        return ByteWrapper::ByteType::UNDEFINED;
      }
      return ByteWrapper::ByteType::FLOAT;

    case 'd':
    case 'D':
      if (typeStringLength < 6 || !((typeString[1] == 'o' || typeString[1] == 'O') &&
                                    (typeString[2] == 'u' || typeString[2] == 'U') &&
                                    (typeString[3] == 'b' || typeString[3] == 'B') &&
                                    (typeString[4] == 'l' || typeString[4] == 'L') &&
                                    (typeString[5] == 'e' || typeString[5] == 'E')

                                        ))
      {
        return ByteWrapper::ByteType::UNDEFINED;
      }
      return ByteWrapper::ByteType::DOUBLE;

    default:
      return ByteWrapper::ByteType::UNDEFINED;
    }
  }

  // returns true if wrapper is big enough to accomodate type, and false otherwise.
  bool ByteWrapper::typeSizeCheck(lua_State* luaState, ByteWrapper* byteWrapperPointer,
                     ByteWrapper::ByteType parsedType, const char* errorMessage)
  {
    switch (parsedType)
    {
    case ByteWrapper::ByteType::UNSIGNED_8:
    case ByteWrapper::ByteType::SIGNED_8:
      if (byteWrapperPointer->totalBytesAllocated < 1)
      {
        luaL_error(
            luaState,
            std::vformat(errorMessage, std::make_format_args(
                                           ByteWrapper::getByteTypeAsString(parsedType).c_str(), 1))
                .c_str());
        return false;
      }
      break;

    case ByteWrapper::ByteType::UNSIGNED_16:
    case ByteWrapper::ByteType::SIGNED_16:
      if (byteWrapperPointer->totalBytesAllocated < 2)
      {
        luaL_error(
            luaState,
            std::vformat(errorMessage, std::make_format_args(
                                           ByteWrapper::getByteTypeAsString(parsedType).c_str(), 2))
                .c_str());
        return false;
      }
      break;

    case ByteWrapper::ByteType::UNSIGNED_32:
    case ByteWrapper::ByteType::SIGNED_32:
    case ByteWrapper::ByteType::FLOAT:
      if (byteWrapperPointer->totalBytesAllocated < 4)
      {
        luaL_error(
            luaState,
            std::vformat(errorMessage, std::make_format_args(
                                           ByteWrapper::getByteTypeAsString(parsedType).c_str(), 4))
                .c_str());
        return false;
      }
      break;

    case ByteWrapper::ByteType::UNSIGNED_64:
    case ByteWrapper::ByteType::SIGNED_64:
    case ByteWrapper::ByteType::DOUBLE:
      if (byteWrapperPointer->totalBytesAllocated < 8)
      {
        luaL_error(
            luaState,
            std::vformat(errorMessage, std::make_format_args(
                                           ByteWrapper::getByteTypeAsString(parsedType).c_str(), 8))
                .c_str());
        return false;
      }
      break;

    default:
      luaL_error(luaState, "Error: invalid type in typeSizeCheck()");
      return false;
      break;
    }
    return true;
  }

  ByteWrapper::ByteWrapper(u8 initialValue, bool fromMemoryAddr)
  {
    byteType = UNSIGNED_8;
    numBytesUsedByType = 1;
    totalBytesAllocated = 1;
    bytes = 0ULL;
    bytes = initialValue;
    createdFromMemoryAddress = fromMemoryAddr;
  }

  ByteWrapper::ByteWrapper(u16 initialValue, bool fromMemoryAddr)
  {
    byteType = UNSIGNED_16;
    numBytesUsedByType = 2;
    totalBytesAllocated = 2;
    bytes = 0ULL;
    bytes = initialValue;
    createdFromMemoryAddress = fromMemoryAddr;
  }

  ByteWrapper::ByteWrapper(u32 initialValue, bool fromMemoryAddr)
  {
    byteType = UNSIGNED_32;
    numBytesUsedByType = 4;
    totalBytesAllocated = 4;
    bytes = 0ULL;
    bytes = initialValue;
    createdFromMemoryAddress = fromMemoryAddr;
  }

  ByteWrapper::ByteWrapper(u64 initialValue, bool fromMemoryAddr)
  {
    byteType = UNSIGNED_64;
    numBytesUsedByType = 8;
    totalBytesAllocated = 8;
    bytes = 0ULL;
    bytes = initialValue;
    createdFromMemoryAddress = fromMemoryAddr;
  }

  void ByteWrapper::setType(ByteType newType)
  {
    switch (newType)
    {
    case UNSIGNED_8:
    case SIGNED_8:
      if (totalBytesAllocated < 1)
        throw std::invalid_argument("Error: Cannot set type to 1 byte data type when 0 bytes were allocated for ByteWrapper!");
      numBytesUsedByType = 1;
      byteType = newType;
      break;

    case UNSIGNED_16:
    case SIGNED_16:
      if (totalBytesAllocated < 2)
        throw std::invalid_argument("Error: Cannot set type to 2 byte data type when less than 2 bytes were allocated for ByteWrapper!");
      numBytesUsedByType = 2;
      byteType = newType;
      break;

    case UNSIGNED_32:
    case SIGNED_32:
    case FLOAT:
      if (totalBytesAllocated < 4)
        throw std::invalid_argument("Error: Cannot set type to 4 byte data type when less than 4 bytes were allocated for ByteWrapper!");
      numBytesUsedByType = 4;
      byteType = newType;
      break;

    case UNSIGNED_64:
    case SIGNED_64:
    case DOUBLE:
      if (totalBytesAllocated < 8)
        throw std::invalid_argument("Error: Cannot set type to 8 byte data type when less than 8 bytes were allocated for ByteWrapper!");
      numBytesUsedByType = 8;
      byteType = newType;
      break;
    }
  }

  bool ByteWrapper::doComparisonOperation(const ByteWrapper& otherByteWrapper, ByteWrapper::OPERATIONS operation) const
  {
    OPERATIONS result = UNDEFINED_OPERATION;
    if (totalBytesAllocated <= 0 || otherByteWrapper.totalBytesAllocated <= 0 || byteType == UNDEFINED || otherByteWrapper.byteType == UNDEFINED)
    {
      throw std::invalid_argument("Error: Type not specified for one of the ByteWrappers in the comparison "
                                  "clause. Both must be specified to make a valid comparison.");
    }

    u8 firstValU8 = 0;
    u16 firstValU16 = 0;
    u32 firstValU32 = 0;
    u64 firstValU64 = 0;
    s8 firstValS8 = 0;
    s16 firstValS16 = 0;
    s32 firstValS32 = 0;
    s64 firstValS64 = 0;
    float firstValFloat = 0;
    double firstValDouble = 0;

    switch (byteType)
    {
    case UNSIGNED_8:
     firstValU8 = getValueAsU8();

      switch (otherByteWrapper.byteType)
      {
      case UNSIGNED_8:
        result = comparison_helper(firstValU8, otherByteWrapper.getValueAsU8());
        break;

      case UNSIGNED_16:
        result = comparison_helper(firstValU8, otherByteWrapper.getValueAsU16());
        break;

      case UNSIGNED_32:
        result = comparison_helper(firstValU8, otherByteWrapper.getValueAsU32());
        break;

      case UNSIGNED_64:
        result = comparison_helper(firstValU8, otherByteWrapper.getValueAsU64());
        break;

      case SIGNED_8:
        result = comparison_helper(firstValU8, otherByteWrapper.getValueAsS8());
        break;

      case SIGNED_16:
        result = comparison_helper(firstValU8, otherByteWrapper.getValueAsS16());
        break;

      case SIGNED_32:
        result = comparison_helper(firstValU8, otherByteWrapper.getValueAsS32());
        break;

      case SIGNED_64:
        result = comparison_helper(firstValU8, otherByteWrapper.getValueAsS64());
        break;

      case FLOAT:
        result = comparison_helper(firstValU8, otherByteWrapper.getValueAsFloat());
        break;

      case DOUBLE:
        result = comparison_helper(firstValU8, otherByteWrapper.getValueAsDouble());
        break;

      default:
        throw std::invalid_argument("Error: Type of ByteWrapper must be specified before comparison can be done...");
      }
    break;

    case UNSIGNED_16:
      firstValU16 = getValueAsU16();

      switch (otherByteWrapper.byteType)
      {
      case UNSIGNED_8:
        result = comparison_helper(firstValU16, otherByteWrapper.getValueAsU8());
        break;

      case UNSIGNED_16:
        result = comparison_helper(firstValU16, otherByteWrapper.getValueAsU16());
        break;

      case UNSIGNED_32:
        result = comparison_helper(firstValU16, otherByteWrapper.getValueAsU32());
        break;

      case UNSIGNED_64:
        result = comparison_helper(firstValU16, otherByteWrapper.getValueAsU64());
        break;

      case SIGNED_8:
        result = comparison_helper(firstValU16, otherByteWrapper.getValueAsS8());
        break;

      case SIGNED_16:
        result = comparison_helper(firstValU16, otherByteWrapper.getValueAsS16());
        break;

      case SIGNED_32:
        result = comparison_helper(firstValU16, otherByteWrapper.getValueAsS32());
        break;

      case SIGNED_64:
        result = comparison_helper(firstValU16, otherByteWrapper.getValueAsS64());
        break;

      case FLOAT:
        result = comparison_helper(firstValU16, otherByteWrapper.getValueAsFloat());
        break;

      case DOUBLE:
        result = comparison_helper(firstValU16, otherByteWrapper.getValueAsDouble());
        break;

      default:
        throw std::invalid_argument("Error: Type of ByteWrapper must be specified before comparison can be done...");
      }
      break;

      case UNSIGNED_32:
      firstValU32 = getValueAsU32();

      switch (otherByteWrapper.byteType)
      {
      case UNSIGNED_8:
        result = comparison_helper(firstValU32, otherByteWrapper.getValueAsU8());
        break;

      case UNSIGNED_16:
        result = comparison_helper(firstValU32, otherByteWrapper.getValueAsU16());
        break;

      case UNSIGNED_32:
        result = comparison_helper(firstValU32, otherByteWrapper.getValueAsU32());
        break;

      case UNSIGNED_64:
        result = comparison_helper(firstValU32, otherByteWrapper.getValueAsU64());
        break;

      case SIGNED_8:
        result = comparison_helper(firstValU32, otherByteWrapper.getValueAsS8());
        break;

      case SIGNED_16:
        result = comparison_helper(firstValU32, otherByteWrapper.getValueAsS16());
        break;

      case SIGNED_32:
        result = comparison_helper(firstValU32, otherByteWrapper.getValueAsS32());
        break;

      case SIGNED_64:
        result = comparison_helper(firstValU32, otherByteWrapper.getValueAsS64());
        break;

      case FLOAT:
        result = comparison_helper(firstValU32, otherByteWrapper.getValueAsFloat());
        break;

      case DOUBLE:
        result = comparison_helper(firstValU32, otherByteWrapper.getValueAsDouble());
        break;

      default:
        throw std::invalid_argument(
            "Error: Type of ByteWrapper must be specified before comparison can be done...");
      }
      break;

      case UNSIGNED_64:
        firstValU64 = getValueAsU64();

        switch (otherByteWrapper.byteType)
        {
        case UNSIGNED_8:
          result = comparison_helper(firstValU64, otherByteWrapper.getValueAsU8());
          break;

        case UNSIGNED_16:
          result = comparison_helper(firstValU64, otherByteWrapper.getValueAsU16());
          break;

        case UNSIGNED_32:
          result = comparison_helper(firstValU64, otherByteWrapper.getValueAsU32());
          break;

        case UNSIGNED_64:
          result = comparison_helper(firstValU64, otherByteWrapper.getValueAsU64());
          break;

        case SIGNED_8:
          result = comparison_helper(firstValU64, otherByteWrapper.getValueAsS8());
          break;

        case SIGNED_16:
          result = comparison_helper(firstValU64, otherByteWrapper.getValueAsS16());
          break;

        case SIGNED_32:
          result = comparison_helper(firstValU64, otherByteWrapper.getValueAsS32());
          break;

        case SIGNED_64:
          result = comparison_helper(firstValU64, otherByteWrapper.getValueAsS64());
          break;

        case FLOAT:
          result = comparison_helper(firstValU64, otherByteWrapper.getValueAsFloat());
          break;

        case DOUBLE:
          result = comparison_helper(firstValU64, otherByteWrapper.getValueAsDouble());
          break;

        default:
          throw std::invalid_argument("Error: Type of ByteWrapper must be specified before comparison can be done...");
        }
        break;

        case SIGNED_8:
          firstValS8 = getValueAsS8();

          switch (otherByteWrapper.byteType)
          {
          case UNSIGNED_8:
            result = comparison_helper(firstValS8, otherByteWrapper.getValueAsU8());
            break;

          case UNSIGNED_16:
            result = comparison_helper(firstValS8, otherByteWrapper.getValueAsU16());
            break;

          case UNSIGNED_32:
            result = comparison_helper(firstValS8, otherByteWrapper.getValueAsU32());
            break;

          case UNSIGNED_64:
            result = comparison_helper(firstValS8, otherByteWrapper.getValueAsU64());
            break;

          case SIGNED_8:
            result = comparison_helper(firstValS8, otherByteWrapper.getValueAsS8());
            break;

          case SIGNED_16:
            result = comparison_helper(firstValS8, otherByteWrapper.getValueAsS16());
            break;

          case SIGNED_32:
            result = comparison_helper(firstValS8, otherByteWrapper.getValueAsS32());
            break;

          case SIGNED_64:
            result = comparison_helper(firstValS8, otherByteWrapper.getValueAsS64());
            break;

          case FLOAT:
            result = comparison_helper(firstValS8, otherByteWrapper.getValueAsFloat());
            break;

          case DOUBLE:
            result = comparison_helper(firstValS8, otherByteWrapper.getValueAsDouble());
            break;

          default:
            throw std::invalid_argument(
                "Error: Type of ByteWrapper must be specified before comparison can be done...");
          }
          break;

          case SIGNED_16:
            firstValS16 = getValueAsS16();

            switch (otherByteWrapper.byteType)
            {
            case UNSIGNED_8:
              result = comparison_helper(firstValS16, otherByteWrapper.getValueAsU8());
              break;

            case UNSIGNED_16:
              result = comparison_helper(firstValS16, otherByteWrapper.getValueAsU16());
              break;

            case UNSIGNED_32:
              result = comparison_helper(firstValS16, otherByteWrapper.getValueAsU32());
              break;

            case UNSIGNED_64:
              result = comparison_helper(firstValS16, otherByteWrapper.getValueAsU64());
              break;

            case SIGNED_8:
              result = comparison_helper(firstValS16, otherByteWrapper.getValueAsS8());
              break;

            case SIGNED_16:
              result = comparison_helper(firstValS16, otherByteWrapper.getValueAsS16());
              break;

            case SIGNED_32:
              result = comparison_helper(firstValS16, otherByteWrapper.getValueAsS32());
              break;

            case SIGNED_64:
              result = comparison_helper(firstValS16, otherByteWrapper.getValueAsS64());
              break;

            case FLOAT:
              result = comparison_helper(firstValS16, otherByteWrapper.getValueAsFloat());
              break;

            case DOUBLE:
              result = comparison_helper(firstValS16, otherByteWrapper.getValueAsDouble());
              break;

            default:
              throw std::invalid_argument("Error: Type of ByteWrapper must be specified before comparison can be done...");
            }
            break;

            case SIGNED_32:
              firstValS32 = getValueAsS32();

              switch (otherByteWrapper.byteType)
              {
              case UNSIGNED_8:
                result = comparison_helper(firstValS32, otherByteWrapper.getValueAsU8());
                break;

              case UNSIGNED_16:
                result = comparison_helper(firstValS32, otherByteWrapper.getValueAsU16());
                break;

              case UNSIGNED_32:
                result = comparison_helper(firstValS32, otherByteWrapper.getValueAsU32());
                break;

              case UNSIGNED_64:
                result = comparison_helper(firstValS32, otherByteWrapper.getValueAsU64());
                break;

              case SIGNED_8:
                result = comparison_helper(firstValS32, otherByteWrapper.getValueAsS8());
                break;

              case SIGNED_16:
                result = comparison_helper(firstValS32, otherByteWrapper.getValueAsS16());
                break;

              case SIGNED_32:
                result = comparison_helper(firstValS32, otherByteWrapper.getValueAsS32());
                break;

              case SIGNED_64:
                result = comparison_helper(firstValS32, otherByteWrapper.getValueAsS64());
                break;

              case FLOAT:
                result = comparison_helper(firstValS32, otherByteWrapper.getValueAsFloat());
                break;

              case DOUBLE:
                result = comparison_helper(firstValS32, otherByteWrapper.getValueAsDouble());
                break;

              default:
                throw std::invalid_argument("Error: Type of ByteWrapper must be specified before comparison can be done...");
              }
              break;

              case SIGNED_64:
                firstValS64 = getValueAsS64();

                switch (otherByteWrapper.byteType)
                {
                case UNSIGNED_8:
                  result = comparison_helper(firstValS64, otherByteWrapper.getValueAsU8());
                  break;

                case UNSIGNED_16:
                  result = comparison_helper(firstValS64, otherByteWrapper.getValueAsU16());
                  break;

                case UNSIGNED_32:
                  result = comparison_helper(firstValS64, otherByteWrapper.getValueAsU32());
                  break;

                case UNSIGNED_64:
                  result = comparison_helper(firstValS64, otherByteWrapper.getValueAsU64());
                  break;

                case SIGNED_8:
                  result = comparison_helper(firstValS64, otherByteWrapper.getValueAsS8());
                  break;

                case SIGNED_16:
                  result = comparison_helper(firstValS64, otherByteWrapper.getValueAsS16());
                  break;

                case SIGNED_32:
                  result = comparison_helper(firstValS64, otherByteWrapper.getValueAsS32());
                  break;

                case SIGNED_64:
                  result = comparison_helper(firstValS64, otherByteWrapper.getValueAsS64());
                  break;

                case FLOAT:
                  result = comparison_helper(firstValS64, otherByteWrapper.getValueAsFloat());
                  break;

                case DOUBLE:
                  result = comparison_helper(firstValS64, otherByteWrapper.getValueAsDouble());
                  break;

                default:
                  throw std::invalid_argument("Error: Type of ByteWrapper must be specified before comparison can be done...");
                }
                break;

                case FLOAT:
                  firstValFloat = getValueAsFloat();

                  switch (otherByteWrapper.byteType)
                  {
                  case UNSIGNED_8:
                    result = comparison_helper(firstValFloat, otherByteWrapper.getValueAsU8());
                    break;

                  case UNSIGNED_16:
                    result = comparison_helper(firstValFloat, otherByteWrapper.getValueAsU16());
                    break;

                  case UNSIGNED_32:
                    result = comparison_helper(firstValFloat, otherByteWrapper.getValueAsU32());
                    break;

                  case UNSIGNED_64:
                    result = comparison_helper(firstValFloat, otherByteWrapper.getValueAsU64());
                    break;

                  case SIGNED_8:
                    result = comparison_helper(firstValFloat, otherByteWrapper.getValueAsS8());
                    break;

                  case SIGNED_16:
                    result = comparison_helper(firstValFloat, otherByteWrapper.getValueAsS16());
                    break;

                  case SIGNED_32:
                    result = comparison_helper(firstValFloat, otherByteWrapper.getValueAsS32());
                    break;

                  case SIGNED_64:
                    result = comparison_helper(firstValFloat, otherByteWrapper.getValueAsS64());
                    break;

                  case FLOAT:
                    result = comparison_helper(firstValFloat, otherByteWrapper.getValueAsFloat());
                    break;

                  case DOUBLE:
                    result = comparison_helper(firstValFloat, otherByteWrapper.getValueAsDouble());
                    break;

                  default:
                    throw std::invalid_argument("Error: Type of ByteWrapper must be specified before comparison can be done...");
                  }
                  break;

                  case DOUBLE:
                    firstValDouble = getValueAsDouble();

                    switch (otherByteWrapper.byteType)
                    {
                    case UNSIGNED_8:
                      result = comparison_helper(firstValDouble, otherByteWrapper.getValueAsU8());
                      break;

                    case UNSIGNED_16:
                      result = comparison_helper(firstValDouble, otherByteWrapper.getValueAsU16());
                      break;

                    case UNSIGNED_32:
                      result = comparison_helper(firstValDouble, otherByteWrapper.getValueAsU32());
                      break;

                    case UNSIGNED_64:
                      result = comparison_helper(firstValDouble, otherByteWrapper.getValueAsU64());
                      break;

                    case SIGNED_8:
                      result = comparison_helper(firstValDouble, otherByteWrapper.getValueAsS8());
                      break;

                    case SIGNED_16:
                      result = comparison_helper(firstValDouble, otherByteWrapper.getValueAsS16());
                      break;

                    case SIGNED_32:
                      result = comparison_helper(firstValDouble, otherByteWrapper.getValueAsS32());
                      break;

                    case SIGNED_64:
                      result = comparison_helper(firstValDouble, otherByteWrapper.getValueAsS64());
                      break;

                    case FLOAT:
                      result = comparison_helper(firstValDouble, otherByteWrapper.getValueAsFloat());
                      break;

                    case DOUBLE:
                      result = comparison_helper(firstValDouble, otherByteWrapper.getValueAsDouble());
                      break;

                    default:
                      throw std::invalid_argument("Error: Type of ByteWrapper must be specified before comparison can be done...");
                    }
                    break;

                    default:
                      throw std::invalid_argument("Error: Type of ByteWrapper must be specified before comparison can be done...");
    }

    if (result == UNDEFINED_OPERATION)
      throw std::invalid_argument("Error: ByteWrapper contained invalid number for comparison.");

    switch (operation)
    {
    case EQUALS_EQUALS:
      return result == EQUALS_EQUALS;
    case LESS_THAN:
      return result == LESS_THAN;
    case GREATER_THAN:
      return result == GREATER_THAN;
    case NOT_EQUALS:
      return result != EQUALS_EQUALS;
    case GREATER_THAN_EQUALS:
      return result == GREATER_THAN || result == EQUALS_EQUALS;
    case LESS_THAN_EQUALS:
      return result == LESS_THAN || result == EQUALS_EQUALS;
    default:
      throw std::invalid_argument("Error: Comparison function was passed an invalid operation!");
    }

  }

  // Returns GREATER_THAN, LESS_THAN, or EQUALS_EQUALS to specify if firstVal is greater than,
  // less than, or equal to the 2nd value.
  template<typename T, typename V>
  ByteWrapper::OPERATIONS ByteWrapper::comparison_helper(T firstVal, V secondVal) const {
    if (std::is_same<T, V>::value)
    {
      if (firstVal > static_cast<T>(secondVal))
        return GREATER_THAN;
      else if (firstVal < static_cast<T>(secondVal))
        return LESS_THAN;
      else
        return EQUALS_EQUALS;
    }

    else if (firstVal < (T)0 && secondVal >= (V)0)
      return LESS_THAN;
    else if (firstVal > (T)0 && secondVal <= (V)0)
      return GREATER_THAN;

    else if (std::is_integral<T>::value && std::is_integral<V>::value)
    {
      if (std::is_unsigned<T>::value == std::is_unsigned<V>::value) //in this case, either both values are signed integer types or both are unsigned integer types. In either case, we cast the smaller type to the bigger type and then compare.
      {
        if (sizeof(T) > sizeof(V))
        {
          return comparison_helper(firstVal, static_cast<T>(secondVal));
        }
        else
        {
          return comparison_helper(static_cast<V>(firstVal), secondVal);
        }
      }
      else // case where 1 value is signed integer type and 1 is unsigned integer type
      {
        //We checked at the start of the function that both numbers are either positive/0, or are negative/0.
        //Since 1 type is unsigned, we now know that both numbers are >= 0. As such, we now convert both numbers
        //to the unsigned number which is the size of the largest type out of the 2.
          u8 sizeOfLargestType = sizeof(T);
          if (sizeof(V) > sizeOfLargestType)
            sizeOfLargestType = sizeof(V);

          if (sizeOfLargestType == 1)
            return comparison_helper(static_cast<u8>(firstVal), static_cast<u8>(secondVal));
          else if (sizeOfLargestType == 2)
            return comparison_helper(static_cast<u16>(firstVal), static_cast<u16>(secondVal));
          else if (sizeOfLargestType == 4)
            return comparison_helper(static_cast<u32>(firstVal), static_cast<u32>(secondVal));
          else if (sizeOfLargestType == 8)
            return comparison_helper(static_cast<u64>(firstVal), static_cast<u64>(secondVal));
        }
      }
    else // At this point, we either have 2 floating point types or a floating point type and an integer.
         // Regardless, we should convert both numbers to doubles and then compare them.
    {
      return comparison_helper(static_cast<double>(firstVal), static_cast<double>(secondVal));
    }

    return UNDEFINED_OPERATION;
    }

    // If the number represented by the first wrapper equals the number represented by the 2nd
  // wrapper, then true is returned. Otherwise, false is returned.
  bool ByteWrapper::operator==(const ByteWrapper& otherByteWrapper) const
  {
    return doComparisonOperation(otherByteWrapper, EQUALS_EQUALS);
  }

  bool ByteWrapper::operator!=(const ByteWrapper& otherByteWrapper) const
  {
    return doComparisonOperation(otherByteWrapper, NOT_EQUALS);
  }

  bool ByteWrapper::operator>(const ByteWrapper& otherByteWrapper) const
  {
    return doComparisonOperation(otherByteWrapper, GREATER_THAN);
  }

  bool ByteWrapper::operator<(const ByteWrapper& otherByteWrapper) const
  {
    return doComparisonOperation(otherByteWrapper, LESS_THAN);
  }

  bool ByteWrapper::operator>=(const ByteWrapper& otherByteWrapper) const
  {
    return doComparisonOperation(otherByteWrapper, GREATER_THAN_EQUALS);
  }

  bool ByteWrapper::operator<=(const ByteWrapper& otherByteWrapper) const
  {
    return doComparisonOperation(otherByteWrapper, LESS_THAN_EQUALS);
  }

 ByteWrapper ByteWrapper::operator&(const ByteWrapper& otherByteWrapper) const
  {
   return doNonComparisonOperation(otherByteWrapper, BITWISE_AND);
 }
  ByteWrapper ByteWrapper::operator|(const ByteWrapper& otherByteWrapper) const
  {
    return doNonComparisonOperation(otherByteWrapper, BITWISE_OR);
  }

  ByteWrapper ByteWrapper::operator^(const ByteWrapper& otherByteWrapper) const
  {
    return doNonComparisonOperation(otherByteWrapper, BITWISE_XOR);
  }

  ByteWrapper ByteWrapper::operator<<(const ByteWrapper& otherByteWrapper) const
  {
    return doNonComparisonOperation(otherByteWrapper, BITSHIFT_LEFT);
  }

  ByteWrapper ByteWrapper::operator>>(const ByteWrapper& otherByteWrapper) const
  {
    return doNonComparisonOperation(otherByteWrapper, BITSHIFT_RIGHT);
  }

  ByteWrapper ByteWrapper::operator&&(const ByteWrapper& otherByteWrapper) const
  {
    return doNonComparisonOperation(otherByteWrapper, LOGICAL_AND);
  }

  ByteWrapper ByteWrapper::operator||(const ByteWrapper& otherByteWrapper) const
  {
    return doNonComparisonOperation(otherByteWrapper, LOGICAL_OR);
  }

  ByteWrapper ByteWrapper::operator~() const
  {
    return doUnaryOperation(BITWISE_NOT);
  }

  ByteWrapper ByteWrapper::operator!() const
  {
    return doUnaryOperation(LOGICAL_NOT);
  }

  template<typename T>
  T ByteWrapper::non_comparison_helper(T firstVal, T secondVal, OPERATIONS operation) const
  {
    switch (operation)
    {
    case BITWISE_AND:
      return firstVal & secondVal;
    case BITWISE_OR:
      return firstVal | secondVal;
    case BITWISE_XOR:
      return firstVal ^ secondVal;
    case BITSHIFT_LEFT:
      return firstVal << secondVal;
    case BITSHIFT_RIGHT:
      return firstVal >> secondVal;
    case LOGICAL_AND:
      return firstVal && secondVal;
    case LOGICAL_OR:
      return firstVal || secondVal;
    default:
      break;
    }
    throw std::invalid_argument("Error: Invalid argument passed to non_comparison_helper()");
  }

  ByteWrapper ByteWrapper::doNonComparisonOperation(const ByteWrapper& otherByteWrapper, OPERATIONS operation) const
{
    if (totalBytesAllocated <= 0 || otherByteWrapper.totalBytesAllocated <= 0 || byteType == UNDEFINED || otherByteWrapper.byteType == UNDEFINED)
    {
      throw std::invalid_argument("Error: type must be specified before bit operation can be performed on ByteWrapper");
    }

    u8 firstValU8 = 0U;
    u16 firstValU16 = 0U;
    u32 firstValU32 = 0U;
    u64 firstValU64 = 0ULL;
    u8 secondValU8 = 0U;
    u16 secondValU16 = 0U;
    u32 secondValU32 = 0U;
    u64 secondValU64 = 0ULL;

    u8 resultU8 = 0U;
    u16 resultU16 = 0U;
    u32 resultU32 = 0U;
    u64 resultU64 = 0ULL;

    u16 convertedU16 = 0U;
    u32 convertedU32 = 0U;
    u64 convertedU64 = 0ULL;

    switch (numBytesUsedByType)
    {
    case 1:
      firstValU8 = getValueAsU8();
      switch (otherByteWrapper.numBytesUsedByType)
      {
        case 1:
          secondValU8 = otherByteWrapper.getValueAsU8();
          resultU8 = non_comparison_helper(firstValU8, secondValU8, operation);
          return ByteWrapper(resultU8, createdFromMemoryAddress || otherByteWrapper.createdFromMemoryAddress);
        case 2:
          secondValU16 = otherByteWrapper.getValueAsU16();
          convertedU16 = static_cast<u16>(firstValU8);
          resultU16 = non_comparison_helper(convertedU16, secondValU16, operation);
          return ByteWrapper(resultU16, createdFromMemoryAddress || otherByteWrapper.createdFromMemoryAddress);
        case 4:
          secondValU32 = otherByteWrapper.getValueAsU32();
          convertedU32 = static_cast<u32>(firstValU8);
          resultU32 = non_comparison_helper(convertedU32, secondValU32, operation);
          return ByteWrapper(resultU32, createdFromMemoryAddress || otherByteWrapper.createdFromMemoryAddress);
        case 8:
          secondValU64 = otherByteWrapper.getValueAsU64();
          convertedU64 = static_cast<u64>(firstValU8);
          resultU64 = non_comparison_helper(convertedU64, secondValU64, operation);
          return ByteWrapper(resultU64, createdFromMemoryAddress || otherByteWrapper.createdFromMemoryAddress);
          break;
      }
      break;
    case 2:
      firstValU16 = getValueAsU16();
      switch (otherByteWrapper.numBytesUsedByType)
      {
      case 1:
        secondValU8 = otherByteWrapper.getValueAsU8();
        convertedU16 = static_cast<u16>(secondValU8);
        resultU16 = non_comparison_helper(firstValU16, convertedU16, operation);
        return ByteWrapper(resultU16, createdFromMemoryAddress || otherByteWrapper.createdFromMemoryAddress);
      case 2:
        secondValU16 = otherByteWrapper.getValueAsU16();
        resultU16 = non_comparison_helper(firstValU16, secondValU16, operation);
        return ByteWrapper(resultU16, createdFromMemoryAddress || otherByteWrapper.createdFromMemoryAddress);
      case 4:
        secondValU32 = otherByteWrapper.getValueAsU32();
        convertedU32 = static_cast<u32>(firstValU16);
        resultU32 = non_comparison_helper(convertedU32, secondValU32, operation);
        return ByteWrapper(resultU32, createdFromMemoryAddress || otherByteWrapper.createdFromMemoryAddress);
      case 8:
        secondValU64 = otherByteWrapper.getValueAsU64();
        convertedU64 = static_cast<u64>(firstValU16);
        resultU64 = non_comparison_helper(convertedU64, secondValU64, operation);
        return ByteWrapper(resultU64, createdFromMemoryAddress || otherByteWrapper.createdFromMemoryAddress);
        break;
      }
      break;
    case 4:
      firstValU32 = getValueAsU32();
      switch (otherByteWrapper.numBytesUsedByType)
      {
      case 1:
        secondValU8 = otherByteWrapper.getValueAsU8();
        convertedU32 = static_cast<u32>(secondValU8);
        resultU32 = non_comparison_helper(firstValU32, convertedU32, operation);
        return ByteWrapper(resultU32, createdFromMemoryAddress || otherByteWrapper.createdFromMemoryAddress);
      case 2:
        secondValU16 = otherByteWrapper.getValueAsU16();
        convertedU32 = static_cast<u32>(secondValU16);
        resultU32 = non_comparison_helper(firstValU32, convertedU32, operation);
        return ByteWrapper(resultU32, createdFromMemoryAddress || otherByteWrapper.createdFromMemoryAddress);
      case 4:
        secondValU32 = otherByteWrapper.getValueAsU32();
        resultU32 = non_comparison_helper(firstValU32, secondValU32, operation);
        return ByteWrapper(resultU32, createdFromMemoryAddress || otherByteWrapper.createdFromMemoryAddress);
      case 8:
        secondValU64 = otherByteWrapper.getValueAsU64();
        convertedU64 = static_cast<u64>(firstValU32);
        resultU64 = non_comparison_helper(convertedU64, secondValU64, operation);
        return ByteWrapper(resultU64, createdFromMemoryAddress || otherByteWrapper.createdFromMemoryAddress);
        break;
      }
      break;
    case 8:
      firstValU64 = getValueAsU64();
      switch (otherByteWrapper.numBytesUsedByType)
      {
      case 1:
        secondValU8 = otherByteWrapper.getValueAsU8();
        convertedU64 = static_cast<u64>(secondValU8);
        resultU64 = non_comparison_helper(firstValU64, convertedU64, operation);
        return ByteWrapper(resultU64, createdFromMemoryAddress || otherByteWrapper.createdFromMemoryAddress);
      case 2:
        secondValU16 = otherByteWrapper.getValueAsU16();
        convertedU64 = static_cast<u64>(secondValU16);
        resultU64 = non_comparison_helper(firstValU64, convertedU64, operation);
        return ByteWrapper(resultU64, createdFromMemoryAddress || otherByteWrapper.createdFromMemoryAddress);
      case 4:
        secondValU32 = otherByteWrapper.getValueAsU32();
        convertedU64 = static_cast<u64>(secondValU32);
        resultU64 = non_comparison_helper(firstValU64, convertedU64, operation);
        return ByteWrapper(resultU64, createdFromMemoryAddress || otherByteWrapper.createdFromMemoryAddress);
      case 8:
        secondValU64 = otherByteWrapper.getValueAsU64();
        resultU64 = non_comparison_helper(firstValU64, secondValU64, operation);
        return ByteWrapper(resultU64, createdFromMemoryAddress || otherByteWrapper.createdFromMemoryAddress);
        break;
      }
      break;
    }
    return ByteWrapper();
  }

  ByteWrapper ByteWrapper::doUnaryOperation(OPERATIONS operation) const
  {
    if (totalBytesAllocated <= 0 || byteType == UNDEFINED)
    {
      throw std::invalid_argument("Error: type must be specified before bit operation can be performed on ByteWrapper");
    }

    u8 inputAsU8, outputValueAsU8;
    u16 inputAsU16, outputValueAsU16;
    u32 inputAsU32, outputValueAsU32;
    u64 inputAsU64, outputValueAsU64;

    switch (numBytesUsedByType)
    {
    case 1:
      inputAsU8 = getValueAsU8();
      outputValueAsU8 = unary_operation_helper(inputAsU8, operation);
      return ByteWrapper(outputValueAsU8, createdFromMemoryAddress);
    case 2:
      inputAsU16 = getValueAsU16();
      outputValueAsU16 = unary_operation_helper(inputAsU16, operation);
      return ByteWrapper(outputValueAsU16, createdFromMemoryAddress);
    case 4:
      inputAsU32 = getValueAsU32();
      outputValueAsU32 = unary_operation_helper(inputAsU32, operation);
      return ByteWrapper(outputValueAsU32, createdFromMemoryAddress);
    case 8:
      inputAsU64 = getValueAsU64();
      outputValueAsU64 = unary_operation_helper(inputAsU64, operation);
      return ByteWrapper(outputValueAsU64, createdFromMemoryAddress);
    break;
    }
    return ByteWrapper();
  }

  template <typename T>
  T ByteWrapper::unary_operation_helper(T val, OPERATIONS operation) const
  {
    switch (operation)
    {
    case BITWISE_NOT:
      return ~val;
    case LOGICAL_NOT:
      return !val;
      break;
    }
    throw std::invalid_argument("Error: Unary arguments in unary_operation_helper() must be either ~ or !");
  }

  u8 ByteWrapper::getValueAsU8() const
  {
    if (totalBytesAllocated < 1)
      throw std::overflow_error("Error: Cannot call getValueAsU8() on ByteWrapper with a size of 0");
    if (createdFromMemoryAddress)
      return static_cast<u8>(bytes >> 56);
    else
      return static_cast<u8>(bytes & 0Xff);
  }

  s8 ByteWrapper::getValueAsS8() const
  {
    if (totalBytesAllocated < 1)
      throw std::overflow_error("Error: Cannot call getValueAsS8() on ByteWrapper with a size of 0");
    u8 unsignedByte = static_cast<u8>(bytes >> 56);
    if (!createdFromMemoryAddress)
      unsignedByte = static_cast<u8>(bytes & 0Xff);
    s8 signedByte = 0;
    memcpy(&signedByte, &unsignedByte, sizeof(s8));
    return signedByte;
  }

  u16 ByteWrapper::getValueAsU16() const
  {
    if (totalBytesAllocated < 2)
      throw std::overflow_error("Error: Cannot call getValueAsU16() on ByteWrapper with a size less than 2");
    if (createdFromMemoryAddress)
      return static_cast<u16>(bytes >> 48);
    else
      return static_cast<u16>(bytes & 0Xffff);
  }

  s16 ByteWrapper::getValueAsS16() const
  {
    if (totalBytesAllocated < 2)
      throw std::overflow_error("Error: Cannot call getValueAsS16() on ByteWrapper with a size less than 2");
    u16 unsignedShort = static_cast<u16>(bytes >> 48);
    if (!createdFromMemoryAddress)
      unsignedShort = static_cast<u16>(bytes & 0Xffff);
    s16 signedShort = 0;
    memcpy(&signedShort, &unsignedShort, sizeof(s16));
    return signedShort;
  }

  u32 ByteWrapper::getValueAsU32() const
  {
    if (totalBytesAllocated < 4)
      throw std::overflow_error("Error: Cannot call getValueAsU32() on ByteWrapper with a size less than 4");
    if (createdFromMemoryAddress)
      return static_cast<u32>(bytes >> 32);
    else
      return static_cast<u32>(bytes & 0Xffffffff);
  }

  s32 ByteWrapper::getValueAsS32() const
  {
    if (totalBytesAllocated < 4)
      throw std::overflow_error("Error: Cannot call getValueAsS32() on ByteWrapper with a size less than 4");
    u32 unsignedInt = static_cast<u32>(bytes >> 32);
    if (!createdFromMemoryAddress)
      unsignedInt = static_cast<u32>(bytes & 0Xffffffff);
    s32 signedInt = 0;
    memcpy(&signedInt, &unsignedInt, sizeof(s32));
    return signedInt;
  }

  u64 ByteWrapper::getValueAsU64() const
  {
    if (totalBytesAllocated < 8)
      throw std::overflow_error("Error: Cannot call getValueAsU64() on ByteWrapper with a size less than 8");
    return static_cast<u64>(bytes & 0XffffffffffffffffULL);
  }

  s64 ByteWrapper::getValueAsS64() const
  {
    if (totalBytesAllocated < 8)
      throw std::overflow_error("Error: Cannot call getValueAsS64() on ByteWrapper with a size less than 8");
    u64 unsignedLongLong = static_cast<u64>(bytes & 0XffffffffffffffffULL);
    s64 signedLongLong = 0;
    memcpy(&signedLongLong, &unsignedLongLong, sizeof(s64));
    return signedLongLong;
  }

  float ByteWrapper::getValueAsFloat() const
  {
    if (totalBytesAllocated < 4)
      throw std::overflow_error("Error: Cannot call getValueAsFloat() on ByteWrapper with a size less than 4");
    u32 unsignedInt = static_cast<u32>(bytes >> 32);
    if (!createdFromMemoryAddress)
      unsignedInt = static_cast<u32>(bytes & 0Xffffffff);
    float floatResult = 0.0f;
    memcpy(&floatResult, &unsignedInt, sizeof(float));
    return floatResult;
  }

  double ByteWrapper::getValueAsDouble() const
  {
    if (totalBytesAllocated < 8)
      throw std::overflow_error("Error: Cannot call getValueAsDouble() on ByteWrapper with a size less than 8");
    u64 unsignedLongLong = static_cast<u64>(bytes & 0xffffffffffffffffULL);
    double doubleResult = 0.0;
    memcpy(&doubleResult, &unsignedLongLong, sizeof(double));
    return doubleResult;
  }


