#include "evaluation.hpp"
#include "stopwatch.hpp"
#include "save.hpp"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <vector>
#include <string>
#include <array>
#include <functional>
#include <thread>

#define THREAD_COUNT 7

#define USE_POSITION_CACHE false

#define EXPORT_INTERVAL 100

using namespace std;

inline const filesystem::path input_path = "input.txt";
inline const filesystem::path cache_path = "cache.bin";
inline const filesystem::path values_path = "values.bin";
inline const filesystem::path output_path = "out";

int N;
vector<Position> positions;
vector<double> results;

void load_input()
{
	Stopwatch stopwatch("loading from input file");
	ifstream file(input_path);
	if (!file)
	{
		cout << "opening input file failed" << endl;
		exit(1);
	}
	string line;
	while (getline(file, line))
	{
		N++;
		positions.emplace_back(line.c_str());
		double result = stod(line.substr(line.find('[') + 1));
		results.push_back(result);
		// TODO: more sophisticated checking of correctness?
#ifdef _DEBUG
		if (N % 12345 == 0)
		{
			cout << Position(line.c_str()).visualize() << endl;
			cin.get();
		}
#endif
	}
}

bool load_cache()
{
	InputFileStream file(cache_path);
	if (!file)
	{
		return false;
	}
	Stopwatch stopwatch("loading from cache file");
	file >> N >> positions >> results;
	return true;
}

void save_cache()
{
	Stopwatch stopwatch("saving to cache file");
	OutputFileStream file(cache_path);
	file << N << positions << results;
}

void load_data()
{
	if (USE_POSITION_CACHE)
	{
		if (load_cache())
		{
			return;
		}
		load_input();
		save_cache();
	}
	else
	{
		cout << "position cache disabled" << endl;
		load_input();
	}
}

inline const string piece_names[6] = { "pawn", "knight", "bishop", "rook", "queen", "king" };
inline const string phase_names[2] = { "opening", "endgame" };
inline const string type_prefix = "constexpr static Score _";
inline const string value_start = " = ";
inline const string value_end = ";\n";
inline const string square_array_start = "[SQUARE_COUNT] = {\n";
inline const string piece_array_start = "[PIECE_COUNT] = {\n";
inline const string array_end = "};\n";
inline const string identation = "  ";

void export_values(const filesystem::path& path, const Evaluator& evaluator)
{
	filesystem::create_directories(output_path);
	ofstream file(output_path / path);
	auto write_weights = [&]()
	{
		constexpr int piece_counts[6] = { 16, 4, 4, 4, 2, 2 };
		file << type_prefix << "weights[PIECE_COUNT] = { ";
		int total_weight = 0;
		for (int piece = 0; piece < 6; piece++)
		{
			int weight = (int)evaluator.weights[piece];
			total_weight += weight * piece_counts[piece];
			file << weight << (piece == 5 ? " " : ", ");
		}
		file << array_end;
		file << type_prefix << "total_weight" << value_start << total_weight << value_end;
	};
	auto write_value = [&](const Score& scores, int phase)
	{
		file << value_start << (int)((double*)&scores)[phase] << value_end;
	};
	auto write_pieces = [&](const Score* scores, int phase)
	{
		file << piece_array_start << identation;
		for (int piece = 0; piece < 6; piece++)
		{
			file << setw(4) << (int)((double*)&scores[piece])[phase] << (piece == 5 ? "\n" : ", ");
		}
		file << array_end;
	};
	auto write_squares = [&](const Score* scores, int phase)
	{
		file << square_array_start;
		for (int square = 0; square < 64; square++)
		{
			if (square % 8 == 0)
			{
				file << identation;
			}
			file << setw(4) << (int)((double*)&scores[square])[phase] << ", ";
			if (square % 8 == 7)
			{
				file << "\n";
			}
		}
		file << array_end;
	};
	write_weights();
	for (int phase = 0; phase < 2; phase++)
	{
		file << type_prefix << phase_names[phase] << "_move_values";
		write_pieces(evaluator.move_values, phase);
	}
	for (int phase = 0; phase < 2; phase++)
	{
		file << type_prefix << phase_names[phase] << "_attack_values";
		write_pieces(evaluator.attack_values, phase);
	}
	for (int phase = 0; phase < 2; phase++)
	{
		file << type_prefix << phase_names[phase] << "_defense_values";
		write_pieces(evaluator.defense_values, phase);
	}
	for (int piece = 0; piece < 6; piece++)
	{
		for (int phase = 0; phase < 2; phase++)
		{
			file << type_prefix << phase_names[phase] << '_' << piece_names[piece] << "_square_values";
			write_squares(evaluator.piece_square_values[piece], phase);
		}
	}
	for (int phase = 0; phase < 2; phase++)
	{
		file << type_prefix << phase_names[phase] << "_passed_pawn_values";
		write_squares(evaluator.passed_pawn_values, phase);
	}
	for (int phase = 0; phase < 2; phase++)
	{
		file << type_prefix << phase_names[phase] << "_doubled_pawn_value";
		write_value(evaluator.doubled_pawn_value, phase);
	}
	for (int phase = 0; phase < 2; phase++)
	{
		file << type_prefix << phase_names[phase] << "_isolated_pawn_value";
		write_value(evaluator.isolated_pawn_value, phase);
	}
	for (int phase = 0; phase < 2; phase++)
	{
		file << type_prefix << phase_names[phase] << "_backward_pawn_value";
		write_value(evaluator.backward_pawn_value, phase);
	}
}

void save_values(const Evaluator& evaluator, int iteration_count, double learning_rate)
{
	OutputFileStream file(values_path);
	file << evaluator << iteration_count << learning_rate;
}

void load_values(Evaluator& evaluator, int& iteration_count, double& learning_rate)
{
	InputFileStream file(values_path);
	if (!file)
	{
		cout << "values not found, starting from scratch" << endl;
		evaluator.init();
		iteration_count = 0;
		learning_rate = 1;
		return;
	}
	cout << "using saved values" << endl;
	file >> evaluator >> iteration_count >> learning_rate;
}

constexpr double C = 0.006;

double sigmoid(double x)
{
	return 1 / (1 + exp(-C * x));
}

double sigmoid_derivative(double x)
{
	double y = sigmoid(x);
	return C * y * (1 - y);
}

void run_threads(const function<void(int, int, int)>& proc)
{
	array<thread, THREAD_COUNT> threads;
	int amount = N / THREAD_COUNT;
	for (int i = 0; i < THREAD_COUNT; i++)
	{
		threads[i] = thread(proc, i, i * amount, min((i + 1) * amount, N));
	}
	for (int i = 0; i < THREAD_COUNT; i++)
	{
		threads[i].join();
	}
}

double evaluate_all(const Evaluator& evaluator, vector<Evaluation>& evaluations)
{
	array<double, THREAD_COUNT> sums = {};
	auto proc = [&](int thread, int first, int last)
	{
		for (int i = first; i < last; i++)
		{
			evaluator.evaluate(positions[i], evaluations[i]);
			double value = evaluations[i].get_value();
			double win_probability = sigmoid(value);
			double error = results[i] - win_probability;
			sums[thread] += error * error;
		}
	};
	run_threads(proc);
	return accumulate(sums.begin(), sums.end(), 0.0);
}

void update_all(Evaluator& evaluator, const vector<Evaluation>& evaluations, double learning_rate)
{
	array<Evaluator, THREAD_COUNT> evaluators;
	auto proc = [&](int thread, int first, int last)
	{
		for (int i = first; i < last; i++)
		{
			double value = evaluations[i].get_value();
			double win_probability = sigmoid(value);
			double error = results[i] - win_probability;
			double d = 2 * error * sigmoid_derivative(value);
			evaluators[thread].update(positions[i], evaluator, evaluations[i], learning_rate * d);
		}
	};
	run_threads(proc);
	double* values = (double*)&evaluator;
	for (int i = 0; i < THREAD_COUNT; i++)
	{
		double* changes = (double*)&evaluators[i];
		for (int j = 0; j < sizeof(Evaluator) / sizeof(double); j++)
		{
			values[j] += changes[j];
		}
	}
	evaluator.finalize();
}

bool wait_for_input() 
{
	static int state = 0;
	static thread wait_thread;
	static auto wait_proc = [&]()
	{
		cin.peek();
		state = 2;
	};
	struct Waiter
	{
		Waiter()
		{
			wait_thread = thread(wait_proc);
		}

		~Waiter()
		{
			wait_thread.detach();
		}

		static void start()
		{
			static struct Waiter instance;
		}
	};
	switch (state)
	{
		case 0:
			state = 1;
			Waiter::start();
		case 1:
			return true;
		case 2:
			state = 3;
	}
	return false;
}

int main()
{
	cout << "tuner version 19 (pst, weights, mobility, pawns)" << endl;
	cout << "running on " << THREAD_COUNT << " threads" << endl;
	load_data();
	cout << "loaded " << N << " positions" << endl;
	Evaluator evaluator;
	cout << "restart? [y/n] ";
	string line;
	getline(cin, line);
	error_code ec;
	if (line.starts_with("y"))
	{
		filesystem::remove(values_path, ec);
	}
	filesystem::remove_all(output_path, ec);
	int iteration_count;
	double learning_rate;
	load_values(evaluator, iteration_count, learning_rate);
	vector<Evaluation> evaluations(N);
	double error = evaluate_all(evaluator, evaluations);
	cout << "starting total error: " << error << " mean: " << error / N << endl;
	// Even when the error already looks steady, the values are still changing quite a bit,
	// so it is worthy to keep going even with a very low learning rate.
	while (wait_for_input() && iteration_count < 12900)
	{
		Evaluator new_evaluator = evaluator;
		update_all(new_evaluator, evaluations, learning_rate);
		vector<Evaluation> new_evaluations(N);
		double new_error = evaluate_all(new_evaluator, new_evaluations);
		cout << "error after iteration " << iteration_count + 1 << ": " << new_error;
		cout << " (" << (new_error < error ? "improved" : "worsened") << ")" << endl;
		if (new_error < error)
		{
			error = new_error;
			evaluator = new_evaluator;
			evaluations = new_evaluations;
			learning_rate *= 1.2;
		}
		else
		{
			learning_rate *= 0.5;
		}
		cout << "new learning rate: " << learning_rate << endl;
		iteration_count++;
		if (iteration_count % EXPORT_INTERVAL == 0)
		{
			save_values(evaluator, iteration_count, learning_rate);
			export_values(to_string(iteration_count) + "_" + to_string(lround(error)) + ".txt", evaluator);
		}
	}
	cout << "final error: " << error << endl;
	export_values("final.txt", evaluator);
	return 0;
}
