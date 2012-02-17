/*

cdsh - a simple *nix shell for educational purposes
Copyright (C) 2005 Cameron Desautels <camdez@gmail.com>

*/

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROMPT                   "cdsh:%s$ "
#define TOKEN_SEPARATOR          " \t"
#define MAX_COMMAND_LENGTH       256
#define MAX_COMMAND_ARGUMENTS    32
#define MAX_DIR_NAME_LENGTH      256

char* ngets(char* str, int max_chars);
void parse_command(char* command_str);
void execute_command(char* c_argv[], int c_argc, int fg, int in_fd, int out_fd);

void builtin_set(char* c_argv[], int c_argc);
void builtin_echo(char* c_argv[], int c_argc);
void builtin_pwd(char* c_argv[], int c_argc);
void builtin_cd(char* c_argv[], int c_argc);

int quit_requested = 0;

int main()
{
  while (!quit_requested)
    {
      /* Print prompt */
      /* TODO: see if we can take it from $PS1 */
      printf(PROMPT, getenv("PWD"));

      /* Read in command */
      char command[MAX_COMMAND_LENGTH];

      if (ngets(command, MAX_COMMAND_LENGTH) == NULL) {
        /* They want to quit or the script has come to an end */
        quit_requested = 1;
        puts("\n");
      } else if (strlen(command) < 1) {
        /* They hit enter, so do nothing */
      } else {
        /* Execute command */
        parse_command(command);
      }
    }

  return EXIT_SUCCESS;
}


void execute_command(char* c_argv[], int c_argc, int fg, int in_fd, int out_fd)
{
  /* Check for built-ins */
  if (strcmp(c_argv[0], "cd") == 0) {
    builtin_cd(c_argv, c_argc);
  } else if (strcmp(c_argv[0], "set") == 0) {
    builtin_set(c_argv, c_argc);
  } else if (strcmp(c_argv[0], "echo") == 0) {
    builtin_echo(c_argv, c_argc);
  } else if (strcmp(c_argv[0], "pwd") == 0) {
    builtin_pwd(c_argv, c_argc);
  } else if ((strcmp(c_argv[0], "exit") == 0) ||
             (strcmp(c_argv[0], "quit") == 0)) {
    quit_requested = 1;
  } else {
    /* Not a built-in, so fork() and exec() it */
    int c_pid = 0;
    c_pid = fork();

    if (c_pid == -1) {
      fprintf(stderr, "Failed to fork()!  Aborting.");
      exit(EXIT_FAILURE);
    } else if (!c_pid) {
      /* This is the child */
      if (in_fd != 0) {
        close(0);
        dup(in_fd);
      }

      if (out_fd != 1) {
        close(1);
        dup(out_fd);
      }

      execvp(c_argv[0], c_argv);
    }

    /* This is the parent process */
    if (fg)
      wait(NULL);
  }
}


void parse_command(char* command_str)
{
  /* Tokenize the string */
  assert(strlen(command_str) > 0);

  char* c_argv[MAX_COMMAND_ARGUMENTS];

  /* We know that we have at least one character in here because we
     checked earlier, so we know that we have at least one token. */
  char* token = strtok(command_str, TOKEN_SEPARATOR);
  int run_command = 0;
  int no_more_commands = 0;
  int c_argc = 0;
  int fg = 1;
  int out_fd = STDOUT_FILENO;
  int in_fd = STDIN_FILENO;
  int next_in_fd = -1;

  for (;;)
    {
      if (next_in_fd != -1)
        {
          in_fd = next_in_fd;

          next_in_fd = -1;
        }

      if (token == NULL)
        {
          no_more_commands = 1;

          run_command = 1;
        }
      else if (strcmp(token, "&") == 0)
        {
          fg = 0;

          run_command = 1;
        }
      else if (strcmp(token, ";") == 0)
        {
          run_command = 1;
        }
      else if ((strcmp(token, "|") == 0) && (out_fd == STDOUT_FILENO))
        {
          int pipe_fds[2];
          if (pipe(pipe_fds) == -1)
            {
              perror("pipe");
              break;
            }

          out_fd = pipe_fds[1];
          next_in_fd = pipe_fds[0];

          run_command = 1;
        }
      else if ((strcmp(token, ">") == 0) && (out_fd == STDOUT_FILENO))
        {
          char* out_name = strtok(NULL, TOKEN_SEPARATOR);
          if (out_name == NULL)
            {
              fputs("Error: missing output redirection location\n", stderr);
              break;
            }

          out_fd = open(out_name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
          if (out_fd == -1)
            {
              perror("open");
              break;
            }
        }
      else if ((strcmp(token, "<") == 0) && (in_fd == STDIN_FILENO))
        {
          char* in_name = strtok(NULL, TOKEN_SEPARATOR);
          if (in_name == NULL)
            {
              fputs("Error: missing input redirection location\n", stderr);
              break;
            }

          in_fd = open(in_name, O_RDONLY);
          if (in_fd == -1)
            {
              perror("open");
              break;
            }
        }
      else
        {
          /* Do something with the tokens! */
          c_argv[c_argc++] = strdup(token);
        }

      /* Run the command */
      if (run_command && c_argc)
        {
          /* Terminate the arguments list with a NULL */
          c_argv[c_argc] = NULL;
          c_argc--; /* We have one less than it appears */

          execute_command(c_argv, c_argc, fg, in_fd, out_fd);

          /* free() the malloc'd memory */
          int i;
          for (i = 0; i <= c_argc; i++)
            free(c_argv[i]);

          /* Reset environment */
          c_argc = 0;
          run_command = 0;
          fg = 1;

          if (out_fd != STDOUT_FILENO)
            {
              close(out_fd);
              out_fd = STDOUT_FILENO;
            }

          if (in_fd != STDIN_FILENO)
            {
              close(in_fd);
              in_fd = STDIN_FILENO;
            }
        }

      if (no_more_commands)
        {
          return;
        }

      /* the lst arg must be NULL in subsequent calls to strtok() */
      token = strtok(NULL, TOKEN_SEPARATOR);
    }
}


/* Why doesn't this function exist already??? */
char* ngets(char* str, int max_chars)
{
  char* fret = fgets(str, max_chars, stdin);

  int i = strlen(str)-1;
  if (str[i] == '\n')
    str[i] = '\0';

  return fret;
}


void builtin_set(char* c_argv[], int c_argc)
{
  if (c_argc != 2) {
    printf("usage: set name value\n");
  } else {
    setenv(c_argv[1], c_argv[2], 1);
  }
}


void builtin_echo(char* c_argv[], int c_argc)
{
  int i;
  for (i = 1; i <= c_argc; i++)
    {
      if (i == c_argc) {
        printf("%s", c_argv[i]);
      } else {
        printf("%s ", c_argv[i]);
      }
    }

  printf("\n");
}


void builtin_pwd(char* c_argv[], int c_argc)
{
  if (c_argc != 0) {
    printf("usage: pwd\n");
  } else {
    char cwd[MAX_DIR_NAME_LENGTH];
    getcwd(cwd, sizeof(cwd));
    printf("%s\n", cwd);
  }
}


void builtin_cd(char* c_argv[], int c_argc)
{
  if (c_argc != 1) {
    printf("usage: cd dir\n");
  } else {
    if (chdir(c_argv[1]) == 0) {
      char cwd[MAX_DIR_NAME_LENGTH];
      getcwd(cwd, sizeof(cwd));
      setenv("PWD", cwd, 1);
    } else {
      printf("Could not change to directory \"%s\"\n", c_argv[1]);
    }
  }
}
