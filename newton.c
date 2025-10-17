#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <threads.h>
#include <time.h>

float min_val = -2;
float max_val = 2;
int n_thrds = 1;
int sz;
int d;
static float *roots_r;
static float *roots_c;

typedef uint8_t TYPE_ATTR;
typedef uint8_t TYPE_CONV;

typedef struct {
  int val;
  char pad[60];
} int_padded;

typedef struct {
  const float *v;
  int **root_idx_res;
  int ib;
  int istep;
  int sz;
  int tx;
  mtx_t *mtx;
  cnd_t *cnd;
  int_padded *status;
} thrd_info_t;

typedef struct {
  const float *v;
  int **root_idx_res;
  int sz;
  int n_thrds;
  mtx_t *mtx;
  cnd_t *cnd;
  int_padded *status;
} thrd_info_write_t;

void parse_args(int argc, char *argv[]);
int main_thrd(void *args);
int main_thrd_write(void *args);

int main(
  int argc,
  char *argv[]
)
{

  parse_args(argc, argv);

  for ( int i = 0; i < d; ++i ) {
    printf("Root x_%d = %.3f + i%.3f\n", i, roots_r[i], roots_c[i]);
  }

  float *v = (float*) malloc(sz*sizeof(float));
  int **root_idx_res = (int**) malloc(sz*sizeof(int*));
  

  thrd_t thrds[n_thrds];
  thrd_info_t thrds_info[n_thrds];

  thrd_t write_thrd;
  thrd_info_write_t thrd_info_write;

  mtx_t mtx;
  mtx_init(&mtx, mtx_plain);

  cnd_t cnd;
  cnd_init(&cnd);

  int_padded status[n_thrds];
  for ( int tx = 0; tx < n_thrds; ++tx ) {
    thrds_info[tx].v = (const float*) v;
    thrds_info[tx].root_idx_res = root_idx_res;
    thrds_info[tx].ib = tx;
    thrds_info[tx].istep = n_thrds;
    thrds_info[tx].sz = sz;
    thrds_info[tx].tx = tx;
    thrds_info[tx].mtx = &mtx;
    thrds_info[tx].cnd = &cnd;
    thrds_info[tx].status = status;
    status[tx].val = -1;

    int r = thrd_create(thrds + tx, main_thrd, (void*) (thrds_info + tx));
    if ( r != thrd_success ) {
      fprintf(stderr, "Failed to create thread\n");
      exit(1);
    }
    thrd_detach(thrds[tx]);
  }
  {
    thrd_info_write.v = (const float*) v;
    thrd_info_write.root_idx_res = (int**) root_idx_res;
    thrd_info_write.sz = sz;
    thrd_info_write.n_thrds = n_thrds;
    thrd_info_write.mtx = &mtx;
    thrd_info_write.cnd = &cnd;
    thrd_info_write.status = status;

    int r = thrd_create(&write_thrd, main_thrd_write, (void*)(&thrd_info_write));
    if ( r != thrd_success ) {
      fprintf(stderr, "Failed to create thread\n");
      exit(1);
    }
  }

  {
    int r;
    thrd_join(write_thrd, &r);
  }

  free(v);
  free(root_idx_res);
  free(roots_r);
  free(roots_c);

  mtx_destroy(&mtx);
  cnd_destroy(&cnd);

  return 0;
}

int main_thrd(
  void *args
)
{
  const thrd_info_t *thrd_info = (thrd_info_t*) args;
  const float *v = thrd_info->v;
  int **root_idx_res = thrd_info->root_idx_res;
  const int ib = thrd_info->ib;
  const int istep = thrd_info->istep;
  const int sz = thrd_info->sz;
  const int tx = thrd_info->tx;
  mtx_t *mtx = thrd_info->mtx;
  cnd_t *cnd = thrd_info->cnd;
  int_padded *status = thrd_info->status;

  for ( int ix = ib; ix < sz; ix += istep ) {
    const float vix = v[ix];
    int *root_idx_row = (int*) malloc(sz*sizeof(int));

    for ( int jx = 0; jx < sz; ++jx ) {
      // Insert computations here or something
      // For now, we do a simple implementation to check functionality of threading/mtx/cnd
      root_idx_row[jx] = ix*jx;
    }

    mtx_lock(mtx);
    root_idx_res[ix] = root_idx_row;
    status[tx].val = ix + istep;
    mtx_unlock(mtx);
    cnd_signal(cnd);

    thrd_sleep(&(struct timespec){.tv_sec = 0, .tv_nsec = 1000}, NULL);
  }

  return 0;
}

int main_thrd_write(
  void *args
)
{
  const thrd_info_write_t *thrd_info = (thrd_info_write_t*) args;
  const float *v = thrd_info->v;
  int **root_idx_res = thrd_info->root_idx_res;
  const int sz = thrd_info->sz;
  const int n_thrds = thrd_info->n_thrds;
  mtx_t *mtx = thrd_info->mtx;
  cnd_t *cnd = thrd_info->cnd;
  int_padded *status = thrd_info->status;

  for ( int ix = 0, ibnd; ix < sz; ) {
    
    for ( mtx_lock(mtx); ; ) {
      ibnd = sz;
      for ( int tx = 0; tx < n_thrds; ++tx ) {
        if ( ibnd > status[tx].val ) ibnd = status[tx].val;
      }

      if ( ibnd <= ix ) {
        cnd_wait(cnd, mtx);
      }
      else {
        mtx_unlock(mtx);
        break;
      }
    }

    printf("\n\nWriting until %i\n", ibnd);
    for ( ; ix < ibnd; ++ix ) {
      // Insert write to file here
      // For now, we do a dummy print to check threading/mtx/cnd functionality
      int res = root_idx_res[ix][ix];
      printf("Index: %i - value: %i\n", ix, res);
      thrd_sleep(&(struct timespec){.tv_sec=0, .tv_nsec=1000}, NULL);
      free(root_idx_res[ix]);
    }
  }

  return 0;
}

void parse_args(
  int argc,
  char *argv[]
)
{
  for (int i = 1; i < argc - 1; ++i) {
    const char *arg = argv[i];
    if ( arg[0] == '-' && arg[2] != '\0' ) {
      if ( !isdigit(arg[2]) ) {
        printf("Unknown argument: %s\n", arg);
        continue;
      }
      if ( arg[1] == 't' ) {
        n_thrds = atoi(&arg[2]);
        printf("Using %d threads\n", n_thrds);
      }
      else if ( arg[1] == 'l' ) {
        sz = atoi(&arg[2]);
        printf("Using %d lines\n", sz);
      }
    }
  }
  
  const char *arg = argv[argc - 1];
  if ( isdigit(arg[0]) ) {
    d = atoi(&arg[0]);
    printf("Using exponent %d\n", d);
  }

  roots_r = (float*) malloc(sizeof(float)*d);
  roots_c = (float*) malloc(sizeof(float)*d);
  for ( int i = 0; i < d; ++i ) {
    double angle = (double)(2*M_PI*i/d);
    roots_r[i] = (float)cos(angle);
    roots_c[i] = (float)sin(angle);
  }
}
