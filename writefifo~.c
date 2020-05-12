#define _GNU_SOURCE

#include <fcntl.h>
#include <libgen.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "m_pd.h"

static t_class *writefifo_tilde_class;

typedef struct _writefifo_tilde {
  t_object x_obj; // mandatory object
  t_inlet *x_inL; // first signal inlet
  t_inlet *x_inR; // second signal inlet
  int writefd;    // fifo file descriptor
} t_writefifo_tilde;

t_int *writefifo_tilde_perform(t_int *w) {
  t_writefifo_tilde *x = (t_writefifo_tilde *)(w[1]);
  t_sample *inL = (t_sample *)(w[2]);
  t_sample *inR = (t_sample *)(w[3]);
  int n = (int)(w[4]);

  int16_t buf[n * 2];

  for (int i = 0; i < n; i++) {
    buf[i * 2] = INT16_MAX * inL[i];
    buf[i * 2 + 1] = INT16_MAX * inR[i];
  }

  write(x->writefd, buf, sizeof(buf));

  return (w + 5); // return pointer to next dsp object dataspace
}

void writefifo_tilde_dsp(t_writefifo_tilde *x, t_signal **sp) {
  /* register the writefifo_perform() function with DSP-tree */
  dsp_add(writefifo_tilde_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec,
          sp[0]->s_n);

  /* initialize loudness */
  x->loudness = 0;

  /* set fifo size to minimize latency */
  const int num_chan = 2;    // number of audio channels
  const int num_blocks = 32; // number of blocks to store in fifo
  int pipe_size = sp[0]->s_n * sizeof(int16_t) * num_chan * num_blocks;
  post("writefifo~: pipe size = %d bytes", pipe_size);
  fcntl(x->writefd, F_SETPIPE_SZ, pipe_size); // 46 ms of audio in fifo
}

void writefifo_tilde_free(t_writefifo_tilde *x) {
  inlet_free(x->x_inL);
  inlet_free(x->x_inR);
  close(x->writefd); // also maybe delete fifo?
}

void mkdir_p(const char *path) {
  char *subpath, *fullpath;

  fullpath = strdup(path);
  subpath = dirname(fullpath);
  if (strlen(subpath) > 1)
    mkdir_p(subpath);
  mkdir(subpath, 0777);
  free(fullpath);
}

void *writefifo_tilde_new(t_symbol *s) {

  const char *path;
  if (s->s_name[0] == '\0') {
    path = "fifo";
  } else {
    path = s->s_name;
  }

  post("writefifo~: path = %s", path);

  t_writefifo_tilde *x = (t_writefifo_tilde *)pd_new(writefifo_tilde_class);
  x->x_inL = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  x->x_inR = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);

  struct stat st;
  if (!stat(path, &st) && S_ISFIFO(st.st_mode)) {
    /* fifo already exists */
    post("writefifo~: %s already exists", path);
  } else {
    /* make fifo */
    post("writefifo~: creating %s", path);
    mkdir_p(path);
    mkfifo(path, 0666);
  }

  /* open fifo for read first so open for write succeeds */
  int readfd = open(path, O_RDONLY | O_NONBLOCK);
  x->writefd = open(path, O_WRONLY);
  close(readfd);

  return (void *)x;
}

#pragma GCC diagnostic ignored "-Wcast-function-type"
void writefifo_tilde_setup(void) {
  writefifo_tilde_class =
      class_new(gensym("writefifo~"),             // object name
                (t_newmethod)writefifo_tilde_new, // constructor
                (t_method)writefifo_tilde_free,   // destructor
                sizeof(t_writefifo_tilde),        // dataspace size
                CLASS_NOINLET,                    // normal object
                A_DEFSYMBOL,                      // message argument
                0);

  /* call writefifo_dsp() when audio-engine starts */
  class_addmethod(writefifo_tilde_class, (t_method)writefifo_tilde_dsp,
                  gensym("dsp"), 0);
}
