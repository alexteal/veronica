#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
//initiate scutil child process to watch for ipv4 changes
//for now, scutil should be killed by init
//Lets just assume on parent's exit, this child is killed
//
const int BUFFSIZE = 32;

char* resize(char* s, unsigned int size){
    if(size<1){
        return "";
    }
    s = realloc(s,size);
    if (s)
        return s;
    printf("\n REALLOC FAILURE \n");
    return "";
}

char* read_pipe(int buffer_size,int fd){
    unsigned int size = lseek(fd,SEEK_END,0);
    lseek(fd,0,0);
    char *s = malloc(size*sizeof(char)+sizeof(char*));
    char buff[buffer_size];
    unsigned int index = 0;
    int n;
    while ((n = read(fd, &buff, buffer_size))>0){
        for(unsigned short i = 0; i<n; i++){
            s[index++] = buff[i];
            printf("%s","reading");
        }//assigned buffer to s
    } 
    s[index] = '\0'; //close string
    return s;
}

//fork off scutil and set stdin to 
int scutil_listener(int* fd,pid_t* cpid){
    pipe(fd); // 0 is read, 1 is write
    *cpid = fork();
    printf("child's pid is %d\n",*cpid);
    fflush(stdout);
    if (*cpid == -1){
        write(1,"error!",10);
        exit(0);
    } //error handling
    if (*cpid == 0){
        printf("scutil taking over child now\n");
        fflush(stdout);
        //should set stdout to pipe. stolen from stackoverflow
        dup2 (fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
       
        //takeover
        char* args[] = {"scutil","-W","-r","State:/Network/Global/IPv4",NULL};
        execvp(args[0],args);

    } //child
    else {
        close(fd[1]);
        return fd[0];
        //fd[0] is open 
        // dup(fd[0]);
    } //parent!
    return fd[0];
}

void fast_forward(int pipe, char* output){
    int i = 0;
    int n;
    while((n=read(pipe,output,1))>0){
        output[2] = '\0';
        printf("loop count = %d \n",i++);
        printf("child output \n%s\n",output);
        fflush(stdout);
    }
}
//invoke scutil
int startup(){
    int pipe[2];
    pid_t cpid;
    char output[1024];
    // set stdin to listen to scutil
    

    pipe[0] = scutil_listener(pipe,&cpid); //handle pipe? fd[0] is read and open
    read(pipe[0],output,1000);

    fast_forward(pipe[0],output);

    //ready to continue after flushing listener
    int n;
    int i = 0;
    while((n=read(pipe[0],output,64))>0){       output[65] = '\0';
        printf("loop count = %d \n",i++);
        printf("child output \n%s\n",output);
        fflush(stdout);
    }


    printf("child's id : %d \n",cpid);
    fflush(stdout);
    sleep(2);
    kill(cpid,SIGTERM);

    return 1;
}
