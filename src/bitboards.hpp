#pragma once

#include <stdint.h>
#include <intrin.h>

inline constexpr bool test_square(uint64_t bitboard, int square)
{
	return bitboard & (1ULL << square);
}

inline constexpr void set_square(uint64_t& bitboard, int square)
{
	bitboard |= 1ULL << square;
}

inline constexpr void clear_square(uint64_t& bitboard, int square)
{
	bitboard &= ~(1ULL << square);
}

inline int get_square(uint64_t bitboard)
{
	return (int)_tzcnt_u64(bitboard);
}

inline int pop_square(uint64_t& bitboard)
{
	int square = get_square(bitboard);
	clear_square(bitboard, square);
	return square;
}

inline int count_squares(uint64_t bitboard)
{
	return (int)__popcnt64(bitboard);
}

inline void flip_squares(uint64_t& bitboard)
{
	bitboard = _byteswap_uint64(bitboard);
}
