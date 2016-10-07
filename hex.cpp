/*
      H    H  EEEEEE  X     X
      H    H  E        X   X
      H    H  E         X X
      HHHHHH  EEEEEE     X
      H    H  E         X X
      H    H  E        X   X
      H    H  EEEEEE  X     X
      
      [October 2016]


Compile options:
g++ -Wall -c "%f" -std=c++11

Link options (with local MinGW implementation):
g++ -Wall -O3 -o "%e" "%f" -s -std=c++11

Link options (for distribution):
g++ -Wall -O3 -o "%e" "%f" -s -std=c++11 -static-libgcc -static-libstdc++ -static -lwinpthread

*/

#include <vector>
#include <queue>
#include <set>
#include <iostream>
#include <string>
#include <thread>
#include <future>
#include <ctime>
#include <algorithm>  // random_shuffle()
#include <numeric>    // accumulate()
#include <conio.h>    // _getch()
#include <windows.h>  // Windows specific display

enum class piece:char {EMPTY, X, O};
// X must link East and West
// O must link North and South

// unsigned char      -> up to 255 (board size up to 15x15 included)
// unsigned short int -> up to 65535 (larger board size)
typedef unsigned char nodenumber;
#define MAX_BOARD_SIZE 15

nodenumber BOARD_SIZE = 11; // size of the Hex board
bool USE_PIE_RULE = true; // use pie rule?
bool PIE_RULE_SYMMETRY = true; // true: use symmetric move for pie rule; false: use exact same move
piece FIRST_PLAYER = piece::X; // first player makes first move
int NUMBER_PROCESSOR = 2; // number of processor to use for parallel threading
int NUMBER_MONTE_CARLO_PATH = 3000 / NUMBER_PROCESSOR; // number of Monte Carlo path per 1 thread

nodenumber BOARD_DIMENSION = BOARD_SIZE * BOARD_SIZE; // total number of squares
const std::string LEFT_MARGIN = "   "; // left margin for the Hex board display

class node {
public:
	// constructor
	node(const std::vector<nodenumber> neighbors, const piece owner)
	: _neighbors(neighbors), _owner(owner) {}
	// getters
	inline std::vector<nodenumber> get_neighbors() const {return _neighbors;}
	inline piece get_owner() const {return _owner;}
	// setters
	inline void set_neighbors(const nodenumber squarenum, std::vector<nodenumber> neighbors) {_neighbors = neighbors;}
	inline void set_owner(const piece owner) {_owner = owner;}
private:
	std::vector<nodenumber> _neighbors; // all the squares adjacent to this square
	piece _owner; // piece played on this square
};

inline bool in_board(const nodenumber square_num) {
	// check if square_num is a legal square (on the board)
	return (square_num >= 0 and square_num < BOARD_DIMENSION);
}

inline nodenumber coordinates_to_node(const nodenumber i, const nodenumber j) {
	// from row,column (i,j) return the associated square number
	// return BOARD_DIMENSION if (i,j) is not a legal square (not on the board)
	if (i >= 0 and i < BOARD_SIZE and j >= 0 and j < BOARD_SIZE) {
		return (i * BOARD_SIZE + j); 
	} else {
		return BOARD_DIMENSION;
	}
}

inline nodenumber transpose_node(const nodenumber n) {
	// transpose node (by symmetry):
	// node -> (i,j) -> (j,i) transposition -> node transposed
	// return BOARD_DIMENSION if (i,j) is not a legal square (not on the board)
	if (in_board(n)) {
		const nodenumber row = n / BOARD_SIZE;
		const nodenumber col = n - row * BOARD_SIZE;
		return coordinates_to_node(col, row);
	} else {
		return BOARD_DIMENSION;
	}
}

class hexGraph {
public:
	// constructor
	hexGraph() {
		for(nodenumber i = 0; i < BOARD_SIZE; i++) {
			for(nodenumber j = 0; j < BOARD_SIZE; j++) {
				// initializing the node number coordinates_to_node(i, j)
				const node n = node(list_neighbors(i, j), piece::EMPTY);
				_hexboard.push_back(n);
			}
		}
	}
	// helper function prototypes
	inline bool check_move(const nodenumber square_num) const;
	inline bool make_move(const nodenumber square_num, const piece p);
	inline void unmake_move(const nodenumber square_num);
	bool is_winner(const piece p);
	void score_move(const int rnd, std::promise<double> *result);
	std::set<nodenumber> victory_path(const piece p);
	void print(const std::set<nodenumber> selection, const piece p) const;
private:
	std::vector<node> _hexboard; // vector of all the squares of the board
	// setter
	inline void set_owner(const nodenumber square_num, const piece p) {_hexboard[square_num].set_owner(p);}
	// getters
	inline piece get_owner(const nodenumber square_num) const {return get_node(square_num).get_owner();}
	inline node get_node(const nodenumber square_num) const {return _hexboard[square_num];}
	// helper function prototypes
	std::vector<nodenumber> list_neighbors(const nodenumber i, const nodenumber j) const;
	std::queue<nodenumber> get_node1(const piece p);
	std::set<nodenumber> get_node2(const piece p);
	bool find_path(std::queue<nodenumber> Q, std::set<nodenumber> node_to, const piece p) const;
	std::set<nodenumber> find_victory_path(std::queue<nodenumber> Q, std::set<nodenumber> node_to, const piece p) const;
	inline void print_piece(const HANDLE std_output, const unsigned int background_color, const unsigned int text_color, const char text) const;
};

std::vector<nodenumber> hexGraph::list_neighbors(const nodenumber i, const nodenumber j) const {
	// return the list of neighbors for the square of coordinates (i,j)
	nodenumber num;
	std::vector<nodenumber> list;
	num = coordinates_to_node(i - 1, j);     if (in_board(num)) {list.push_back(num);}
	num = coordinates_to_node(i - 1, j + 1); if (in_board(num)) {list.push_back(num);}
	num = coordinates_to_node(i    , j - 1); if (in_board(num)) {list.push_back(num);}
	num = coordinates_to_node(i    , j + 1); if (in_board(num)) {list.push_back(num);}
	num = coordinates_to_node(i + 1, j - 1); if (in_board(num)) {list.push_back(num);}
	num = coordinates_to_node(i + 1, j);     if (in_board(num)) {list.push_back(num);}
	return list;
}

inline bool hexGraph::check_move(const nodenumber square_num) const {
	// check if a move if possible
	// (return false if the square was occupied)
	return (get_owner(square_num) == piece::EMPTY);
}

inline bool hexGraph::make_move(const nodenumber square_num, const piece p) {
	// make a move on the board
	// return false if the square was occupied, true otherwise
	if (check_move(square_num)) {
		set_owner(square_num, p);
		return true;
	} else {
		return false;
	}
}

inline void hexGraph::unmake_move(const nodenumber square_num) {
	// unmake a move
	set_owner(square_num, piece::EMPTY);
}

bool hexGraph::find_path(std::queue<nodenumber> Q, const std::set<nodenumber> node_set, const piece p) const {
	// return true is there is a path from Q to node_set
	if (Q.empty() or node_set.empty()) {return false;}
	bool visited[BOARD_DIMENSION] = {false};
	bool target[BOARD_DIMENSION] = {false};
	for (nodenumber n : node_set) {
		target[n] = true;
	}
	while (not Q.empty()) {
		const nodenumber n = Q.front();
		if (target[n]) {return true;}
		visited[n] = true;
		Q.pop();
		for (nodenumber neighbor : get_node(n).get_neighbors()) {
			if (get_node(neighbor).get_owner() == p and not visited[neighbor]) {Q.push(neighbor);}
		}
 	}
	return false;
}

std::set<nodenumber> hexGraph::find_victory_path(std::queue<nodenumber> Q, const std::set<nodenumber> node_set, const piece p) const {
	// return the path from Q to node_set
	// similar to find_path() but find_path() needs to be optimized for speed so cannot return victory path
	std::set<nodenumber> path;
	bool visited[BOARD_DIMENSION] = {false};
	bool target[BOARD_DIMENSION] = {false};
	nodenumber parent_node[BOARD_DIMENSION];
	for (nodenumber i = 0; i < BOARD_DIMENSION; i++) {
		 parent_node[i] = BOARD_DIMENSION;
	 }
	for (nodenumber n : node_set) {
		target[n] = true;
	}
	while (not Q.empty()) {
		nodenumber n = Q.front();
		if (target[n]) {
			while (true) {
				if (n == BOARD_DIMENSION) {break;}
				path.insert(n);
				n = parent_node[n];
			}
			return path;
		}
		visited[n] = true;
		Q.pop();
		for (nodenumber neighbor : get_node(n).get_neighbors()) {
			if (get_node(neighbor).get_owner() == p and not visited[neighbor]) {
				parent_node[neighbor] = n;
				Q.push(neighbor);
			}
		}
 	}
	return path;
}
std::queue<nodenumber> hexGraph::get_node1(const piece p) {
	std::queue<nodenumber> node1;
	if (p == piece::O) {
		// O must link North and South
		// North: nodes are 1, 2, ..., BOARD_SIZE - 1
		for (nodenumber i = 0; i < BOARD_SIZE; i++) {
			if (get_owner(i) == p) {node1.push(i);}
		}
	} else if (p == piece::X) {
		// X must link East and West
		// East: nodes are 0 * BOARD_SIZE, 1 * BOARD_SIZE, ..., (BOARD_SIZE - 1) * BOARD_SIZE
		for (nodenumber i = 0; i < BOARD_SIZE; i++) {
			const nodenumber n = i * BOARD_SIZE;
			if (get_owner(n) == p) {node1.push(n);}
		}
	}
	return node1;
}

std::set<nodenumber> hexGraph::get_node2(const piece p) {
	std::set<nodenumber> node2;
	if (p == piece::O) {
		// O must link North and South
		// South: nodes are (BOARD_SIZE - 1) * BOARD_SIZE, (BOARD_SIZE - 1) * BOARD_SIZE + 1, ..., (BOARD_SIZE - 1) * BOARD_SIZE + (BOARD_SIZE - 2)
		for (nodenumber i = 0; i < BOARD_SIZE; i++) {
			const nodenumber n = BOARD_DIMENSION - (i + 1);
			if (get_owner(n) == p) {node2.insert(n);}
		}
	} else if (p == piece::X) {
		// X must link East and West
		// West: nodes are 1 * BOARD_SIZE - 1, 2 * BOARD_SIZE - 1, ..., BOARD_SIZE * BOARD_SIZE - 1
		for (nodenumber i = 0; i < BOARD_SIZE; i++) {
			const nodenumber n = (i + 1) * BOARD_SIZE - 1;
			if (get_owner(n) == p) {node2.insert(n);}
		}
	}
	return node2;
}

bool hexGraph::is_winner(const piece p) {
	// return true if p is the winner
	std::queue<nodenumber> node1 = get_node1(p);
	std::set<nodenumber> node2 = get_node2(p);
	return find_path(node1, node2, p);
}

std::set<nodenumber> hexGraph::victory_path(const piece p) {
	// return the winning path
	std::queue<nodenumber> node1 = get_node1(p);
	std::set<nodenumber> node2 = get_node2(p);
	return find_victory_path(node1, node2, p);
}

// colors see: http://www.cplusplus.com/articles/2ywTURfi/
#define color_black 0
#define color_dark_blue 1
#define color_dark_green 2
#define color_dark_aqua 3
#define color_dark_cyan 3
#define color_dark_red 4
#define color_dark_purple 5
#define color_dark_pink 5
#define color_dark_magenta 5
#define color_dark_yellow 6
#define color_dark_white 7
#define color_gray 8
#define color_blue 9
#define color_green 10
#define color_aqua 11
#define color_cyan 11
#define color_red 12
#define color_purple 13
#define color_pink 13
#define color_magenta 13
#define color_yellow 14
#define color_white 15
	
unsigned int COLOR_X = color_white;
unsigned int COLOR_O = color_red;
unsigned int BACKGROUND_SELECT = color_gray;
const unsigned int BACKGROUND_DEFAULT = color_black;

inline void hexGraph::print_piece(const HANDLE std_output, const unsigned int background_color, const unsigned int text_color, const char text) const {
	// print text in color
	unsigned short wAttributes = (background_color << 4) | text_color;
	SetConsoleTextAttribute(std_output, wAttributes);
	std::cout << text;
	// restore white on black
	wAttributes = (color_black << 4) | color_white;
	SetConsoleTextAttribute(std_output, wAttributes);
}

void hexGraph::print(const std::set<nodenumber> selection = {}, const piece winner = piece::EMPTY) const {
	// print the board (Microsoft Windows)
	// if selection < BOARD_DIMENSION, the corresponding square is highlighted (for all members of the set)
	const HANDLE std_output = GetStdHandle(STD_OUTPUT_HANDLE);
	const unsigned int background_X = (winner == piece::X) ? BACKGROUND_SELECT : BACKGROUND_DEFAULT;
	const unsigned int background_O = (winner == piece::O) ? BACKGROUND_SELECT : BACKGROUND_DEFAULT;
	// set cursor position to (0,0)
	// equivalent of system("cls"); with better graphic effect (no flickering)
	// but requires the use of functions erase_end_line() and erase_bottom()
	const COORD coord = {0, 0};
    SetConsoleCursorPosition(std_output, coord);
	// set windows titel, default color & no cursor (Microsoft Windows)
	SetConsoleTitle("Hex by Arnaud");
	const CONSOLE_CURSOR_INFO cursorInfo = {1, false};
    SetConsoleCursorInfo(std_output, &cursorInfo);
	const unsigned int textcol = color_white;
	const unsigned int backcol = BACKGROUND_DEFAULT;
	const unsigned short wAttributes = (backcol << 4) | textcol;
	SetConsoleTextAttribute(std_output, wAttributes);
	std::cout << std::endl;
	std::cout << LEFT_MARGIN << "  ";
	for(nodenumber i = 0; i < BOARD_SIZE; i++) {
		std::cout << ' ';
		print_piece(std_output, background_O, COLOR_O, 'O');
	}
	std::cout << std::endl;
	for(nodenumber i = 0; i < BOARD_SIZE; i++) {
		std::cout << LEFT_MARGIN << std::string(i + 1, ' ');
		std::cout << ' ';
		print_piece(std_output, background_X, COLOR_X, 'X');
		for(nodenumber j = 0; j < BOARD_SIZE; j++) {
			char owner = '.';
			unsigned int col_txt = textcol;
			unsigned int col_bck = backcol;
			if (get_owner(coordinates_to_node(i, j)) == piece::X) {owner = 'X'; col_txt = COLOR_X;}
			if (get_owner(coordinates_to_node(i, j)) == piece::O) {owner = 'O'; col_txt = COLOR_O;}	
			std::cout << ' ';
			if (selection.find(coordinates_to_node(i, j)) != selection.end()) {
				col_bck = BACKGROUND_SELECT;
			}
			print_piece(std_output, col_bck, col_txt, owner);
		}
		std::cout << ' ';
		print_piece(std_output, background_X, COLOR_X, 'X');
		std::cout << std::endl;
	}
	std::cout << LEFT_MARGIN << "  ";
	std::cout << std::string(BOARD_SIZE + 1, ' ');
	for(nodenumber i = 0; i < BOARD_SIZE; i++) {
		std::cout << ' ';
		print_piece(std_output, background_O, COLOR_O, 'O');
	}
	std::cout << std::endl;
	std::cout << std::endl;
}

void hexGraph::score_move(const int rnd, std::promise<double> *result) {
	// return the score assessed
	// computer has just played, so it is player's turn
	srand(rnd);
	std::vector<nodenumber> moves;
	// all possible upcoming moves
	for (nodenumber i = 0; i < BOARD_DIMENSION; i++) {
		if (_hexboard[i].get_owner() == piece::EMPTY) {
			moves.push_back(i);
		}
	}
	int count_win = 0;
	for (int i = 0; i < NUMBER_MONTE_CARLO_PATH; i++) {
		// random play: allocate X, O, ..., X, O, ...
		// to the list of possible moves randomly shuffled
		std::random_shuffle(moves.begin(), moves.end());
		bool player = true;
		for (nodenumber move : moves) {
			if (player) {
				set_owner(move, piece::X);
			} else {
				set_owner(move, piece::O);
			}
			player = not player;
		}
		if (is_winner(piece::O)) {
			count_win++;
		}
	}
	for (nodenumber move : moves) {
		set_owner(move, piece::EMPTY);
	}
	result->set_value(1. * count_win / NUMBER_MONTE_CARLO_PATH);
}

double assess_move(std::vector<hexGraph> &hex) {
	// definition of promise/future variables
	std::promise<double> res_before[NUMBER_PROCESSOR];
	std::future<double> res_after[NUMBER_PROCESSOR];
	for (int i = 0; i < NUMBER_PROCESSOR; i++) {
		res_after[i] = res_before[i].get_future();
	}
	std::thread monte_carlo_thread[NUMBER_PROCESSOR];
	// definition of threads
	for (int i = 0; i < NUMBER_PROCESSOR; i++) {
		// it is critical to have different seeds for each thread
		const int rnd_seed = (1. + i / 10.) * time(0);
		monte_carlo_thread[i] = std::thread(&hexGraph::score_move, hex[i], rnd_seed, &res_before[i]);
	}
	for (int i = 0; i < NUMBER_PROCESSOR; i++) {
		monte_carlo_thread[i].join();
	}
	std::vector<double> out;
	for (int i = 0; i < NUMBER_PROCESSOR; i++) {
		out.push_back(res_after[i].get());
	}
	// averaging of results
	const double average = std::accumulate(out.begin(), out.end(), 0.0) / NUMBER_PROCESSOR;
	return average;
}

inline void parallel_make_move(std::vector<hexGraph> &hex, const nodenumber n, const piece p) {
	// make a move on board
	for (int i = 0; i < NUMBER_PROCESSOR; i++) {
		hex[i].make_move(n, p);
	}
}

inline void parallel_unmake_move(std::vector<hexGraph> &hex, const nodenumber n) {
	// unmake a move on board
	for (int i = 0; i < NUMBER_PROCESSOR; i++) {
		hex[i].unmake_move(n);
	}
}

#define LONGEST_LINE 65

inline void erase_end_line() {
	// print LONGEST_LINE spaces and return to the beginning of the line
	const HANDLE std_output = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
    GetConsoleScreenBufferInfo(std_output, &csbiInfo);
	const int x = csbiInfo.dwCursorPosition.X;
	if (x < LONGEST_LINE) {
		std::cout << std::string(LONGEST_LINE - x, ' ');
	}
	std::cout << std::endl;
}

inline void erase_bottom() {
	// prints 3 blank lines (LONGEST_LINE spaces) to make sure bottom of the screen is erased
	for (int i = 0; i < 3; i++) {
		std::cout << std::string(LONGEST_LINE, ' ') << std::endl;
	}
}

piece play_player_turn(std::vector<hexGraph> &hex, const nodenumber move_O, nodenumber &move_X, const double score_O, bool &pie_rule, bool &pie_rule_was_used, const double time_O) {
	// Player [X]'s turn
	if (move_O < BOARD_DIMENSION) {
		move_X = move_O; // cursor placed on computer's move
	}
	while (true) {
		hex[0].print({move_X});
		if (move_O < BOARD_DIMENSION) {
			if (pie_rule_was_used) {
				std::cout << " Pie rule!";
				erase_end_line();
			}
			std::cout << " Computer [O] just played";
			std::cout << " (score = " << (int) (1000. * score_O) / 10. << "%)";
			std::cout << " in " << (int) (10. * time_O) / 10. << " seconds";
			erase_end_line();
		}
		std::cout << " Player [X]: use <Arrows> to move, <Enter> to select";
		erase_end_line();
		erase_bottom();
		const int keyboard_input = _getch();
		bool move_done = false;
		switch (keyboard_input) {
			case 75: // Left
				if (move_X % BOARD_SIZE != 0) { // current position is not left of board
					move_X--;
				}
				break;
			case 77: // Right
				if (move_X % BOARD_SIZE != BOARD_SIZE - 1) { // current position is not right of board
					move_X++;
				}
				break;
			case 72: // Up
				if (move_X >= BOARD_SIZE) { // current position is not top of board
					move_X = move_X - BOARD_SIZE;
				}
				break;
			case 80: // Down
				if (move_X < BOARD_DIMENSION - BOARD_SIZE) { // current position is not bottom of board
					move_X = move_X + BOARD_SIZE;
				}
				break;
			case 13: // Enter
				if (hex[0].check_move(move_X) or (pie_rule and move_X == move_O)) {
					move_done = true;
				}
				break;
			default: // other
				break;
		}
		if (move_done) {
			if (pie_rule_was_used) {
				pie_rule_was_used = false;
			}
			if (pie_rule and move_X == move_O) {
				if (PIE_RULE_SYMMETRY) {
					move_X = transpose_node(move_X);
				}
				parallel_unmake_move(hex, move_O);
				pie_rule_was_used = true;
			}
			break;
		}
	}
	parallel_make_move(hex, move_X, piece::X);
	if (hex[0].is_winner(piece::X)) {
		return piece::X;
	} else {
		return piece::EMPTY;
	}
}

piece play_computer_turn(std::vector<hexGraph> &hex, nodenumber &move_O, const nodenumber move_X, double &score_O, bool &pie_rule, bool &pie_rule_was_used, const bool play_average) {
	nodenumber best_move = BOARD_DIMENSION;
	nodenumber worse_move = BOARD_DIMENSION;
	double worse_score = 1.1; // maximum score possible is 1.0
	score_O = -1.;
	// assess all possible moves
	for (nodenumber square_num = 0; square_num < BOARD_DIMENSION; square_num++) {
		if (hex[0].check_move(square_num)) {
			parallel_make_move(hex, square_num, piece::O);
			hex[0].print({square_num});
			if (move_X < BOARD_DIMENSION) {
				if (pie_rule_was_used) {
					std::cout << " Pie rule!";
					erase_end_line();
				}
				std::cout << " Player [X] just played";
				erase_end_line();
			}
			std::cout << " Computer [O] is assessing move...";
			erase_end_line();
			erase_bottom();
			const double s = assess_move(hex);
			 // select largest score square, or for pie rule
			 // the square of largest score below 0.5 and
			 // if it doesn't exist, the square of lowest score
			if (s > score_O and not (play_average and s > 0.5)) {
				score_O = s;
				best_move = square_num;
			}
			if (s < worse_score) {
				worse_score = s;
				worse_move = square_num;
			}
			parallel_unmake_move(hex, square_num);
		}
	}
	if (pie_rule_was_used) {
		pie_rule_was_used = false;
	}
	// assess pie rule
	if (pie_rule) {
		// O move to move_X
		parallel_unmake_move(hex, move_X);
		nodenumber move_X_symmetric;
		if (PIE_RULE_SYMMETRY) {
			move_X_symmetric = transpose_node(move_X);
		} else {
			move_X_symmetric = move_X;
		}
		parallel_make_move(hex, move_X_symmetric, piece::O);
		hex[0].print({move_X_symmetric});		
		std::cout << " Player [X] just played";
		erase_end_line();
		std::cout << " Computer [O] is assessing pie rule move...";
		erase_end_line();
		erase_bottom();
		const double s = assess_move(hex);
		parallel_unmake_move(hex, move_X_symmetric);
		if (s > score_O) {
			score_O = s;
			best_move = move_X_symmetric;
			pie_rule_was_used = true;
		} else {
			parallel_make_move(hex, move_X, piece::X);
		}
	}
	if (best_move < BOARD_DIMENSION) {
		move_O = best_move;
	} else {
		move_O = worse_move;
		score_O = worse_score;
	}
	parallel_make_move(hex, move_O, piece::O);
	if (hex[0].is_winner(piece::O)) {
		return piece::O;
	} else {
		return piece::EMPTY;
	}
}

void init_global_variables(int argc, char ** argv) {
	// re-initialize global variables from command line
	if (argc >= 2) {BOARD_SIZE = (int) std::atoi(argv[1]);}
	if (BOARD_SIZE < 3) {BOARD_SIZE = 3;}
	if (argc >= 3) {const std::string str(argv[2]); USE_PIE_RULE = (str != "NO");}
	if (argc >= 4) {const std::string str(argv[3]); PIE_RULE_SYMMETRY = (str != "NO");}
	if (argc >= 5) {const std::string str(argv[4]); FIRST_PLAYER = (str == "O") ? piece::O : piece::X;}
	if (argc >= 6) {NUMBER_MONTE_CARLO_PATH = std::atoi(argv[5]);}
	if (NUMBER_MONTE_CARLO_PATH < 100) {NUMBER_MONTE_CARLO_PATH = 100;}
	if (argc >= 7) {NUMBER_PROCESSOR = std::atoi(argv[6]);}
	NUMBER_MONTE_CARLO_PATH = NUMBER_MONTE_CARLO_PATH / NUMBER_PROCESSOR;
	if (NUMBER_PROCESSOR < 1) {NUMBER_PROCESSOR = 1;}
	if (argc >= 8) {COLOR_X = std::atoi(argv[7]);}
	if (COLOR_X < color_black or COLOR_X > color_white) {COLOR_X = color_white;}
	if (argc >= 9) {COLOR_O = std::atoi(argv[8]);}
	if (COLOR_O < color_black or COLOR_O > color_white) {COLOR_O = color_red;}
	if (argc >= 10) {BACKGROUND_SELECT = std::atoi(argv[9]);}
	if (BACKGROUND_SELECT < color_black or BACKGROUND_SELECT > color_white) {BACKGROUND_SELECT = color_gray;}
	bool DISPLAY_MODIFS = true;
	if (argc >= 11) {const std::string str(argv[10]); DISPLAY_MODIFS = (str != "NO");}
	if (DISPLAY_MODIFS) {
		std::cout << "Per command line, Hex will use:\n\n";
		std::cout << "Board size           = " << (int) BOARD_SIZE << std::endl;
		std::cout << "Pie rule enforced    = " << ((USE_PIE_RULE) ? "YES" : "NO") << std::endl;
		std::cout << "Pie rule symmetry    = " << ((PIE_RULE_SYMMETRY) ? "YES" : "NO") << std::endl;
		std::cout << "First move           = " << ((FIRST_PLAYER == piece::X) ? 'X' : 'O') << std::endl;
		std::cout << "Monte Carlo paths    = " << NUMBER_MONTE_CARLO_PATH << " per processor" << std::endl;
		std::cout << "Number of processors = " << NUMBER_PROCESSOR << std::endl;
		std::cout << "Player color [X]     = " << COLOR_X << std::endl;
		std::cout << "Computer color [O]   = " << COLOR_O << std::endl;
		std::cout << "Selection color      = " << BACKGROUND_SELECT << std::endl;
		std::cout << "\nPress any key to continue..." << std::endl;
		_getch();
		system("cls");
	}
}

int main(int argc, char ** argv){
	// update defaults
	// argv[0] is "<path>\hex.exe"
	if (argc >= 2) {
		init_global_variables(argc, argv);
		BOARD_DIMENSION = BOARD_SIZE * BOARD_SIZE;
	}
	if (BOARD_SIZE > MAX_BOARD_SIZE) {
		std::cout << "The maximum board size for this version of Hex is " << MAX_BOARD_SIZE << ".\n\n";
		std::cout << "Change " << '"' << "typedef unsigned char nodenumber;" << '"' << std::endl;;
		std::cout << "To     " << '"' << "typedef unsigned short int nodenumber;" << '"' << std::endl;
		std::cout << std::endl;
		std::cout << "Press any key to continue...";
		_getch();
		return 0;
	}
	while (true) {
		// create parallel board games, all equal, for parallel thread processing
		std::vector<hexGraph> hex;
		for (int i = 0; i < NUMBER_PROCESSOR; i++) {
			hex.push_back(hexGraph());
		}
		nodenumber move_X;
		if (FIRST_PLAYER == piece::X ) {
			move_X = BOARD_DIMENSION / 2; // cursor in the middle if player [X] starts
		} else {
			move_X = BOARD_DIMENSION;
		}
		nodenumber move_O = BOARD_DIMENSION;
		double score_O = -1.;
		float time_O = 0.;
		bool first_move = true;
		bool pie_rule = USE_PIE_RULE;
		bool pie_rule_was_used = false;
		piece winner = piece::EMPTY;
		while (true) {
			// Player [X]'s turn
			if (not (FIRST_PLAYER == piece::O and first_move)) {
				pie_rule = USE_PIE_RULE and (move_X == BOARD_DIMENSION) and (move_O < BOARD_DIMENSION);
				winner = play_player_turn(hex, move_O, move_X, score_O, pie_rule, pie_rule_was_used, time_O);
				if (winner == piece::X) {
					break;
				}
			} else {
				move_X = BOARD_DIMENSION;
			}
			// Computer [O]'s turn
			const clock_t time0 = clock();
			pie_rule = USE_PIE_RULE and (move_X < BOARD_DIMENSION) and (move_O == BOARD_DIMENSION);
			const bool play_average = USE_PIE_RULE and (move_X == BOARD_DIMENSION) and (move_O == BOARD_DIMENSION);
			winner = play_computer_turn(hex, move_O, move_X, score_O, pie_rule, pie_rule_was_used, play_average);
			if (winner == piece::O) {
				break;
			}
			first_move = false;
			time_O = float(clock() - time0) / 1000.;
		}
		hex[0].print(hex[0].victory_path(winner), winner);
		std::cout << " ******** GAME OVER ********";
		erase_end_line();
		if (winner == piece::O) {
			std::cout << " Computer [O]";
		} else {
			std::cout << " Player [X]";
		}
		std::cout << " wins the game!";
		erase_end_line(); 
		erase_end_line();
		std::cout << " press <Enter> to start a new game (loser starts)";
		erase_end_line();
		std::cout << " press <Esc> to quit the game...";
		erase_end_line();
		int keyboard_input;
		while (true) {
			keyboard_input = _getch();
			if (keyboard_input == 13 or keyboard_input == 27 ) { // Enter or Esc
				break;
			}
		}
		if (keyboard_input == 27 ) { // Esc
			break;
		}
		if (winner == piece::O) {
			FIRST_PLAYER = piece::X;
		} else {
			FIRST_PLAYER = piece::O;
		}
	}
	return 0;
}
