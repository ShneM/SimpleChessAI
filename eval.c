/*
 *	Name:	Carly Deming, Trevor Sands
 *	Class:	CS331 - Intro. to Intelligent Systems
 *	File:	eval.c
 *	Info.:	Provides functions and definitions for
 *		the agent to determine a move's relative
 *		score.
 *	
 *	Credit:	Adapted from Tom Kerrigan's Simple
 *		Chess Program (TSCP.)
 *		Copyright 1997, Tom Kerrigan
 */


#include <string.h>
#include "defs.h"
#include "data.h"
#include "protos.h"

// Define bonuses and penalties to apply to scores
#define DOUBLED_PAWN_PENALTY		10
#define ISOLATED_PAWN_PENALTY		20
#define BACKWARDS_PAWN_PENALTY		8
#define PASSED_PAWN_BONUS		20
#define ROOK_SEMI_OPEN_FILE_BONUS	10
#define ROOK_OPEN_FILE_BONUS		15
#define ROOK_ON_SEVENTH_BONUS		20


/* Intrinsic piece value
   Pawn, Knight, Bishop, Rook, Queen, King */
int piece_value[6] = {
	100, 300, 300, 500, 900, 0
};

// Piece-square table modifies score based on position
int pawn_pcsq[64] = {
	  0,   0,   0,   0,   0,   0,   0,   0,
	  5,  10,  15,  20,  20,  15,  10,   5,
	  4,   8,  12,  16,  16,  12,   8,   4,
	  3,   6,   9,  12,  12,   9,   6,   3,
	  2,   4,   6,   8,   8,   6,   4,   2,
	  1,   2,   3, -10, -10,   3,   2,   1,
	  0,   0,   0, -40, -40,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0
};

int knight_pcsq[64] = {
	-10, -10, -10, -10, -10, -10, -10, -10,
	-10,   0,   0,   0,   0,   0,   0, -10,
	-10,   0,   5,   5,   5,   5,   0, -10,
	-10,   0,   5,  10,  10,   5,   0, -10,
	-10,   0,   5,  10,  10,   5,   0, -10,
	-10,   0,   5,   5,   5,   5,   0, -10,
	-10,   0,   0,   0,   0,   0,   0, -10,
	-10, -30, -10, -10, -10, -10, -30, -10
};

int bishop_pcsq[64] = {
	-10, -10, -10, -10, -10, -10, -10, -10,
	-10,   0,   0,   0,   0,   0,   0, -10,
	-10,   0,   5,   5,   5,   5,   0, -10,
	-10,   0,   5,  10,  10,   5,   0, -10,
	-10,   0,   5,  10,  10,   5,   0, -10,
	-10,   0,   5,   5,   5,   5,   0, -10,
	-10,   0,   0,   0,   0,   0,   0, -10,
	-10, -10, -20, -10, -10, -20, -10, -10
};

int king_pcsq[64] = {
	-40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40,
	-20, -20, -20, -20, -20, -20, -20, -20,
	  0,  20,  40, -20,   0, -20,  40,  20
};

int king_endgame_pcsq[64] = {
	  0,  10,  20,  30,  30,  20,  10,   0,
	 10,  20,  30,  40,  40,  30,  20,  10,
	 20,  30,  40,  50,  50,  40,  30,  20,
	 30,  40,  50,  60,  60,  50,  40,  30,
	 30,  40,  50,  60,  60,  50,  40,  30,
	 20,  30,  40,  50,  50,  40,  30,  20,
	 10,  20,  30,  40,  40,  30,  20,  10,
	  0,  10,  20,  30,  30,  20,  10,   0
};

/* The flip array is used to calculate the piece/square
   values for DARK pieces. The piece/square value of a
   LIGHT pawn is pawn_pcsq[sq] and the value of a DARK
   pawn is pawn_pcsq[flip[sq]] */
int flip[64] = {
	 56,  57,  58,  59,  60,  61,  62,  63,
	 48,  49,  50,  51,  52,  53,  54,  55,
	 40,  41,  42,  43,  44,  45,  46,  47,
	 32,  33,  34,  35,  36,  37,  38,  39,
	 24,  25,  26,  27,  28,  29,  30,  31,
	 16,  17,  18,  19,  20,  21,  22,  23,
	  8,   9,  10,  11,  12,  13,  14,  15,
	  0,   1,   2,   3,   4,   5,   6,   7
};

/* pawn_rank[x][y] is the rank of the least advanced pawn of color x on file
   y - 1. There are "buffer files" on the left and right to avoid special-case
   logic later. If there's no pawn on a rank, we pretend the pawn is
   impossibly far advanced (0 for LIGHT and 7 for DARK). This makes it easy to
   test for pawns on a rank and it simplifies some pawn evaluation code. */
int pawn_rank[2][10];
int piece_mat[2]; // Matrix of a side's piece values
int pawn_mat[2]; // Matrix of a side's collective pawn values

/* Used to evaluate the future state of the board,
   returns a relative score used to assess move strength. */
int eval()
{
	int i;  // Counter
	int f;  // File
	int score[2]; // Each side's score

	// Initialize board scores by ranking pawns
	for (i = 0; i < 10; ++i) {
		pawn_rank[LIGHT][i] = 0;
		pawn_rank[DARK][i] = 7;
	}
	// Initialize score matricies to zero
	piece_mat[LIGHT] = 0;
	piece_mat[DARK] = 0;
	pawn_mat[LIGHT] = 0;
	pawn_mat[DARK] = 0;
	
	// Scan through board and initialize pawn score matrix
	for (i = 0; i < 64; ++i) {
		if (color[i] == EMPTY)
			continue;
		if (piece[i] == PAWN) {
			pawn_mat[color[i]] += piece_value[PAWN];
			f = COL(i) + 1;  // Add 1 to account for extra file
			if (color[i] == LIGHT) {
				if (pawn_rank[LIGHT][f] < ROW(i))
					pawn_rank[LIGHT][f] = ROW(i);
			}
			else {
				if (pawn_rank[DARK][f] > ROW(i))
					pawn_rank[DARK][f] = ROW(i);
			}
		}
		else
			piece_mat[color[i]] += piece_value[piece[i]];
	}

	// Evaluate scores of each piece
	score[LIGHT] = piece_mat[LIGHT] + pawn_mat[LIGHT];
	score[DARK] = piece_mat[DARK] + pawn_mat[DARK];
	for (i = 0; i < 64; ++i) {
		if (color[i] == EMPTY)
			continue;
		if (color[i] == LIGHT) {
			switch (piece[i]) {
				case PAWN:
					score[LIGHT] += eval_light_pawn(i);
					break;
				case KNIGHT:
					score[LIGHT] += knight_pcsq[i];
					break;
				case BISHOP:
					score[LIGHT] += bishop_pcsq[i];
					break;
				case ROOK:
					if (pawn_rank[LIGHT][COL(i) + 1] == 0) {
						if (pawn_rank[DARK][COL(i) + 1] == 7)
							score[LIGHT] += ROOK_OPEN_FILE_BONUS;
						else
							score[LIGHT] += ROOK_SEMI_OPEN_FILE_BONUS;
					}
					if (ROW(i) == 1)
						score[LIGHT] += ROOK_ON_SEVENTH_BONUS;
					break;
				case KING:
					if (piece_mat[DARK] <= 1200) // Determines endgame score conditions
						score[LIGHT] += king_endgame_pcsq[i];
					else
						score[LIGHT] += eval_light_king(i);
					break;
			}
		}
		else {
			switch (piece[i]) {
				case PAWN:
					score[DARK] += eval_dark_pawn(i);
					break;
				case KNIGHT:
					score[DARK] += knight_pcsq[flip[i]];
					break;
				case BISHOP:
					score[DARK] += bishop_pcsq[flip[i]];
					break;
				case ROOK:
					if (pawn_rank[DARK][COL(i) + 1] == 7) {
						if (pawn_rank[LIGHT][COL(i) + 1] == 0)
							score[DARK] += ROOK_OPEN_FILE_BONUS;
						else
							score[DARK] += ROOK_SEMI_OPEN_FILE_BONUS;
					}
					if (ROW(i) == 6)
						score[DARK] += ROOK_ON_SEVENTH_BONUS;
					break;
				case KING:
					if (piece_mat[LIGHT] <= 1200) // Determines endgame score conditions
						score[DARK] += king_endgame_pcsq[flip[i]];
					else
						score[DARK] += eval_dark_king(i);
					break;
			}
		}
	}

	// Returns score relative to side
	if (side == LIGHT)
		return score[LIGHT] - score[DARK];
	return score[DARK] - score[LIGHT];
}

int eval_light_pawn(int sq)
{
	int r;
	int f;

	r = 0;
	f = COL(sq) + 1;

	r += pawn_pcsq[sq];

	// Penalize if a pawn occupies the space behind the pawn in question
	if (pawn_rank[LIGHT][f] > ROW(sq))
		r -= DOUBLED_PAWN_PENALTY;

	// Penalize if not flanked by friendly pawns
	if ((pawn_rank[LIGHT][f - 1] == 0) &&
			(pawn_rank[LIGHT][f + 1] == 0))
		r -= ISOLATED_PAWN_PENALTY;

	// Penalize if pawn is backwards (cannot safely advance)
	else if ((pawn_rank[LIGHT][f - 1] < ROW(sq)) &&
			(pawn_rank[LIGHT][f + 1] < ROW(sq)))
		r -= BACKWARDS_PAWN_PENALTY;

	// Add bonus if pawn is passed
	if ((pawn_rank[DARK][f - 1] >= ROW(sq)) &&
			(pawn_rank[DARK][f] >= ROW(sq)) &&
			(pawn_rank[DARK][f + 1] >= ROW(sq)))
		r += (7 - ROW(sq)) * PASSED_PAWN_BONUS;

	return r;
}

int eval_dark_pawn(int sq)
{
	int r;
	int f;

	r = 0;
	f = COL(sq) + 1;

	r += pawn_pcsq[flip[sq]];

	// Penalize if a pawn occupies the space behind the pawn in question
	if (pawn_rank[DARK][f] < ROW(sq))
		r -= DOUBLED_PAWN_PENALTY;

	// Penalize if not flanked by friendly pawns
	if ((pawn_rank[DARK][f - 1] == 7) &&
			(pawn_rank[DARK][f + 1] == 7))
		r -= ISOLATED_PAWN_PENALTY;

	// Penalize if pawn is backwards (cannot safely advance)
	else if ((pawn_rank[DARK][f - 1] > ROW(sq)) &&
			(pawn_rank[DARK][f + 1] > ROW(sq)))
		r -= BACKWARDS_PAWN_PENALTY;

	// Add bonus if pawn is passed
	if ((pawn_rank[LIGHT][f - 1] <= ROW(sq)) &&
			(pawn_rank[LIGHT][f] <= ROW(sq)) &&
			(pawn_rank[LIGHT][f + 1] <= ROW(sq)))
		r += ROW(sq) * PASSED_PAWN_BONUS;

	return r;
}

int eval_light_king(int sq)
{
	int r;
	int i;

	r = king_pcsq[sq];

	// Evaluates safety of king if castled
	if (COL(sq) < 3) {
		r += eval_lkp(1);
		r += eval_lkp(2);
		r += eval_lkp(3) / 2;
	}
	else if (COL(sq) > 4) {
		r += eval_lkp(8);
		r += eval_lkp(7);
		r += eval_lkp(6) / 2;
	}
	// Penalty added if open files around king
	else {
		for (i = COL(sq); i <= COL(sq) + 2; ++i)
			if ((pawn_rank[LIGHT][i] == 0) &&
					(pawn_rank[DARK][i] == 7))
				r -= 10;
	}
	
	// Scale score based on number of opposing pieces
	r *= piece_mat[DARK];
	r /= 3100;

	return r;
}

// Modulates score based on state of the light-side pawn on rank f
int eval_lkp(int f)
{
	int r = 0;

	if (pawn_rank[LIGHT][f] == 6);  // Pawn occupies rank f
	else if (pawn_rank[LIGHT][f] == 5)
		r -= 10; // Pawn has moved 1 square
	else if (pawn_rank[LIGHT][f] != 0)
		r -= 20; // Pawn has moved > 1 square
	else
		r -= 25; // No pawn present

	if (pawn_rank[DARK][f] == 7)
		r -= 15; // No enemy pawn present
	else if (pawn_rank[DARK][f] == 5)
		r -= 10; // Enemy pawn on 3rd rank
	else if (pawn_rank[DARK][f] == 4)
		r -= 5;  // Enemy pawn on 4th rank

	return r;
}

int eval_dark_king(int sq)
{
	int r;
	int i;

	// Evaluates safety of king if castled
	r = king_pcsq[flip[sq]];
	if (COL(sq) < 3) {
		r += eval_dkp(1);
		r += eval_dkp(2);
		r += eval_dkp(3) / 2;
	}
	else if (COL(sq) > 4) {
		r += eval_dkp(8);
		r += eval_dkp(7);
		r += eval_dkp(6) / 2;
	}
	// Penalty added if open files around king
	else {
		for (i = COL(sq); i <= COL(sq) + 2; ++i)
			if ((pawn_rank[LIGHT][i] == 0) &&
					(pawn_rank[DARK][i] == 7))
				r -= 10;
	}
	
	// Scale score based on number of opposing pieces
	r *= piece_mat[LIGHT];
	r /= 3100;
	return r;
}

// Modulates score based on state of the dark-side pawn on rank f
int eval_dkp(int f)
{
	int r = 0;

	if (pawn_rank[DARK][f] == 1); // Pawn occupies rank f
	else if (pawn_rank[DARK][f] == 2)
		r -= 10; // Pawn has moved 1 square
	else if (pawn_rank[DARK][f] != 7)
		r -= 20; // Pawn has moved > 1 square
	else
		r -= 25; // No pawn present

	if (pawn_rank[LIGHT][f] == 0)
		r -= 15; // No enemy pawn present
	else if (pawn_rank[LIGHT][f] == 2)
		r -= 10; // Enemy pawn on 3rd rank
	else if (pawn_rank[LIGHT][f] == 3)
		r -= 5; // Enemy pawn on 4th rank

	return r;
}
