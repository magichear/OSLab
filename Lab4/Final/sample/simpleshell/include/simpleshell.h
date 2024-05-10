#ifndef SIMPLESHELL_H
#define SIMPLESHELL_H
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

int split_string(char *string, char *sep, char **string_clips);
int exec_builtin(int argc, char **argv, int *fd);
int process_redirect(int argc, char **argv, int *fd);
int execute(int argc, char **argv);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif  // SIMPLESHELL_H

