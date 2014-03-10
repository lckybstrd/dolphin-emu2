// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

// NOTE: Do not break compatibility with devkitPPC here!
// In particular, this means we're stuck with the c++11 feature set
// supported by GCC 4.6 until a new update of devkitPPC is released.

#include <limits>

#include "CommonTypes.h"

#pragma once

/*
 * Abstract bitfield class
 *
 * Allows endianness-independent access to individual bitfields within some raw
 * integer value. The assembly generated by this class is identical to the
 * usage of raw bitfields, so it's a perfectly fine replacement.
 *
 * For BitField<X,Y,Z>, X is the distance of the bitfield to the LSB of the
 * raw value, Y is the length in bits of the bitfield. Z is an integer type
 * which determines the sign of the bitfield. Z must have the same size as the
 * raw integer.
 *
 *
 * General usage:
 *
 * Create a new union with the raw integer value as a member.
 * Then for each bitfield you want to expose, add a BitField member
 * in the union. The template parameters are the bit offset and the number
 * of desired bits.
 *
 * Changes in the bitfield members will then get reflected in the raw integer
 * value and vice-versa.
 *
 *
 * Sample usage:
 *
 * union SomeRegister
 * {
 *     u32 hex;
 *
 *     BitField<0,7,u32> first_seven_bits;     // unsigned
 *     BitField<7,8,32> next_eight_bits;       // unsigned
 *     BitField<3,15,s32> some_signed_fields;  // signed
 * };
 *
 * This is equivalent to the little-endian specific code:
 *
 * union SomeRegister
 * {
 *     u32 hex;
 *
 *     struct
 *     {
 *         u32 first_seven_bits : 7;
 *         u32 first_seven_bits : 8;
 *     };
 *     struct
 *     {
 *         u32 : 3; // padding
 *         s32 some_signed_fields : 15;
 *     };
 * };
 *
 *
 * Caveats:
 *
 * BitField provides automatic casting from and to u32 where appropriate.
 * However, when using non-typesafe functions like printf, an explicit cast
 * must be performed on the BitField object to make sure it gets passed
 * correctly, e.g. printf("Value: %d", (s32)some_register.some_signed_fields);
 *
 */
template<u32 position, u32 bits, typename T>
struct BitField
{
private:
	// This constructor might be considered ambiguous:
	// Would it initialize the storage or just the bitfield?
	// Hence, delete it. Use the assignment operator to set bitfield values!
	BitField(T val) = delete;

public:
	// Force default constructor to be created
	// so that we can use this within unions
	BitField() = default;

	BitField& operator = (T val)
	{
		storage = (storage & ~GetMask()) | ((val<<position) & GetMask());
		return *this;
	}

	operator T() const
	{
		if (bits == 64)
			return storage;

		if (std::numeric_limits<T>::is_signed)
		{
			u64 shift = 8 * sizeof(T) - bits;
			return (T)((storage & GetMask()) << (shift - position)) >> shift;
		}
		else
		{
			return (storage & GetMask()) >> position;
		}
	}

	static T MaxVal()
	{
		return (std::numeric_limits<T>::is_signed)
		            ? ((1ull << (bits - 1)) - 1ull)
		            : (bits == 64)
		                ? 0xFFFFFFFFFFFFFFFFull
		                : ((1ull << bits) - 1ull);
	}

private:
	u64 GetMask() const
	{
		return (bits == 64)
		           ? (~0ull)
		           : ((1ull << bits) - 1ull) << position;
	}

	T storage;

	static_assert(bits + position <= 8 * sizeof(T), "Bitfield out of range");

	// And, you know, just in case people specify something stupid like bits=position=0x80000000
	static_assert(position < 8 * sizeof(T), "Invalid position");
	static_assert(bits <= 8 * sizeof(T), "Invalid number of bits");
	static_assert(bits > 0, "Invalid number of bits");
};
