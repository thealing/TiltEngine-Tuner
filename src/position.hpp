#pragma once

#include "chess.hpp"
#include "bitmasks.hpp"

#include <iterator>
#include <cstring>
#include <cctype>

struct Position
{
	uint64_t pieces[6];
	uint64_t colors[2];
	char move_counts[6];
	char attack_counts[6];
	char defense_counts[6];

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
		// precalculating mobility counters
		for (int color = 0; color < 2; color++)
		{
			uint64_t color_mask = colors[color];
			uint64_t opponent_mask = colors[color ^ 1];
			uint64_t occupied_mask = color_mask | opponent_mask;
			uint64_t empty_mask = ~occupied_mask;
			uint64_t pawn_mask = get_mask(PAWN, color);
			uint64_t pawn_move_mask = pawn_mask >> 8;
			uint64_t pawn_attack_mask = ((pawn_mask >> 7) & ~0x0101010101010101ULL) | ((pawn_mask >> 9) & ~0x8080808080808080ULL);
			move_counts[PAWN] += count_squares(pawn_move_mask & empty_mask);
			attack_counts[PAWN] += count_squares(pawn_attack_mask & opponent_mask);
			defense_counts[PAWN] += count_squares(pawn_attack_mask & color_mask);
			uint64_t opponent_pawn_mask = get_mask(PAWN, color ^ 1);
			uint64_t opponent_pawn_attack_mask = ((opponent_pawn_mask << 9) & ~0x0101010101010101ULL) | ((opponent_pawn_mask << 7) & ~0x8080808080808080ULL);
			for (int piece = 1; piece < 6; piece++)
			{
				uint64_t src_mask = get_mask(piece, color);
				while (src_mask != 0)
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
					dst_mask &= ~opponent_pawn_attack_mask;
					uint64_t move_mask = dst_mask & empty_mask;
					uint64_t attack_mask = dst_mask & opponent_mask;
					uint64_t defense_mask = dst_mask & color_mask;
					move_counts[piece] += count_squares(move_mask);
					attack_counts[piece] += count_squares(attack_mask);
					defense_counts[piece] += count_squares(defense_mask);
				}
			}
			for (int piece = 0; piece < 6; piece++)
			{
				move_counts[piece] *= -1;
				attack_counts[piece] *= -1;
				defense_counts[piece] *= -1;
			}
			flip();
		}
	}

	uint64_t get_mask(int piece, int color) const
	{
		return pieces[piece] & colors[color];
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
};
