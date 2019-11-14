#include <stdlib.h>
#include <unistd.h>
#include <linux/limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "parser/ast.h"
#include "shell.h"


void initialize(void)
{
  /* This code will be called once at startup */
  if (prompt)
      prompt = "vush$ ";
}

void handle_cmd(node_t*);

void handle_seq(node_t*);

void run_command(node_t* node)
{
  /* For testing: */
  //print_tree(node);

  if (prompt)
      prompt = "vush$ ";

  switch (node->type) {
    case NODE_COMMAND:
      handle_cmd(node);
      break;
    case NODE_SEQUENCE:
      handle_seq(node);
      break;
    case NODE_PIPE:
      break;
    case NODE_DETACH:
      break;
    case NODE_REDIRECT:
      break;
    case NODE_SUBSHELL:
      break;
  }
}

_Bool handle_builtin_cmd(node_t* n){
  if (strcmp(n->command.program, "exit") == 0) {
    exit(42);
  } else if (strcmp(n->command.program, "cd") == 0) {
    chdir(n->command.argv[1]);
    return 1;
  }
  return 0;
}

void handle_cmd(node_t* n) {
  if (handle_builtin_cmd(n)) { return; }

  int status;
  pid_t proc_id = fork();
  if (proc_id == -1) {
      printf("Failed to fork off :(");
      return;
  } else if (proc_id == 0) {
      execvp(n->command.program, n->command.argv);
      exit(0);
  } else do {
     if ((proc_id = waitpid(proc_id, &status, WNOHANG)) == 0){
       sleep(.5);
     }
  } while (proc_id == 0);
}

void handle_seq(node_t* n) {
  handle_cmd(n->sequence.first);

  if (n->sequence.second->type == NODE_COMMAND) {
    handle_cmd(n->sequence.second);
  } else {
    handle_seq(n->sequence.second);
  }
}
