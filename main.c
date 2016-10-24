/******************************************************************************
 *
 *  File Name........: main.c
 *
 *  Description......: Simple driver program for ush's parser
 *
 *  Original copy author...........: Vincent W. Freeh
 *  Modified by : Durgesh Kumar Gupta (dgupta9@ncsu.edu)
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse.h"

//header files for changes
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <unistd.h>

#define READBUFFSIZE 8192
#define USHRC "/.ushrc"

char *cwd;

int tempInputPipefd=-1;

static int handleBuiltIn(char *cmd[]){
    if(!strcmp(cmd[0],"cd")){
        //handle cd command
        int i=1;
        while(cmd[i][0]=='-')   i++; // removing all options in cd
        
        if(chdir(cmd[i])){
            perror("Error in chdir");
            exit(-1);
        }
         return 1;  
    }else if(!strcmp(cmd[0],"cd")){
        
    }
    return 0;
}

static void prCmd_wasted(Cmd c){
    if(!strcmp(c->args[0],"end"))
        exit(0);
    int outputfd,inputfd,savedstdout,savedstdin,savedstderr;
    if ( c->in == Tin ){
        inputfd = open(c->infile,O_RDONLY);
        if(inputfd<=0){
            perror("INPUT FILE ERROR");
            exit(-1);
        }
        savedstdin = dup(0);
        dup2(inputfd,0);
    }
    if ( c->out != Tnil )
    switch ( c->out ) {
        case Tout:
            //redirect output to file
            outputfd = open(c->outfile,O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
            if(outputfd<=0){
                perror("OUTPUT IN FILE");
            }
            savedstdout = dup(1);
            dup2(outputfd,1);
            break;
        case Tapp:
            outputfd = open(c->outfile,O_WRONLY|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
            if(outputfd<=0){
                perror("OUTPUT IN FILE");
            }
            savedstdout = dup(1);
            dup2(outputfd,1);
            break;
        case ToutErr:
            outputfd = open(c->outfile,O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
            if(outputfd<=0){
                perror("OUTPUT IN FILE");
            }
            savedstdout = dup(1);
            savedstderr = dup(2);
            dup2(outputfd,1);
            dup2(outputfd,2);
            break;
        case TappErr:
             outputfd = open(c->outfile,O_WRONLY|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
            if(outputfd<=0){
                perror("OUTPUT IN FILE");
            }
            savedstdout = dup(1);
            savedstderr = dup(2);
            dup2(outputfd,1);
            dup2(outputfd,2);
            break;
        default:
            fprintf(stderr, "Shouldn't get here\n");
            exit(-1);
    }
    if(c->exec != Tamp){
        int status;
        //foreground process
        
        //check for builtin commands
        if(handleBuiltIn(c->args))
            return ;
        
        
        pid_t childPid = fork();
        if(childPid<0){
            perror("Shell error in fork");
            exit(-1);
        }else if(childPid == 0){
            //child process
            
            //handle pipes before execvp
            if((c->out==Tpipe)||(c->out==TpipeErr)){
                int pipe_fd[2]; // pipe b/w 2 commands
                if(pipe(pipe_fd)<0){
                    perror("Error in Pipe");
                    exit(-1);
                }
                // close input side of pipe
                close(pipe_fd[0]);
                dup2(pipe_fd[1],1);
                if(c->out==TpipeErr)
                    dup2(pipe_fd[1],2);
                if(tempInputPipefd != -1){
                    // previous command has some created pipe
                }
            }
            
            if(execvp(c->args[0],c->args)==-1){
                printf("Error in running command");
                exit(-1);
            }
        }else{
            //parent process
            
            // revert back the descriptor for parent class
            // first parent will always be shell
            
            if ( c->in == Tin ){
                dup2(savedstdin,0);
                close(inputfd);
                close(savedstdin);
                savedstdin = dup(0);
                dup2(inputfd,0);
            }
            
             if((c->out==Tpipe)||(c->out==TpipeErr)){
                // close input side of pipe
                close(pipe_fd[0]);
                dup2(pipe_fd[1],1);
                if(c->out==TpipeErr) 
                    dup2(pipe_fd[1],2);
            }
            
            if ( c->out != Tnil )
            switch ( c->out ) {
                case Tout:
                case Tapp:
                    //redirect output to file
                    dup2(savedstdout,1);
                    close(outputfd);
                    close(savedstdout);
                    break;
                case ToutErr:
                case TappErr:
                    dup2(savedstdout,1);
                    dup2(savedstderr,2);
                    close(outputfd);
                    close(savedstdout);
                    close(savedstderr);
                    break;
                case Tpipe:
                   printf("| ");
                   break;
                case TpipeErr:
                    printf("|& ");
                    break;
                default:
                    fprintf(stderr, "Shouldn't get here\n");
                    exit(-1);
            }
        }
    }
}

static void prCmd_2(Cmd c)
{
  int i;

  if ( c ) {
    printf("%s%s ", c->exec == Tamp ? "BG " : "", c->args[0]);
    if ( c->in == Tin )
      printf("<(%s) ", c->infile);
    if ( c->out != Tnil )
      switch ( c->out ) {
      case Tout:
	printf(">(%s) ", c->outfile);
	break;
      case Tapp:
	printf(">>(%s) ", c->outfile);
	break;
      case ToutErr:
	printf(">&(%s) ", c->outfile);
	break;
      case TappErr:
	printf(">>&(%s) ", c->outfile);
	break;
      case Tpipe:
	printf("| ");
	break;
      case TpipeErr:
	printf("|& ");
	break;
      default:
	fprintf(stderr, "Shouldn't get here\n");
	exit(-1);
      }

    if ( c->nargs > 1 ) {
      printf("[");
      for ( i = 1; c->args[i] != NULL; i++ )
	printf("%d:%s,", i, c->args[i]);
      printf("\b]");
    }
    putchar('\n');
    // this driver understands one command
    if ( !strcmp(c->args[0], "end") )
      exit(0);
  }
}

static void prPipe(Pipe p)
{
  int i = 0;
  Cmd c;

  if ( p == NULL )
    return;

  //printf("Begin pipe%s\n", p->type == Pout ? "" : " Error");
  for ( c = p->head; c != NULL; c = c->next ) {
    //printf("  Cmd #%d: ", ++i);
    prCmd(c);
  }
  //printf("End pipe\n");
  prPipe(p->next);
}

void handleSignal(int signalNum){
    //handle signal
    puts("\n");
    exit(0);
}

void readnrunrc(){
    char *resolved_path = (char*) malloc(sizeof(char)*PATH_MAX);
    strcpy(resolved_path,getenv("HOME")); // from http://stackoverflow.com/questions/2552416/how-can-i-find-the-users-home-dir-in-a-cross-platform-manner-using-c
	strcat(resolved_path,USHRC);
    int rcfd = open(resolved_path,O_RDONLY);
    if(rcfd<=0){
        //failed to read rc file
        perror("401");
        return;
    }
    //Trick parse into reading from std input 
    int stdinfd = dup(0);   //http://stackoverflow.com/questions/11042218/c-restore-stdout-to-terminal
    close(0);
    if(dup2(rcfd,0)<0){
        perror("error while redirecting stdin to file");
        error(-1);
    }
    
    Pipe p;
    while ( 1 ) {
        p = parse();
        if(!strcmp(p->head->args[0],"end")){
            freePipe(p);
            close(rcfd);
            break;
        }
        prPipe(p);
        freePipe(p);
    }
    
    // restore stdin
    dup2(stdinfd,0);
    close(stdinfd); 
    /*
    char buffer[READBUFFSIZE];
    int byteRead;
    while((byteRead = read(rcfd,buffer,sizeof(buffer)))>0){
        char *p = strtok(buffer,"\n");
        while(p != NULL){
            printf("\n#CMD: %s#\n",p);
            p = strtok(NULL,"\n");
        }
    }
    */
}

int main(int argc, char *argv[]){
    Pipe p;
    
    //get the host name
    size_t hostnamesize= HOST_NAME_MAX;
    char * host = (char *) malloc(sizeof(char) * hostnamesize);     //"armadillo";
    
    if(gethostname(host,hostnamesize))
        strcpy(host,"armadillo");
    
    //handle ctrl + c
    if (signal(SIGINT, handleSignal) == SIG_ERR)
        printf("shell can't catch SIGINT signal");
    
    // run ushrc
    //readnrunrc();
    
    
    //handle cd jobs
    //when cwd is changed but files are referenced relatively
    while ( 1 ) {
        printf("\n%s%% ", host);fflush(stdout);
        p = parse();
        prPipe(p);
        freePipe(p);
    }
}

/*........................ end of main.c ....................................*/
