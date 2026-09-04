#pragma once

#include "position.hpp"

#include <algorithm>
#include <numeric>

struct alignas(16) Score
{
	double opening;
	double endgame;

	Score() : opening{}, endgame{}
	{
	}

	Score(double opening, double endgame) : opening{opening}, endgame{endgame}
	{
	}

	explicit Score(double value) : opening{value}, endgame{value}
	{
	}

	explicit operator double() const
	{
		return opening + endgame;
	}

	Score operator-() const
	{
		return Score(-opening, -endgame);
	}

	Score operator+(const Score& other) const
	{
		return Score(opening + other.opening, endgame + other.endgame);
	}

	Score operator-(const Score& other) const
	{
		return Score(opening - other.opening, endgame - other.endgame);
	}

	Score operator*(const Score& other) const
	{
		return Score(opening * other.opening, endgame * other.endgame);
	}

	Score operator*(double scalar) const
	{
		return Score(opening * scalar, endgame * scalar);
	}

	Score operator/(double scalar) const
	{
		return Score(opening / scalar, endgame / scalar);
	}

	Score& operator+=(const Score& other)
	{
		opening += other.opening;
		endgame += other.endgame;
		return *this;
	}

	Score& operator-=(const Score& other)
	{
		opening -= other.opening;
		endgame -= other.endgame;
		return *this;
	}

	Score& operator*=(const Score& other)
	{
		opening *= other.opening;
		endgame *= other.endgame;
		return *this;
	}

	Score& operator*=(double scalar)
	{
		opening *= scalar;
		endgame *= scalar;
		return *this;
	}

	Score& operator/=(double scalar)
	{
		opening /= scalar;
		endgame /= scalar;
		return *this;
	}
};

struct Evaluation
{
	Score score;
	Score weight;

	double get_value() const
	{
		return (double)(score * weight);
	}
};

struct Evaluator
{
	constexpr static double PIECE_COUNTS[6] = { 16, 4, 4, 4, 2, 2 };

	Score piece_square_values[6][64];
	Score move_values[6];
	Score attack_values[6];
	Score defense_values[6];
	Score passed_pawn_values[64];
	Score doubled_pawn_value;
	Score isolated_pawn_value;
	Score backward_pawn_value;
	double weights[6];
	double total_weight;

	Evaluator()
	{
		memset(this, 0, sizeof(*this));
	}
	
	void init()
	{
		std::fill(std::begin(piece_square_values[0]), std::end(piece_square_values[0]), Score(100, 100));
		std::fill(std::begin(piece_square_values[1]), std::end(piece_square_values[1]), Score(300, 300));
		std::fill(std::begin(piece_square_values[2]), std::end(piece_square_values[2]), Score(300, 300));
		std::fill(std::begin(piece_square_values[3]), std::end(piece_square_values[3]), Score(500, 500));
		std::fill(std::begin(piece_square_values[4]), std::end(piece_square_values[4]), Score(900, 900));
		weights[PAWN] = 100;
		weights[KNIGHT] = 300;
		weights[BISHOP] = 300;
		weights[ROOK] = 500;
		weights[QUEEN] = 900;
		finalize();
	}

	void evaluate(Position& position, Evaluation& evaluation) const
	{
		Score score = {};
		uint64_t mask;
		for (int color = 0; color < 2; color++)
		{
			for (int piece = 0; piece < 6; piece++)
			{
				mask = position.get_mask(piece, color);
				while (mask)
				{
					int square = pop_square(mask);
					score += piece_square_values[piece][square];
				}
				score += move_values[piece] * position.move_counts[color][piece];
				score += attack_values[piece] * position.attack_counts[color][piece];
				score += defense_values[piece] * position.defense_counts[color][piece];
			}
			mask = position.passed_pawns[color];
			while (mask)
			{
				int square = pop_square(mask);
				score += passed_pawn_values[square];
			}
			score += doubled_pawn_value * count_squares(position.doubled_pawns[color]);
			score += isolated_pawn_value * count_squares(position.isolated_pawns[color]);
			score += backward_pawn_value * count_squares(position.backward_pawns[color]);
			score *= -1;
			position.flip();
		}
		double weight = 0;
		for (int piece = 0; piece < 6; piece++)
		{
			mask = position.pieces[piece];
			weight += count_squares(mask) * weights[piece];
		}
		evaluation.score = score;
		evaluation.weight = Score(weight, total_weight - weight) / total_weight;
	}

	void update(Position& position, const Evaluator& evaluator, const Evaluation& evaluation, double scale)
	{
		// Gradient Descent:
		// Add the derivative of all dynamic values to this instance, scaled by the "scale" parameter.
		// Using the "evaluator" and "evaluation" parameters from the previous iteration.
		Score score_change = evaluation.weight * scale;
		uint64_t mask;
		for (int color = 0; color < 2; color++)
		{
			for (int piece = 0; piece < 6; piece++)
			{
				mask = position.get_mask(piece, color);
				while (mask)
				{
					int square = pop_square(mask);
					piece_square_values[piece][square] += score_change;
				}
				move_values[piece] += score_change * position.move_counts[color][piece];
				attack_values[piece] += score_change * position.attack_counts[color][piece];
				defense_values[piece] += score_change * position.defense_counts[color][piece];
			}
			mask = position.passed_pawns[color];
			while (mask)
			{
				int square = pop_square(mask);
				passed_pawn_values[square] += score_change;
			}
			doubled_pawn_value += score_change * count_squares(position.doubled_pawns[color]);
			isolated_pawn_value += score_change * count_squares(position.isolated_pawns[color]);
			backward_pawn_value += score_change * count_squares(position.backward_pawns[color]);
			score_change *= -1;
			position.flip();
		}
		double t = evaluation.weight.opening * evaluator.total_weight;
		double T = evaluator.total_weight;
		double D = T * T;
		for (int piece = 0; piece < 6; piece++)
		{
			double w = evaluator.weights[piece];
			double p = count_squares(position.pieces[piece]);
			double m = PIECE_COUNTS[piece];
			double q = t - p * w;
			double Q = T - m * w;
			double x = p * Q - m * q;
			Score N = evaluation.score * Score(x, -x);
			weights[piece] += (double)N / D * scale;
		}
	}

	void finalize()
	{
		// This happens after each update iteration before the evaluating the positions.
		total_weight = std::inner_product(std::begin(weights), std::end(weights), std::begin(PIECE_COUNTS), 0.0);
	}
};
