/*
 * $HEADER$
 */

#ifndef LAM_ARGV_H
#define LAM_ARGV_H

#include "lam/types.h"

#ifdef __cplusplus
extern "C" {
#endif
  int lam_argv_add(int *argc, char ***argv, char *arg);
  void lam_argv_free(char **argv);
  char **lam_argv_split(char *src_string, int delimiter);
  int lam_argv_count(char **argv);
  char *lam_argv_join(char **argv, int delimiter);
  size_t lam_argv_len(char **argv);
  char **lam_argv_copy(char **argv);
#ifdef __cplusplus
}
#endif

#endif /* LAM_ARGV_H */
