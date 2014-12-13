#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUDO_SIZE 9
#define ALL_CANDIDATES ((1<<(SUDO_SIZE))-1)

typedef struct {
  int array[SUDO_SIZE][SUDO_SIZE];
  int n_candidates[SUDO_SIZE][SUDO_SIZE];
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

void sudo_init(sudo *s) {
  int i, j;
  for (i = 0; i < SUDO_SIZE; i++)
    for (j = 0; j < SUDO_SIZE; j++) {
      s->array[i][j] = ALL_CANDIDATES;
      s->n_candidates[i][j] = SUDO_SIZE;
    }
}

void sudo_print(sudo *s) {
  int i, j, n;
  char val;

  printf("The sudo created is:\n");
  printf("------------------\n");
  for (i = 0; i < SUDO_SIZE; i++)
    for (j = 0; j < SUDO_SIZE; j++) {
      val = 'x';
      if (s->n_candidates[i][j] == 1) {
        for (n = 1; n<= SUDO_SIZE && !IS_BIT_SET(s->array[i][j], n); n++);
        val = '0' + n; 
      }
      printf("%c%c", val, ((j == SUDO_SIZE-1) ? '\n' : ' '));
    }  
  printf("------------------\n");
}

sudo *sudo_create() {
  sudo *s=malloc(sizeof(sudo));
  if (s != NULL)  sudo_init(s);
  return(s);
}

void sudo_free(sudo *s) {
  if (s) free(s);
}

void sudo_clear_col(sudo *s, int i, int j, int value) {
  int *p = &s->array[i][j];
  if (!IS_BIT_SET_PTR(p, value)) {
    //CLEAR_BIT_PTR(p, value);
    //s->n_candidates[i][j]--;
  }
}

void sudo_clear_row(sudo *s, int i, int j, int value) {
  int *p = &s->array[i][j];
  if (!IS_BIT_SET_PTR(p, value)) {
    //CLEAR_BIT_PTR(p, value);
    //s->n_candidates[i][j]--;
  }
}

void sudo_clear_sub_3x3(sudo *s, int i, int j, int value) {
  int *p = &s->array[i][j];
  if (!IS_BIT_SET_PTR(p, value)) {
    //CLEAR_BIT_PTR(p, value);
    //s->n_candidates[i][j]--;
  }
}

int sudo_set_value(sudo *s, int i, int j, int value) {
  int *p = &s->array[i][j];
  *p = 0;
  SET_BIT_PTR(p, value);
  s->n_candidates[i][j]=1;

  sudo_clear_row(s, i, j, value);
  sudo_clear_col(s, i, j, value);
  sudo_clear_sub_3x3(s, i, j, value);
}

int sudo_init_from_file(sudo *s, FILE *fp) {
  char line[64];
  int i, j;
  i = 0;
  char *p;
  while (fgets(line, 64, fp)) {
    j = 0;
    p = line;
    while (*p && *p != '\n') {
      if (*(p+1) != ' ' && *(p+1) != '\n') return 1;
      else if (*p >= '1' && *p <= '9') sudo_set_value(s, i, j, *p-'0');
      else if (*p != 'x' && *p != 'X') return 2;
      j++;
      p += 2;
    }
    i++;
  }
  return(0);
}

int main(int argc, char **argv) {
  FILE *fp;
  sudo *s;
  
  if (argc != 2) {
    printf("Usage: %s sudo_file\n",  argv[0]);
    return(1);
  }
  
  fp = fopen(argv[1], "r");
  if (!fp) {
    printf("File %s cannot be opened.\n", argv[1]);
    return(2);
  }

  s = sudo_create();
  if (sudo_init_from_file(s, fp)) {
     printf("sudo file format error.\n");
     return(3);
  }

  sudo_print(s);

  sudo_free(s);
}

