#include <stdlib.h>
#include <unistd.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "parser/ast.h"
#include "shell.h"

#define TRUE 1
#define FALSE 0

void handle_cmd(node_t*);

void handle_seq(node_t*);

void handle_pipe(node_t*);

void handle_subshell(node_t*);

void do_nothing_signal_handler(){ return; }

void initialize(void){
  signal(SIGINT, do_nothing_signal_handler);

  if (prompt)
      prompt = "vush$ ";
}

void run_command(node_t* node)
{
  /* For testing: */
  // print_tree(node);
  // printf("<--->\n");

  if (prompt)
      prompt = "vush$ ";

  switch (node->type){
    case NODE_COMMAND:
      handle_cmd(node);
      break;
    case NODE_SEQUENCE:
      handle_seq(node);
      break;
    case NODE_PIPE:
      handle_pipe(node);
      break;
    case NODE_DETACH:
      //run_command(node->detach.child);
      break;
    case NODE_REDIRECT:
      break;
    case NODE_SUBSHELL:
      handle_subshell(node);
      break;
  }
}

_Bool handle_builtin_cmd(node_t* n){
  if (strcmp(n->command.program, "exit") == 0) {
    if (n->command.argc > 1){
      exit(atoi(n->command.argv[1]));
    }
    exit(42);
  } else if (strcmp(n->command.program, "cd") == 0) {
    // printf("<!>Directory changed<!>\n");
    // printf("<!>PID: %d\n", getpid());
    chdir(n->command.argv[1]);
    return TRUE;
  }
  return FALSE;
}

void handle_cmd(node_t* n){
  if (handle_builtin_cmd(n)) { return; }

  int status;
  pid_t proc_id = fork();

  if (proc_id == 0) {
    // printf("<!>Command execution<!>\n");
    // printf("<!>PID: %d\n", getpid());
    if (execvp(n->command.program, n->command.argv) == -1){
      perror("No such file or directory");
      return;
    }
    exit(0);
  } else {
    waitpid(proc_id, &status, 0);
  }
}

void handle_seq(node_t* n){
  run_command(n->sequence.first);

  if (n->sequence.second->type == NODE_COMMAND) {
    run_command(n->sequence.second);
  } else {
    handle_seq(n->sequence.second);
  }
}

void handle_pipe(node_t* n){
  int pipe_ends[2];
  pipe(pipe_ends);

  pid_t child_pid = fork();

  if(child_pid == 0){
    dup2(pipe_ends[1], STDOUT_FILENO);
    close(pipe_ends[1]);
    run_command(n->pipe.parts[0]);
    abort();
  } else {
    dup2(pipe_ends[0], STDIN_FILENO);
    close(pipe_ends[1]);
    run_command(n->pipe.parts[1]);
  }
}

void handle_subshell(node_t* n){
  int status;
  pid_t subshell = fork();

  if (subshell == 0) {
    run_command(n->subshell.child);
    exit(0);
  } else {
    waitpid(subshell, &status, 0);
  }
}
