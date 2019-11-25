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

pid_t current_procss_id;

void handle_cmd(node_t*);

void handle_seq(node_t*);

void handle_pipe(node_t*);

void handle_detach(node_t*);

void handle_redirect(node_t*);

void handle_subshell(node_t*);

void do_nothing_signal_handler(){ return; }

void suspend_signal_handler(){
  //kill(current_procss_id, SIGTSTP);
}

void initialize(void){
  signal(SIGINT, do_nothing_signal_handler);
  signal(SIGTSTP, suspend_signal_handler);

  //current_procss_id = fork();

  // if(current_procss_id == 0) {
    if (prompt)
        prompt = "vush$ ";
  // } else if (current_procss_id > 0){
  //   waitpid(current_procss_id, 0, 0);
  //   exit(0);
  // }
}

void run_command(node_t* node){
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
      handle_detach(node);
      break;
    case NODE_REDIRECT:
      handle_redirect(node);
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
    exit(0);
  } else if (strcmp(n->command.program, "cd") == 0) {
    chdir(n->command.argv[1]);
    return TRUE;
  } else if (strcmp(n->command.program, "set") == 0){
    putenv(n->command.argv[1]);
    return TRUE;
  } else if (strcmp(n->command.program, "unset") == 0){
    unsetenv(n->command.argv[1]);
    return TRUE;
  } else {
    return FALSE;
  }
}

void handle_cmd(node_t* n){
  if (handle_builtin_cmd(n)) { return; }

  pid_t proc_id = fork();

  if (proc_id == 0) {
    if (execvp(n->command.program, n->command.argv) == -1){
      perror("No such file or directory");
      return;
    }
    exit(0);
  } else {
    waitpid(proc_id, 0, 0);
  }
}

void handle_seq(node_t* n){
  run_command(n->sequence.first);
  run_command(n->sequence.second);
}

void handle_pipe(node_t* n){
  unsigned int i;

  for (i = 0; i < (n->pipe.n_parts - 1); i++) {
    int pipe_ends[2];
    pipe(pipe_ends);

    pid_t child_pid = fork();

    if(child_pid == 0){
      dup2(pipe_ends[1], STDOUT_FILENO);
      close(pipe_ends[1]);
      run_command(n->pipe.parts[i]);
      exit(0);
    } else {
      dup2(pipe_ends[0], STDIN_FILENO);
      close(pipe_ends[1]);
    }
  }
  run_command(n->pipe.parts[i]);
}

void handle_detach(node_t* n){
  pid_t parent = fork();

  if (parent == 0){
    pid_t detached = fork();

    if (detached > 0){
      exit(0);
    }

    setpgid(0, 0);
    chdir("/");

    run_command(n->detach.child);
    exit(0);
  }
}

void handle_redirect(node_t* n){
  pid_t redirect = fork();

  if (redirect == 0) {
    switch(n->redirect.mode){
      case REDIRECT_DUP: ;
        break;
      case REDIRECT_INPUT: ;
        int input_fd = open(n->redirect.target, O_RDONLY | O_CREAT);

        printf("Input fd: %d\n", input_fd);

        int dup = dup2(input_fd, n->redirect.fd);

        printf("Input fd: %d\n", input_fd);

        if (dup < 0){
          printf("dup fail!\n");
        }

        close(n->redirect.fd);

        run_command(n->redirect.child);
        close(input_fd);
        break;
      case REDIRECT_OUTPUT: ;
        int output_fd = open(n->redirect.target, O_WRONLY);
        dup2(output_fd, n->redirect.fd);
        close(n->redirect.fd);
        run_command(n->redirect.child);
        close(output_fd);
        break;
      case REDIRECT_APPEND: ;
        break;
    }
  } else {
    waitpid(redirect, 0, 0);
  }
}

void handle_subshell(node_t* n){
  pid_t subshell = fork();

  if (subshell == 0) {
    run_command(n->subshell.child);
    exit(0);
  } else {
    waitpid(subshell, 0, 0);
  }
}
