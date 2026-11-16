#include <limits.h>

#include "moves.h"
#include "pairs.h"
#include "pdb.h"
#include "rubik.h"

#define CUBIES 7
#define MAX_DEPTH 11

static const char *const move_names[9] = {"R",  "R2", "R'", "B", "B2",
                                          "B'", "D",  "D2", "D'"};

static const uint8_t move_face[9] = {0, 0, 0, 1, 1, 1, 2, 2, 2};
static const uint8_t move_turns[9] = {1, 2, 3, 1, 2, 3, 1, 2, 3};

static int rank_permutation(const uint8_t *perm)
{
    uint8_t available[CUBIES] = {0, 1, 2, 3, 4, 5, 6};
    int rank = 0;

    for (int i = 0; i < CUBIES; i++) {
        int index = 0;
        while (available[index] != perm[i])
            index++;
        for (int j = index; j < CUBIES - 1 - i; j++)
            available[j] = available[j + 1];
        int fact = 1;
        for (int j = 1; j <= CUBIES - 1 - i; j++)
            fact *= j;
        rank += index * fact;
    }
    return rank;
}

static int rank_orientation(const uint8_t *o)
{
    int rank = 0;

    for (int i = 0; i < CUBIES - 1; i++)
        rank = rank * 3 + o[i];
    return rank;
}

static int heuristic(int perm, int ori, int q0, int q1)
{
    int row = ori * 49;
    int h = permutation_pdb[perm];
    int h0 = pair_pdb[0][row + q0];
    int h1 = pair_pdb[1][row + q1];

    if (h0 > h)
        h = h0;
    return h1 > h ? h1 : h;
}

static int solve(int start_perm,
                 int start_ori,
                 int start_q0,
                 int start_q1,
                 uint8_t path[MAX_DEPTH])
{
    uint16_t perm[MAX_DEPTH + 1], ori[MAX_DEPTH + 1];
    uint8_t q0[MAX_DEPTH + 1], q1[MAX_DEPTH + 1];
    uint8_t next_moves[MAX_DEPTH + 1];
    int bound = heuristic(start_perm, start_ori, start_q0, start_q1);

    perm[0] = (uint16_t) start_perm;
    ori[0] = (uint16_t) start_ori;
    q0[0] = (uint8_t) start_q0;
    q1[0] = (uint8_t) start_q1;
    for (;;) {
        int depth = 0;
        int next_bound = INT_MAX;

        next_moves[0] = 0;
        for (;;) {
            int f = depth +
                    heuristic(perm[depth], ori[depth], q0[depth], q1[depth]);

            if (f > bound) {
                if (f < next_bound)
                    next_bound = f;
                if (depth == 0)
                    break;
                depth--;
                continue;
            }
            if (perm[depth] == 0 && ori[depth] == 0)
                return depth;
            if (next_moves[depth] >= 9 || depth >= MAX_DEPTH) {
                if (depth == 0)
                    break;
                depth--;
                continue;
            }

            int move = next_moves[depth]++;
            int face = move_face[move];

            if (depth > 0 && face == move_face[path[depth - 1]])
                continue;

            int p = perm[depth], o = ori[depth];
            int a = q0[depth], b = q1[depth];
            for (int t = 0; t < move_turns[move]; t++) {
                p = perm_turn[face][p];
                o = ori_turn[face][o];
                a = pair_turn[face][a];
                b = pair_turn[face][b];
            }
            path[depth] = (uint8_t) move;
            depth++;
            perm[depth] = (uint16_t) p;
            ori[depth] = (uint16_t) o;
            q0[depth] = (uint8_t) a;
            q1[depth] = (uint8_t) b;
            next_moves[depth] = 0;
        }
        if (next_bound == INT_MAX || next_bound > MAX_DEPTH)
            return -1;
        bound = next_bound;
    }
}

static int parse_state(const char *input, uint8_t *p, uint8_t *o)
{
    for (int i = 0; i < RUBIK_STATE_LEN; i++)
        if (!input[i])
            return 0;
    if (input[RUBIK_STATE_LEN])
        return 0;

    unsigned seen = 0, orientation_sum = 0;

    for (int i = 0; i < CUBIES; i++) {
        char c = input[i], t = input[i + CUBIES];

        if (c < '1' || c > '7' || t < '1' || t > '3')
            return 0;
        unsigned bit = 1U << (c - '1');
        if (seen & bit)
            return 0;
        seen |= bit;
        p[i] = (uint8_t) (c - '1');
        o[i] = (uint8_t) (t - '1');
        orientation_sum += o[i];
    }
    while (orientation_sum >= 3)
        orientation_sum -= 3;
    return orientation_sum == 0;
}

int rubik_solve(const char *state, char *out, size_t cap)
{
    uint8_t p[CUBIES], o[CUBIES];
    uint8_t path[MAX_DEPTH];

    if (!parse_state(state, p, o))
        return -1;

    uint8_t where[CUBIES];
    for (int i = 0; i < CUBIES; i++)
        where[p[i]] = (uint8_t) i;

    int length = solve(rank_permutation(p), rank_orientation(o),
                       where[0] * 7 + where[6], where[1] * 7 + where[4], path);
    if (length < 0)
        return -1;

    size_t pos = 0;
    for (int i = 0; i < length; i++) {
        const char *name = move_names[path[i]];

        if (i > 0) {
            if (pos + 1 >= cap)
                goto too_small;
            out[pos++] = ' ';
        }
        for (; *name; name++) {
            if (pos + 1 >= cap)
                goto too_small;
            out[pos++] = *name;
        }
    }
    if (pos >= cap)
        goto too_small;
    out[pos] = '\0';
    return length;

too_small:
    if (cap > 0)
        out[0] = '\0';
    return -2;
}
