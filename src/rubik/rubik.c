#include "rubik.h"
#include "depth5.h"
#include "moves.h"
#include "pdb.h"

#define CUBIES 7
#define MAX_DEPTH 11
#define NEAR_DEPTH 5

#define PRUNE -2
#define EXPAND -1

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

static int near_distance(int perm, int ori)
{
    uint32_t key = (uint32_t) perm << 10 | (uint32_t) ori;
    int lo = near_index[perm], hi = near_index[perm + 1] - 1;

    while (lo <= hi) {
        int mid = (lo + hi) >> 1;
        uint32_t entry = near_states[mid] >> 3;

        if (entry == key)
            return (int) (near_states[mid] & 7);
        if (entry < key)
            lo = mid + 1;
        else
            hi = mid - 1;
    }
    return -1;
}

static int check(int perm, int ori, int remaining)
{
    if (permutation_pdb[perm] > remaining || orientation_pdb[ori] > remaining)
        return PRUNE;
    if (remaining > NEAR_DEPTH)
        return EXPAND;

    int d = near_distance(perm, ori);
    return (d >= 0 && d <= remaining) ? d : PRUNE;
}

static int descend(int perm, int ori, int d, uint8_t *path)
{
    int length = 0;

    while (d > 0) {
        for (int move = 0; move < 9; move++) {
            int face = move_face[move];
            int p = perm, o = ori;

            for (int t = 0; t < move_turns[move]; t++) {
                p = perm_turn[face][p];
                o = ori_turn[face][o];
            }
            if (near_distance(p, o) == d - 1) {
                path[length++] = (uint8_t) move;
                perm = p;
                ori = o;
                d--;
                break;
            }
        }
    }
    return length;
}

static int solve(int start_perm, int start_ori, uint8_t path[MAX_DEPTH])
{
    uint16_t perm[MAX_DEPTH + 1], ori[MAX_DEPTH + 1];
    uint8_t next_moves[MAX_DEPTH + 1];

    for (int bound = 0; bound <= MAX_DEPTH; bound++) {
        int r = check(start_perm, start_ori, bound);

        if (r == PRUNE)
            continue;
        if (r >= 0)
            return descend(start_perm, start_ori, r, path);

        int depth = 0;
        perm[0] = (uint16_t) start_perm;
        ori[0] = (uint16_t) start_ori;
        next_moves[0] = 0;
        while (depth >= 0) {
            if (next_moves[depth] >= 9) {
                depth--;
                continue;
            }

            int move = next_moves[depth]++;
            int face = move_face[move];

            if (depth > 0 && face == move_face[path[depth - 1]])
                continue;

            int p = perm[depth], o = ori[depth];
            for (int t = 0; t < move_turns[move]; t++) {
                p = perm_turn[face][p];
                o = ori_turn[face][o];
            }

            r = check(p, o, bound - depth - 1);
            if (r == PRUNE)
                continue;
            path[depth] = (uint8_t) move;
            if (r >= 0)
                return depth + 1 + descend(p, o, r, path + depth + 1);
            depth++;
            perm[depth] = (uint16_t) p;
            ori[depth] = (uint16_t) o;
            next_moves[depth] = 0;
        }
    }
    return -1;
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

    int length = solve(rank_permutation(p), rank_orientation(o), path);
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
