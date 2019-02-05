#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define SUDO_SIZE 9
#define ALL_CANDIDATES ((1<<(SUDO_SIZE))-1)
#define SUDO_OK   0
#define SUDO_ERR  1

typedef struct {
  int array[SUDO_SIZE][SUDO_SIZE];
  int n_candidates[SUDO_SIZE][SUDO_SIZE];
  int unknown;  /* undertermined elments */
} sudo;

typedef struct sudo_snapshot {
  sudo sudo_data;
  int i;
  int j;
  int cand_val;
  struct sudo_snapshot *prev;
  struct sudo_snapshot *next;
} snapshot;

// n - 1..9
#define IS_BIT_SET(array_data, n) ((array_data) & (1 << ((n)-1)))
#define IS_BIT_SET_PTR(ptr, n) ((*(ptr)) & (1 << ((n)-1)))
#define CLEAR_BIT_PTR(ptr, n) (*(ptr) = (*(ptr)) & (~(1 << ((n)-1))))
#define SET_BIT_PTR(ptr, n)   (*(ptr) = (*(ptr)) | (1<<((n)-1)))

int sudo_check_unique_cand(sudo *s, int i, int j);

void sudo_init(sudo *s) {
  int i, j;
  for (i = 0; i < SUDO_SIZE; i++)
    for (j = 0; j < SUDO_SIZE; j++) {
      s->array[i][j] = ALL_CANDIDATES;
      s->n_candidates[i][j] = SUDO_SIZE;
    }
  s->unknown = SUDO_SIZE * SUDO_SIZE;
}

void sudo_print(sudo *s) {
  int i, j, n;
  char val;

  printf("=== answer ======    == n_candidates =    =========== cand_bitmap ===========\n");
  for (i = 0; i < SUDO_SIZE; i++) {
    for (j = 0; j < SUDO_SIZE; j++) {
      val = '-';
      if (s->n_candidates[i][j] == 1) {
        for (n = 1; n<= SUDO_SIZE && !IS_BIT_SET(s->array[i][j], n); n++);
        val = '0' + n;
      }
      printf("%c%s", val, ((j == SUDO_SIZE-1) ? "    " : " "));
    }

    for (j = 0; j < SUDO_SIZE; j++) {
      printf("%d%s", s->n_candidates[i][j], ((j == SUDO_SIZE-1) ? "    " : " "));
    }

    for (j = 0; j < SUDO_SIZE; j++) {
      printf("%03x%c", s->array[i][j], ((j == SUDO_SIZE-1) ? '\n' : ' '));
    }
  }
  printf("=================\n");
}

sudo *sudo_create() {
  sudo *s=malloc(sizeof(sudo));
  if (s != NULL)  sudo_init(s);
  return(s);
}

sudo *sudo_copy(sudo *src) {
  sudo *s=malloc(sizeof(sudo));
  if (s != NULL)  memcpy(s, src, sizeof(sudo));
  return(s);
}

void sudo_free(sudo *s) {
  if (s) free(s);
}

// return: bit map of new candidate in col that the value is determined.
int sudo_clear_col(sudo *s, int i, int j, int value, int *cand) {
  int ii;
  int *p = &s->array[0][j];
  int new_cand=0;
  int error = SUDO_OK;
  for (ii = 0; ii < SUDO_SIZE; ii++) {
    if (ii != i && IS_BIT_SET_PTR(p, value)) {
      CLEAR_BIT_PTR(p, value);
      s->n_candidates[ii][j]--;
      if (s->n_candidates[ii][j] == 0) return(SUDO_ERR);
      else if (s->n_candidates[ii][j] == 1) {
        SET_BIT_PTR(&new_cand, ii+1);
        s->unknown--;
      }
    }
    p += SUDO_SIZE;
  }
  *cand = new_cand;
  return (error);
}

int sudo_clear_row(sudo *s, int i, int j, int value, int *cand) {
  int jj;
  int *p = &s->array[i][0];
  int new_cand = 0;
  int error = SUDO_OK;
  for (jj = 0; jj < SUDO_SIZE; jj++) {
    if (jj != j && IS_BIT_SET_PTR(p, value)) {
      CLEAR_BIT_PTR(p, value);
      s->n_candidates[i][jj]--;
      if (s->n_candidates[i][jj] == 0) return(SUDO_ERR);
      else if (s->n_candidates[i][jj] == 1) {
        SET_BIT_PTR(&new_cand, jj+1);
        s->unknown--;
      }
    }
    p++;
  }
  *cand = new_cand;
  return (error);
}

int sudo_clear_sub_3x3(sudo *s, int i, int j, int value, int *cand) {
  int sub_i, sub_j, ii, jj;
  int *p;
  int new_cand = 0;
  int error = SUDO_OK;
  sub_i = (i / 3) * 3;
  sub_j = (j / 3) * 3;
  p = &s->array[sub_i][sub_j];
  for (ii = sub_i; ii < sub_i + 3; ii++) {
    for (jj = sub_j; jj < sub_j + 3; jj++) {
      if (ii != i && jj != j && IS_BIT_SET_PTR(p, value)) {
        CLEAR_BIT_PTR(p, value);
        s->n_candidates[ii][jj]--;
        if (s->n_candidates[ii][jj] == 0) return(SUDO_ERR);
        else if (s->n_candidates[ii][jj] == 1) {
          SET_BIT_PTR(&new_cand, (ii-sub_i)*3+(jj-sub_j)+1);
          s->unknown--;
        }
      }
      p++;
    }
    p += SUDO_SIZE - 3;
  }
  *cand = new_cand;
  return (error);
}

/* Return value:
   0 - Error detected.
   1 - Set successfully. */
int sudo_set_value(sudo *s, int i, int j, int value) {
  int *p = &s->array[i][j];
  int new_cand_in_row, new_cand_in_col, new_cand_in_sub_3x3;
  int n, val;
  int error = SUDO_OK;

//  assert(s->n_candidates[i][j]);
//  assert(IS_BIT_SET_PTR(p, value));
  if (!IS_BIT_SET_PTR(p, value)) return SUDO_ERR;
  if (s->n_candidates[i][j] != 1) {
    *p = 0;
    SET_BIT_PTR(p, value);
    if (s->n_candidates[i][j] != 1) s->unknown--;
    s->n_candidates[i][j]=1;
  }

  printf("Adding %d @ (%d, %d), %d left.\n", value, i, j, s->unknown);
  printf("sudo after adding %d at (%d, %d)\n", value, i, j);
  sudo_print(s);

  error = sudo_clear_row(s, i, j, value, &new_cand_in_row);
  if (error != SUDO_OK) return error;
//  printf("sudo after clearing row: %d at (%d, %d)\n", value, i, j);
//  sudo_print(s);
  error = sudo_clear_col(s, i, j, value, &new_cand_in_col);
  if (error != SUDO_OK) return error;
//  printf("sudo after clearing col %d at (%d, %d)\n", value, i, j);
//  sudo_print(s);
  error = sudo_clear_sub_3x3(s, i, j, value, &new_cand_in_sub_3x3);
  if (error != SUDO_OK) return error;
  printf("sudo after clearing row/col/sub3x3: %d at (%d, %d)\n", value, i, j);
  sudo_print(s);

  if (new_cand_in_row > 0) {
    for (n = 1; n <= SUDO_SIZE; n++) {
      if (IS_BIT_SET(new_cand_in_row, n)) {
        for (val = 1; val<= SUDO_SIZE && !IS_BIT_SET(s->array[i][n-1], val); val++);
        if (sudo_set_value(s, i, n-1, val) != SUDO_OK) return SUDO_ERR;
      }
    }
  }

  if (new_cand_in_col > 0) {
    for (n = 1; n <= SUDO_SIZE; n++) {
      if (IS_BIT_SET(new_cand_in_col, n)) {
        for (val = 1; val<= SUDO_SIZE && !IS_BIT_SET(s->array[n-1][j], val); val++);
        if (sudo_set_value(s, n-1, j, val) != SUDO_OK) return SUDO_ERR;
      }
    }
  }

  if (new_cand_in_sub_3x3) {
    int sub_i, sub_j, ii, jj;
    sub_i = (i / 3) * 3;
    sub_j = (j / 3) * 3;
    for (n = 1; n <= SUDO_SIZE; n++) {
      if (IS_BIT_SET(new_cand_in_sub_3x3, n)) {
        ii = (n-1) / 3;
        jj = (n-1) - 3 * ii;
        for (val = 1; val<= SUDO_SIZE && !IS_BIT_SET(s->array[sub_i+ii][sub_j+jj], val); val++);
        if (sudo_set_value(s, sub_i+ii, sub_j+jj, val) != SUDO_OK) return SUDO_ERR;
      }
    }
  }

  error = sudo_check_unique_cand(s, i, j);
  return(error);
}

int sudo_init_from_file(sudo *s, FILE *fp) {
  char line[64];
  int i, j;
  i = 0;
  char *p, ch;

  /* read from stdin */
  if (fp == NULL) {
    int reset;
    do {
      reset = 0;
      for (i = 0; i < SUDO_SIZE; i++) {
        for (j = 0; j < SUDO_SIZE; j++) {
          do {
            printf("Input element at (%d, %d) [ '1' - '9', RET if not determined, 'R' to reset ]: ", i, j);
            ch = getchar();
            if (ch != '\n') {
              /* ignore all other characters */
              while (getchar() != '\n');
            }
            if (ch == 'R' || ch == 'r') {
              reset = 1;
              goto SUDO_RESET;
            }
            if (ch == '\n') break;
            if (ch < '1' || ch > '9') {
              printf("Invalid [%c]\n", ch);
              ch = '0';
            } else sudo_set_value(s, i, j, ch - '0');
          } while (ch == '0');
        }
      }
    SUDO_RESET:
      if (reset) {
        i = 0; j = 0;
        sudo_init(s);
      }
    } while (reset);
    return (0);
  }

  while (fgets(line, 64, fp)) {
    j = 0;
    p = line;
    while (*p && *p != '\n') {
      if (*(p+1) != ' ' && *(p+1) != '\n') return 1;
      else if (*p >= '1' && *p <= '9') sudo_set_value(s, i, j, *p-'0');
      else if (*p != 'x' && *p != 'X' && *p != '-') return 2;
      j++;
      p += 2;
    }
    i++;
  }
  return(0);
}

/*
------------------
6 x x x x x x 1 x    1 2 3 5 5 7 4 1 4    020 180 114 156 0ce 1de 1d0 001 01e
4 x 7 x x x x x x    1 2 1 5 5 6 3 5 4    008 180 040 136 0a7 197 190 136 036
1 2 x x x x x x x    1 1 3 5 5 6 4 6 4    001 002 114 174 0ec 1dc 1d0 17c 03c
x 3 x x 5 x 4 8 7    2 1 2 2 1 2 1 1 1    102 004 102 022 010 003 008 080 040
x 6 8 x x x 3 x x    4 1 1 2 4 4 1 3 3    152 020 080 042 04b 04b 004 112 013
x x 1 x 9 x 6 x x    3 2 1 3 1 5 1 2 2    052 048 001 046 100 0ce 020 012 012
3 x x 4 7 x 2 x 8    1 3 2 1 1 3 1 3 1    004 141 120 008 040 150 002 070 080
x 5 x 1 x x 7 x x    4 1 4 1 3 4 1 4 3    1c2 010 12a 001 046 146 040 06c 02c
x x x 8 x 6 1 x 9    2 2 2 1 3 1 1 4 1    042 048 00a 080 046 020 001 05c 100
------------------
For the example above, consider for element @ (6, 1), though it has 3 candidates (1, 7, 9) , notice in the sub 3x3 @ (6, 0), the value 1 can only be here.

This function checks each candidate in each row/col/sub3x3 to see if there is a candidate that can locate only in one element. If so, set the the candidate value to that element.

Return value: SUDO_OK/SUDO_ERR
*/
int sudo_check_unique_cand(sudo *s, int i, int j) {
  int n, val, ii, jj;
  int cand_count, cand_i, cand_j;
  int sub_i, sub_j, sub3x3;
  int error = SUDO_OK;

  // check row
  for (n = 1; n <= SUDO_SIZE; n++) {
    cand_count=0;
    for (jj = 0; jj < SUDO_SIZE; jj++) {
      if (IS_BIT_SET(s->array[i][jj], n)) {
        cand_count++;
        cand_j = jj;
      }
      if (cand_count > 1) break;
    }
    if (cand_count == 1 && s->n_candidates[i][cand_j] > 1) error = sudo_set_value(s, i, cand_j, n);
    if (cand_count == 0 || error == SUDO_ERR)  return(SUDO_ERR);
  }

  // check col
  for (n = 1; n <= SUDO_SIZE; n++) {
    cand_count=0;
    for (ii = 0; ii < SUDO_SIZE; ii++) {
      if (IS_BIT_SET(s->array[ii][j], n)) {
        cand_count++;
        cand_i = ii;
      }
      if (cand_count > 1) break;
    }
    if (cand_count == 1 && s->n_candidates[cand_i][j] > 1) error = sudo_set_value(s, cand_i, j, n);
    if (cand_count == 0 || error == SUDO_ERR)  return(SUDO_ERR);
  }

  // check sub3x3
  sub_i = (i / 3) * 3;
  sub_j = (j / 3) * 3;
  for (n = 1; n <= SUDO_SIZE; n++) {
    cand_count=0;
    for (ii = sub_i; ii < sub_i + 3; ii++) {
      if (IS_BIT_SET(s->array[ii][sub_j+0], n)) {
        cand_count++;
        cand_i = ii;
        cand_j = sub_j;
      }
      if (IS_BIT_SET(s->array[ii][sub_j+1], n)) {
        cand_count++;
        cand_i = ii;
        cand_j = sub_j + 1;
      }
      if (IS_BIT_SET(s->array[ii][sub_j+2], n)) {
        cand_count++;
        cand_i = ii;
        cand_j = sub_j + 2;
      }
      if (cand_count > 1) break;
    }
    // After all 9 elements have been scanned to given value n.
    if (cand_count == 1 && s->n_candidates[cand_i][cand_j] > 1) error = sudo_set_value(s, cand_i, cand_j, n);
    if (cand_count == 0 || error == SUDO_ERR)  return(SUDO_ERR);
  }

  return(SUDO_OK);
}

int sudo_speculate(sudo *s) {
  int i, j, n;
  int cand_i, cand_j, cand_n, cand_min;
  int error = SUDO_OK;
  sudo *snapshot = NULL;

  if (s->unknown == 0) return SUDO_OK;

  while (s->unknown) {
    snapshot = sudo_copy(s);
    cand_min = SUDO_SIZE;
    cand_n = 1;
    /* Find an element with fewest candidates. */
    for (i = 0; i < SUDO_SIZE; i++)
      for (j = 0; j < SUDO_SIZE; j++) {
        if (s->n_candidates[i][j] > 1 && s->n_candidates[i][j] < cand_min) {
          cand_min = s->n_candidates[i][j];
          cand_i = i;
          cand_j = j;
        }
      }
    for (n = 1; n <= SUDO_SIZE; n++) {
      if (IS_BIT_SET(s->array[cand_i][cand_j], n)) {
        printf("Speculating (%d, %d) -> %d\n", cand_i, cand_j, n);
        error = sudo_set_value(s, cand_i, cand_j, n);
        if (error == SUDO_ERR) {
          /* Bad luck. Speculate error. */
          /* [cand_i, cand_j] cannot be value n. Remove it from candidate bitmap. */
          /* Restore snapshot. */
          CLEAR_BIT_PTR(&snapshot->array[cand_i][cand_j], n);
          snapshot->n_candidates[cand_i][cand_j]--;
          if (snapshot->n_candidates[cand_i][cand_j] == 1) snapshot->unknown--;
          memcpy(s, snapshot, sizeof(sudo));
          printf("Speculating (%d, %d) -> %d ERROR! Restore... %d left\n", cand_i, cand_j, n, s->unknown);
          sudo_print(s);
          if (s->n_candidates[cand_i][cand_j] == 0) {
            sudo_free(snapshot);
            return SUDO_ERR;
          }
        } else {
          if (s->unknown == 0) {
            sudo_free(snapshot);
            return(SUDO_OK);
          } else {
            error = sudo_speculate(s);
            if (SUDO_ERR == error) {
              CLEAR_BIT_PTR(&snapshot->array[cand_i][cand_j], n);
              snapshot->n_candidates[cand_i][cand_j]--;
              if (snapshot->n_candidates[cand_i][cand_j] == 1) snapshot->unknown--;
              memcpy(s, snapshot, sizeof(sudo));
              printf("Speculating (%d, %d) -> %d ERROR! Restore... %d left\n", cand_i, cand_j, n, s->unknown);
              if (s->n_candidates[cand_i][cand_j] == 0) {
                sudo_free(snapshot);
                return(error);
              }
            }
          }
        }
        /*
        if (SUDO_ERR == error) {
          sudo_free(snapshot);
          return(error);
        }
        */
      }
    }
  }
}

int main(int argc, char **argv) {
  FILE *fp = NULL;
  sudo *s;

  if (argc == 1) {
    printf("Usage: %s [sudo_file]\n",  argv[0]);
    printf("No input file, reading sudoku from stdin.\n");
  }

  if (argc > 1) {
    fp = fopen(argv[1], "r");
    if (!fp) {
      printf("File %s cannot be opened.\n", argv[1]);
      return(2);
    }
  }

  s = sudo_create();
  if (sudo_init_from_file(s, fp)) {
     printf("sudo file format error.\n");
     return(3);
  }

  printf("\n\nResult of adding only candidates (%d left):\n", s->unknown);
  sudo_print(s);
  printf("\n\n");

  /* Getting here means speculation is needed now. */
  sudo_speculate(s);

  printf("\n\nSUDO calculation %s, %d left. Answer is: \n\n", s->unknown ? "FAIL" : "OK", s->unknown);
  sudo_print(s);

  sudo_free(s);
}

