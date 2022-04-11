#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
//initiate scutil child process to watch for ipv4 changes
//for now, scutil should be killed by init
//Lets just assume on parent's exit, this child is killed
//
const int BUFFSIZE = 32;
extern int errno;

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

    if(fcntl(fd[0], F_SETFL, O_NONBLOCK))
        exit(2);
    *cpid = fork();

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
    } //parent
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
    printf("attempted to fast forward\n");
}
//invoke scutil
int startup(){
    int pipe[2];
    pid_t cpid;
    char output[1025];
    // set stdin to listen to scutil


    pipe[0] = scutil_listener(pipe,&cpid); //handle pipe? fd[0] is read and open


        fflush(stdout);
    //stolen code
    sleep(2); // wait for scutil to launch

    while(1){
        int nread = read(pipe[0], output, 1024);
        output[1024] = '\0';
        switch (nread) {
            case -1:
                // case -1 means pipe is empty and errono
                // set EAGAIN
                if (errno == EAGAIN) {
                    printf("(pipe empty)\n");
                    sleep(1);
                    break;
                }
                else {
                    perror("read");
                    exit(4);
                }
                // case 0 means all bytes are read and EOF(end of conv.)
            case 0:
                printf("End of conversation\n");
                // read link
                close(pipe[0]);
                kill(cpid,SIGTERM);
                exit(0);
            default:
                // text read
                // by default return no. of bytes
                // which read call read at that time
                printf("scutil says \"%s \"\n", output);



        }
        sleep(5);
        fflush(stdout);
    }

    printf("child's id : %d \n",cpid);
    fflush(stdout);
    sleep(2);
    kill(cpid,SIGTERM);

    return 1;
}
