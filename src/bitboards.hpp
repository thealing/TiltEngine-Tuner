#pragma once

#include <cstdint>
#include <bit>

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
	return std::countr_zero(bitboard);
}

inline int pop_square(uint64_t& bitboard)
{
	int square = get_square(bitboard);
	clear_square(bitboard, square);
	return square;
}

inline int count_squares(uint64_t bitboard)
{
	return std::popcount(bitboard);
}

inline void flip_squares(uint64_t& bitboard)
{
	bitboard = std::byteswap(bitboard);
}

inline uint64_t move_up(uint64_t bitboard)
{
	return bitboard >> 8;
}

inline uint64_t move_down(uint64_t bitboard)
{
	return bitboard << 8;
}

inline uint64_t move_left(uint64_t bitboard)
{
	return (bitboard >> 1) & ~0x8080808080808080ULL;
}

inline uint64_t move_right(uint64_t bitboard)
{
	return (bitboard << 1) & ~0x0101010101010101ULL;
}
