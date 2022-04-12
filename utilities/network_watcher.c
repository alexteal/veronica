#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <string.h> /* for strncpy */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>




//stolen code
char* get_ip(char* host){

    struct ifaddrs *ifaddr, *ifa;
    int s;

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }


    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        s=getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in),host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

        if( (strcmp(ifa->ifa_name,"en0")==0)&&(  ifa->ifa_addr->sa_family==AF_INET)  )
        {
            if (s != 0)
            {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
            printf("\tInterface : <%s>\n",ifa->ifa_name );
            printf("\t  Address : <%s>\n", host);
            break;
        }
    }
    freeifaddrs(ifaddr);
    
    return host;
}



//initiate scutil child process to watch for ipv4 changes
//for now, scutil should be killed by init
//Lets just assume on parent's exit, this child is killed
//
extern int errno;
//fork off scutil and set stdin to 
int scutil_listener(int* fd,pid_t* cpid){

    pipe(fd); // 0 is read, 1 is write

    *cpid = fork();

    if (*cpid == -1){
        exit(0);
    } //error handling

    if (*cpid == 0){
        printf("scutil starting up...\n");
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
    int nread;

    while(1){
        nread = read(pipe[0], output, 1024);
        output[1024] = '\0';
        switch (nread) {
            case -1:
                // case -1 means pipe is empty and errono
                // set EAGAIN
                if (errno == EAGAIN) {
                    break;
                }
                else {
                    exit(4);
                }
                // case 0 means all bytes are read and EOF(end of conv.)
            case 0:
                // read link
                close(pipe[0]);
                kill(cpid,SIGTERM);
                exit(0);
            default:
                // text read
                // by default return no. of bytes
                // which read call read at that time
                ;  // i don't understand you C... smh
                char ip[NI_MAXHOST];
                get_ip(ip);
                printf(" IP UPDATE: %s \n",ip);
                /*
                 *
                 *  insert ip file update here
                 *
                 */

        }
    }
    sleep(2);
    kill(cpid,SIGTERM);

    return 1;
}
