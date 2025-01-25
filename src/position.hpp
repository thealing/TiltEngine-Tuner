#pragma once

#include "chess.hpp"
#include "bitmasks.hpp"

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <iterator>
#include <bitset>
#include <cstring>
#include <cctype>

struct Position
{
	uint64_t pieces[6];
	uint64_t colors[2];
	int16_t move_counts[2][6];
	int16_t attack_counts[2][6];
	int16_t defense_counts[2][6];
	uint64_t passed_pawns[2];
	uint64_t doubled_pawns[2];
	uint64_t isolated_pawns[2];
	uint64_t backward_pawns[2];

	Position()
	{
		memset(this, 0, sizeof(*this));
	}

	Position(const char fen[]) : Position()
	{
		// parsing fen
		for (int square = 0; square < 64; fen++)
		{
			char c = *fen;
			if (isdigit(c))
			{
				square += c - '0';
			}
			if (isalpha(c))
			{
				int piece;
				switch (tolower(c))
				{
					case 'p':
						piece = PAWN;
						break;
					case 'n':
						piece = KNIGHT;
						break;
					case 'b':
						piece = BISHOP;
						break;
					case 'r':
						piece = ROOK;
						break;
					case 'q':
						piece = QUEEN;
						break;
					case 'k':
						piece = KING;
						break;
				}
				set_square(pieces[piece], square);
				if (isupper(c))
				{
					set_square(colors[WHITE], square);
				}
				else
				{
					set_square(colors[BLACK], square);
				}
				square++;
			}
		}
		// preprocessing
		for (int color = 0; color < 2; color++)
		{
			uint64_t color_mask = colors[color];
			uint64_t opp_mask = colors[color ^ 1];
			uint64_t occupied_mask = color_mask | opp_mask;
			uint64_t empty_mask = ~occupied_mask;
			uint64_t pawn_mask = get_mask(PAWN, color);
			uint64_t pawn_move_mask = move_up(pawn_mask);
			uint64_t pawn_attack_mask = move_left(pawn_move_mask) | move_right(pawn_move_mask);
			uint64_t opp_pawn_mask = get_mask(PAWN, color ^ 1);
			uint64_t opp_pawn_move_mask = move_down(opp_pawn_mask);
			uint64_t opp_pawn_attack_mask = move_left(opp_pawn_move_mask) | move_right(opp_pawn_move_mask);
			move_counts[color][PAWN] += count_squares(pawn_move_mask & empty_mask);
			attack_counts[color][PAWN] += count_squares(pawn_attack_mask & opp_mask);
			defense_counts[color][PAWN] += count_squares(pawn_attack_mask & color_mask);
			for (int piece = 1; piece < 6; piece++)
			{
				uint64_t src_mask = get_mask(piece, color);
				while (src_mask)
				{
					int src_square = pop_square(src_mask);
					uint64_t dst_mask = 0;
					switch (piece)
					{
						case KNIGHT:
							dst_mask = get_knight_mask(src_square);
							break;
						case BISHOP:
							dst_mask = get_bishop_mask(src_square, occupied_mask);
							break;
						case ROOK:
							dst_mask = get_rook_mask(src_square, occupied_mask);
							break;
						case QUEEN:
							dst_mask = get_queen_mask(src_square, occupied_mask);
							break;
						case KING:
							dst_mask = get_king_mask(src_square);
							break;
					}
					dst_mask &= ~opp_pawn_attack_mask;
					uint64_t move_mask = dst_mask & empty_mask;
					uint64_t attack_mask = dst_mask & opp_mask;
					uint64_t defense_mask = dst_mask & color_mask;
					move_counts[color][piece] += count_squares(move_mask);
					attack_counts[color][piece] += count_squares(attack_mask);
					defense_counts[color][piece] += count_squares(defense_mask);
				}
			}
			while (pawn_mask)
			{
				int pawn_square = pop_square(pawn_mask);
				int pawn_rank = pawn_square / 8;
				int pawn_file = pawn_square % 8;
				bool passed = true;
				bool doubled = false;
				bool isolated = true;
				bool backward = true;
				uint64_t mask = pieces[PAWN];
				while (mask)
				{
					int square = pop_square(mask);
					int rank = square / 8;
					int file = square % 8;
					if (test_square(color_mask, square))
					{
						if (file == pawn_file && rank < pawn_rank)
						{
							doubled = true;
						}
						if (abs(file - pawn_file) == 1)
						{
							isolated = false;
						}
						if (abs(file - pawn_file) == 1 && rank >= pawn_rank)
						{
							backward = false;
						}
					}
					if (test_square(opp_mask, square))
					{
						if (abs(file - pawn_file) <= 1 && rank < pawn_rank)
						{
							passed = false;
						}
					}
					if (file == pawn_file && rank == pawn_rank - 1)
					{
						backward = false;
					}
				}
				for (int square = pawn_square - 8; square >= 0; square -= 8)
				{
					if (test_square(pawn_attack_mask, square))
					{
						backward = false;
						break;
					}
					if (test_square(opp_pawn_attack_mask, square))
					{
						break;
					}
				}
				if (passed)
				{
					isolated = false;
					backward = false;
				}
				if (isolated)
				{
					backward = false;
				}
				if (doubled)
				{
					passed = false;
					isolated = false;
				}
				if (passed)
				{
					set_square(passed_pawns[color], pawn_square);
				}
				if (doubled)
				{
					set_square(doubled_pawns[color], pawn_square);
				}
				if (isolated)
				{
					set_square(isolated_pawns[color], pawn_square);
				}
				if (backward)
				{
					set_square(backward_pawns[color], pawn_square);
				}
			}
			flip();
		}
	}

	uint64_t get_mask(int piece, int color) const
	{
		return pieces[piece] & colors[color];
	}

	int get_piece(int square) const
	{
		for (int piece = 0; piece < 6; piece++)
		{
			if (test_square(pieces[piece], square))
			{
				return piece;
			}
		}
		return NONE;
	}

	int get_color(int square) const
	{
		for (int color = 0; color < 2; color++)
		{
			if (test_square(colors[color], square))
			{
				return color;
			}
		}
		return NONE;
	}

	void flip()
	{
		for (int piece = 0; piece < 6; piece++)
		{
			flip_squares(pieces[piece]);
		}
		for (int color = 0; color < 2; color++)
		{
			flip_squares(colors[color]);
		}
	}

	std::string visualize() const
	{
		const char piece_letters[] = { 'p', 'n', 'b', 'r', 'q', 'k' };
		const char* piece_names[] = { "pawn", "knight", "bishop", "rook", "queen", "king" };
		const char* color_names[] = { "white", "black" };
		std::stringstream ss;
		ss << "board:" << '\n' << '\n';
		for (int square = 0; square < 64; square++)
		{
			int piece = get_piece(square);
			int color = get_color(square);
			char letter = piece < 0 ? '.' : piece_letters[piece];
			letter = color == WHITE ? toupper(letter) : tolower(letter);
			ss << letter;
			ss << ' ' << ' ';
			if (square % 8 == 7)
			{
				ss << '\n' << '\n';
			}
		}
		for (int color = 0; color < 2; color++)
		{
			auto format_square = [&](int square) -> std::string
			{
				return std::string() + (char)('a' + square % 8) + (char)(color == WHITE ? '8' - square / 8 : '1' + square / 8);
			};
			auto format_bitboard = [&](uint64_t bitboard) -> std::string
			{
				std::string str;
				while (bitboard)
				{
					int square = pop_square(bitboard);
					str += format_square(square) + ' ';
				}
				return str;
			};
			ss << std::setw(47) << color_names[color] << '\n';
			ss << std::setw(55) << "---------------------" << '\n' << '\n';
			ss << "                ";
			for (int piece = 0; piece < 6; piece++)
			{
				ss << std::setw(9) << piece_names[piece];
			}
			ss << '\n';
			ss << "move counts:    ";
			for (int piece = 0; piece < 6; piece++)
			{
				ss << std::setw(9) << move_counts[color][piece];
			}
			ss << '\n';
			ss << "attack counts:  ";
			for (int piece = 0; piece < 6; piece++)
			{
				ss << std::setw(9) << attack_counts[color][piece];
			}
			ss << '\n';
			ss << "defense counts: ";
			for (int piece = 0; piece < 6; piece++)
			{
				ss << std::setw(9) << defense_counts[color][piece];
			}
			ss << '\n' << '\n';
			ss << "passed pawns:   " << format_bitboard(passed_pawns[color]) << '\n' << '\n';
			ss << "doubled pawns:  " << format_bitboard(doubled_pawns[color]) << '\n' << '\n';
			ss << "isolated pawns: " << format_bitboard(isolated_pawns[color]) << '\n' << '\n';
			ss << "backward pawns: " << format_bitboard(backward_pawns[color]) << '\n' << '\n';
		}
		return ss.str();
	}
};
