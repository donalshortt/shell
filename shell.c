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

//void handle_pipe(node_t*);

void do_nothing_signal_handler(){ return; }

void initialize(void){
  signal(SIGINT, do_nothing_signal_handler);

  if (prompt)
      prompt = "vush$ ";
}

void run_command(node_t* node)
{
  /* For testing: */
  //print_tree(node);

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
      //handle_pipe(node);
      break;
    case NODE_DETACH:
      // run_command(node->detach.child);
      break;
    case NODE_REDIRECT:
      break;
    case NODE_SUBSHELL:
      pid_t subshell = fork();

      if (subshell == 0) {
          run_command(node->subshell.child);
      }
      if ()
      break;
  }
}

_Bool handle_builtin_cmd(node_t* n){
  if (strcmp(n->command.program, "exit") == 0) {
    exit(42);
  } else if (strcmp(n->command.program, "cd") == 0) {
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
        if (execvp(n->command.program, n->command.argv) == -1){
          perror("No such file or directory");
          return;
        exit(0);
    } else do {
       if ((proc_id = waitpid(proc_id, &status, WNOHANG)) == 0){
         sleep(.5);
       }
    } while (proc_id == 0);
  }
}

void handle_seq(node_t* n){
  handle_cmd(n->sequence.first);

  if (n->sequence.second->type == NODE_COMMAND) {
    handle_cmd(n->sequence.second);
  } else {
    handle_seq(n->sequence.second);
  }
}

//void handle_pipe(node_t* n){
  //FILE* fpin;
  //FILE* fpout;
  //print_tree(n);

  // for (unsigned int i = 0; i < n->pipe.n_parts; i++) {
  //   //if(n->pipe.parts[i]->type == NODE_COMMAND) {
  //     //fpin = popen(argvec, "w");
  //     //free(argvec);
  //     //pclose(fpin);
  //   //}
  // }

  // FILE* fpin = popen(n->pipe.parts[0] vec_s->command.program, "w");
  // FILE* fpout1 = popen(n->pipe.parts[0]->command.program, "r");
  // FILE* fpin2 = popen(n->pipe.parts[1]->command.program, "w");
  // fputs(n->pipe.parts[0]->command.argv[1], fpin1);
  // splice(fpout1, NULL, fpin2, NULL);
  // pclose(fpin1);
  // pclose(fpin2);
  // pclose(fpout1);
//}
