/*
Jane Lockshin
CSCI 442: Operating Systems
Project 0
Jan 30, 2019
Colorado School of Mines
*/

// Include necessary libraries
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
using namespace std;

// "The one and only error message"
char error_message[30] = "An error has occurred\n";

// Declare built-in functions
int mysh_cd(char **args);
int mysh_pwd(char **args);
int mysh_exit(char **args);

// Declare built-in functions
int (*builtins[]) (char **) = {
  &mysh_cd,
  &mysh_pwd,
  &mysh_exit
};

// Declare built-in functions
const char *builtins_str[] = {
  "cd",
  "pwd",
  "exit"
};

// Built-in function: 'cd'
int mysh_cd(char **args)
{
  // Declare 'home' environment
  const char *home = getenv("HOME");
  // If there are more than two arguments, print an error message
  if (sizeof(**args) > 2) {
    write(STDERR_FILENO, error_message, strlen(error_message));
  } else {
    // If there are no arguments, send to 'home' directory
    if (args[1] == NULL) {
      chdir(home);
    } else {
      // 'cd' into directory (it it exists)
      if (chdir(args[1]) != 0) {
        write(STDERR_FILENO, error_message, strlen(error_message));
      }
    }
  }
  return 1;
}

// Built-in function: 'pwd'
int mysh_pwd(char **args)
{
  // Declare current working directory
  char pwd[256];
  // 'pwd' should not take any other arguments
  if (args[1] != NULL) {
    write(STDERR_FILENO, error_message, strlen(error_message));
  } else {
    // Retrieve current working directory
    if (getcwd(pwd, sizeof(pwd)) == NULL) {
      write(STDERR_FILENO, error_message, strlen(error_message));
    } else {
      // Print current working directory
      cout << pwd << '\n';
    }
  }
  return 1;
}

// Built-in function: 'exit'
int mysh_exit(char **args)
{
  exit(0);
}

// Read the command line
char *mysh_read_cmdline(void)
{
  char *cmdline = NULL;
  size_t buffsize = 0;
  getline(&cmdline, &buffsize, stdin);
  return cmdline;
}

// Parse command line (with folowing delimiters)
#define MYSH_DELIMS " \t\r\n\a"
char **mysh_parse_cmdline(char *cmdline)
{
  int buffsize = 64;
  int i = 0;
  char *argument;
  char **orig_args;
  char **args = (char**) malloc(buffsize * sizeof(char*));
  // If memory cannot be allocated, print error message and exit
  if (!args) {
    write(STDERR_FILENO, error_message, strlen(error_message));
    exit(EXIT_FAILURE);
  }
  // Tokenize argument
  argument = strtok(cmdline, MYSH_DELIMS);
  while (argument != NULL) {
    args[i] = argument;
    i++;
    // Allocate bigger buffer, if needed
    if (i >= buffsize) {
      buffsize += 64;
      // Keep track of original arguments
      orig_args = args;
      // Reallocate meory
      args = (char**) realloc(args, buffsize * sizeof(char*));
      // If not reallocated, print error message and exit
      if (!args) {
		    free(orig_args);
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(EXIT_FAILURE);
      }
    }
    argument = strtok(NULL, MYSH_DELIMS);
  }
  args[i] = NULL;
  return args;
}

// Launch the process
int mysh_launch_process(char **args)
{
  pid_t process_id;
  int process_status;
  // Fork the process
  process_id = fork();
  if (process_id == 0) {
    // Execute child process
    if (execvp(args[0], args) == -1) {
      write(STDERR_FILENO, error_message, strlen(error_message));
    }
    exit(EXIT_FAILURE);
  } else if (process_id < 0) {
    // Unable to fork process, print error message
    write(STDERR_FILENO, error_message, strlen(error_message));
  } else {
    // Process waits
    do {
      waitpid(process_id, &process_status, WUNTRACED);
    } while (!WIFEXITED(process_status) && !WIFSIGNALED(process_status));
  }
  // Continue
  return 1;
}

// Execute command
#define MYSH_NUM_BUILTINS 3
int mysh_execute_process(char **args)
{
  int i;
  // If command is empty, continue
  if (args[0] == NULL) {
    return 1;
  }
  // Check to see if the command is built-in
  for (i = 0; i < MYSH_NUM_BUILTINS; i++) {
    if (strcmp(args[0], builtins_str[i]) == 0) {
      return (*builtins[i])(args);
    }
  }
  // Launch the process
  return mysh_launch_process(args);
}

// Interactive mode execution
#define MYSH_MAX_SIZE 512
void mysh_interactive_mode(void)
{
  char *cmdline;
  char **args;
  int status;

  do {
    // Print shell prompt
    printf("mysh> ");
    // Read command line
    cmdline = mysh_read_cmdline();
    // If the command line exceeds 512 characters, print error message
    if (strlen(cmdline) < MYSH_MAX_SIZE) {
      // Parse command line
      args = mysh_parse_cmdline(cmdline);
      // Execute command line
      status = mysh_execute_process(args);
      // Start a new shell prompt
      free(cmdline);
      free(args);
    } else {
      write(STDERR_FILENO, error_message, strlen(error_message));
      // Continue executing shell
      status = 1;
    }
  } while (status);
}

// Batch mode execution
void mysh_batch_mode(char *argv)
{
  ifstream batchFile(argv);
  char *cmdline;
  char **args;
  int status;
  string line;

  // Check to make sure file exists
  if (batchFile.is_open()) {
    // Execute each line in the batch file
    while (getline(batchFile, line)) {
      cmdline = &line[0];
      write(STDOUT_FILENO, cmdline, strlen(cmdline));
      cout << '\n';
      // Do not execute line if it exceeds 512 characters
      if (strlen(cmdline) < MYSH_MAX_SIZE) {
        args = mysh_parse_cmdline(cmdline);
        status = mysh_execute_process(args);
        // If status is 0, exit shell
        if (!status) {
          exit(0);
        }
      } else {
        write(STDERR_FILENO, error_message, strlen(error_message));
      }
    }
    // Close batch file
    batchFile.close();
  } else {
    write(STDERR_FILENO, error_message, strlen(error_message));
  }
}

// Main function (called when program is prompted to run)
int main(int argc, char *argv[])
{
  // If there are more than two arguments in the command line, print error message
  if (argc > 2) {
    write(STDERR_FILENO, error_message, strlen(error_message));
  } else {
    // If there is more than one argument, execute batch mode
    if (argc == 2) {
      mysh_batch_mode(argv[1]);
    } else {
      // If there is one argument, execute interactive shell
      mysh_interactive_mode();
    }
  }
  // End shell
  return EXIT_SUCCESS;
}
