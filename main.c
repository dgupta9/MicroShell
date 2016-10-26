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

int childListCount=0;

Cmd cmdList[100];
int pipeinputlist[100];
int pipeinputlist1[100];
int pipeCount=0;

static void strip(char *s)
{ //copied finnw from http://stackoverflow.com/questions/1515195/how-to-remove-n-or-t-from-a-given-string-in-c
    char *p = s;
    int n;
    while (*s)
    {
        n = strcspn(s, "\n");
        strncpy(p, s, n);
        p += n;
        s += n + strspn(s+n, "\n");
    }
    *p = 0;
}

char *trimwhitespace(char *str)
{
    // copied from http://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}


static int getMyPipeInput(Cmd c){
    int i;
    for(i=0;i<pipeCount;i++){
        if(cmdList[i]==c){
            return i;
        }
    }
    return -1;    
}

static int handleBuiltIn(char *cmd[],int nargs){
    //do some input cleaning due to '\' in command line
    int j;
    for(j=1;j<nargs;j++){;
                 strip(cmd[j]);
    }
    
    if(!strcmp(cmd[0],"cd")){
        //handle cd command
        int i=1;
        while((i<nargs)&&(cmd[i][0]=='-'))   i++; // removing all options in cd
        if(chdir(cmd[i])){
            perror("Error in chdir");
            exit(-1);
        }
         return 1;  
    }else if(!strcmp(cmd[0],"echo")){
        int i=1;
        while(cmd[i][0]=='-')   i++; // removing all options in echo
        
        
        
    }
    return 0;
}

static void prCmd(Cmd c,Cmd nextCmd){
    if(!strcmp(c->args[0],"end"))
        exit(0);
    
    int outputfd,inputfd,savedstdout,savedstdin,savedstderr;
    int pipefd[2];
    
    
    if(nextCmd==NULL){
        if(handleBuiltIn(c->args,c->nargs)){
            // close the if any previous commands output pipe
            int pindex = getMyPipeInput(c);
            if(pindex != -1){
                close(pipeinputlist1[pindex]);
                int devNull = open("/dev/null", O_WRONLY);
                //close(pipeinputlist[pindex]);
                dup2(pipeinputlist[pindex],devNull);
                //close(devNull);
            }
            return;
        }
    }
    
    if((c->out==Tpipe)||(c->out==TpipeErr)){
        pipe(pipefd);
        cmdList[pipeCount]=nextCmd;
        pipeinputlist[pipeCount]=pipefd[0];
        pipeinputlist1[pipeCount++]=pipefd[1];
    }
    pid_t childPid = fork();
    if(childPid<0){
        perror("Shell error in fork");
        exit(-1);
    }else if(childPid == 0){
        //child process
        signal(SIGPIPE, SIG_IGN);
        //handle INPUT Redirection
        if ( c->in == Tin ){
            inputfd = open(c->infile,O_RDONLY);
            if(inputfd<=0){
                perror("INPUT FILE ERROR");
                exit(-1);
            }
            savedstdin = dup(0);
            dup2(inputfd,0);
        }
        
        //handle pipe input
        int pindex = getMyPipeInput(c);
        if(pindex != -1){
            close(pipeinputlist1[pindex]);
            close(0);
            dup2(pipeinputlist[pindex],0);
        }
        
        
        // handle OUTPUT Redirection
        if ( c->out != Tnil )
            switch ( c->out ) {
                case Tout:
                    //redirect output to file
                    outputfd = open(c->outfile,O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
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
                    outputfd = open(c->outfile,O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
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
                case Tpipe:
                    close(pipefd[0]);
                    close(1);
                    dup2(pipefd[1],1);
                    break;
                case TpipeErr:
                    close(pipefd[0]);
                    close(1);
                    close(2);
                    dup2(pipefd[1],1);
                    dup2(pipefd[1],2);
                    break;
                default:
                    fprintf(stderr, "Shouldn't get here\n");
                    exit(-1);
            }
        //printf("Running command %s\n",c->args[0]);
        
            
            
            //check for builtin commands
            if(handleBuiltIn(c->args,c->nargs))
                return ;
            
            //do some input cleaning due to '\' in command line
            int nargs = c->nargs;
            int j;
            for(j=1;j<nargs;j++){;
                         strip(c->args[j]);
            }
        
            if(execvp(c->args[0],c->args)==-1){
                printf("Error in running command\n");
                exit(-1);
            }
        
        exit(0);
    }else{
        //set the child process id
        childListCount++;
        
        //parent shell process
         if((c->out==Tpipe)||(c->out==TpipeErr)){
                close(pipefd[1]);
         }
        
         int pindex = getMyPipeInput(c);
         if(pindex != -1){
             close(pipeinputlist[pindex]);
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
    if(c->args[0]!=NULL&&c->args[0][0]=='#')
        return;
    prCmd(c,c->next);
  }
  while((childListCount--)>0){
      wait(NULL); //https://www.daniweb.com/programming/software-development/threads/419275/fork-n-process-and-wait-till-all-children-finish-before-parent-resume
  }
  childListCount=0;
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
        exit(-1);
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
        printf("%s%% ", host);
        fflush(stdout);
        p = parse();
        prPipe(p);
        freePipe(p);
    }
}

/*........................ end of main.c ....................................*/
