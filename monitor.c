#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#include <assert.h>


// Local file Descriptor
int monitor2aws_fd;

// Socket Addrs
struct sockaddr_in aws_addr;

char startsWith(const char *pre, const char *str){
    size_t lenpre = strlen(pre), lenstr = strlen(str);
    return lenpre > lenstr ? 0: memcmp(pre, str, lenpre) == 0;
}

int main(int argc, char *argv[]){

    // Socket
    monitor2aws_fd = socket(PF_INET, SOCK_STREAM, 0);

    // Addrs 
    memset(&aws_addr, 0, sizeof(struct sockaddr_in));
    aws_addr.sin_family = AF_INET;
    aws_addr.sin_addr.s_addr = INADDR_ANY;
    aws_addr.sin_port = htons(25000 + 816);

    // Establish TCP Connection
    socklen_t sin_size = sizeof(struct sockaddr_in);
    assert(connect(monitor2aws_fd, (struct sockaddr *)&aws_addr, sizeof(struct sockaddr)) >= 0);
    
    
    // Prepare itself
    int len;
    char buf[BUFSIZ];
    fprintf(stdout, "Monitor is up and running\n");
    
    
    while(1){
        assert((len = recv(monitor2aws_fd, buf, BUFSIZ, 0)) > 0);
        buf[len] = '\0';

        if(startsWith("exit", buf)){
            fprintf(stdout, "Stopping Signal Received, terminating monitor......\n");
            break;
        }
        else if(startsWith("test", buf)){
            fprintf(stdout, "test Command Received from AWS, Pinging Back\n");
            send(monitor2aws_fd, "ACK", 4, 0);
        }
        else{
            fprintf(stdout, "Operation ACK from AWS, LinkNode Concerned: %s\n", buf);
        }
    }

    // Close All FDs
    close(monitor2aws_fd);
    return 0;
}



