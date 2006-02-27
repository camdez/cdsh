/*

cdsh - a simple *nix shell for education purposes
Copyright (C) 2005 Cameron Desautels <cam@apt2324.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/


#include <assert.h>
#include <sys/types.h>
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
void execute_command(char* c_argv[], int c_argc);

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
      /* TODO: see if we can take if from $PS1 */
      printf(PROMPT, getenv("PWD"));

      /* Read in command */
      char command[MAX_COMMAND_LENGTH];

      if (ngets(command, MAX_COMMAND_LENGTH) == NULL) {
        /* They want to quit or the script has come to an end */
        quit_requested = 1;
        printf("\n");
      } else if (strlen(command) < 1) {
        /* They hit enter, so do nothing */
      } else {
        /* Execute command */
        parse_command(command);
      }
    }

  return EXIT_SUCCESS;
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
      printf("%s", c_argv[i]);
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


void execute_command(char* c_argv[], int c_argc)
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
      /* This is the child: exec() */
      execvp(c_argv[0], c_argv);
    }

    /* This is the parent process */
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
  int c_argc = 0; 

  while (token != NULL)
    {
      /* Do something with the tokens! */
      c_argv[c_argc] = malloc(MAX_COMMAND_LENGTH*sizeof(char));
      strcpy(c_argv[c_argc], token);

      /* strtok() that the 2nd arg be NULL in subsequent calls*/
      token = strtok(NULL, TOKEN_SEPARATOR);
      c_argc++;
    }

  /* Terminate the arguments list with a NULL */
  c_argv[c_argc] = NULL;
  c_argc--; /* We have one less than it appears */

  execute_command(c_argv, c_argc);

  /* free() the malloc'd memory */
  int i;
  for (i = 0; i <= c_argc; i++)
    free(c_argv[i]);
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

