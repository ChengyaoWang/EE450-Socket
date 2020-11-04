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
#include <math.h>


char startsWith(const char *pre, const char *str){
    size_t lenpre = strlen(pre), lenstr = strlen(str);
    return lenpre > lenstr ? 0: memcmp(pre, str, lenpre) == 0;
}

double link_length = 0.;
int velocity = INT32_MAX;
int noise_power = INT32_MAX;
int signal_power = INT32_MAX;
int file_size = INT32_MAX;
int band_width = INT32_MAX;


int main(int argc, char *argv[]){
    int serverB_fd;
    struct sockaddr_in my_addr;
    struct sockaddr_in aws_addr;
    char buf[BUFSIZ];

    // Initialize the my Socket Addr
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(22000 + 816);
    memset(&aws_addr, 0, sizeof(aws_addr));
    aws_addr.sin_family = AF_INET;
    aws_addr.sin_addr.s_addr = INADDR_ANY;
    aws_addr.sin_port = htons(23000 + 816);

    // Initialize Server_a Sockets
    if((serverB_fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0){
        perror("Server B Socket Initialization Error, aborting ......");    return 1;
    }
    // Bind Port
    if(bind(serverB_fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) < 0){
        perror("Server B Port Binding Error, aborting ......");    return 1;
    }
    
    // Prepare itself
    socklen_t sin_size;
    int len, data_in_record = 0, queryIdx, computeResult;
    double C, tp, tt, tall;
    char replyMessage[100];
    fprintf(stdout, "The Server B is up and running using UDP on port 22816\n");

    while(1){

        if((len = recvfrom(serverB_fd, buf, BUFSIZ, 0, (struct sockaddr *)&aws_addr, &sin_size)) < 0){
            perror("Received Failure From AWS\n");     return 1;
        }

        // Now Everything in buf & aws_addr
        buf[len] = '\0';

        if(startsWith("exit", buf)){
            fprintf(stdout, "Stopping Signal Received, terminating Server B......\n");
            break;
        }
        else if(startsWith("test", buf)){
            fprintf(stdout, "test Command Received from AWS, Pinging Back\n");
            sendto(serverB_fd, "ACK", 4, 0, (struct sockaddr *)&aws_addr, sizeof(struct sockaddr));
            continue;
        }

        // Parse the Data
        if(sscanf(  buf, "%d %d %lf %d %d %d %d",
                    &queryIdx, &band_width, &link_length, &velocity, &noise_power, &file_size, &signal_power) < 7){
            perror("Data Parsing Failure in Server B\n");
            return 1;
        }

        fprintf(
            stdout, "The Server B received link information: link %d, file size %d, and signal power %d\n",
            queryIdx, file_size, signal_power
        );

        // Compute
        C = band_width * log2(1 + (double)signal_power / noise_power);
        tp = file_size / C / 1000;
        tt = 1000 * (double)link_length / velocity;
        tall = tp + tt;

        // Compose reply message
        sprintf(replyMessage, "%.2f %.2f %.2f", tp, tt, tall);
        len = sendto(
            serverB_fd, replyMessage, strlen(replyMessage), 0, (struct sockaddr *)&aws_addr, sizeof(struct sockaddr)
        );
        if(len < 0)     perror("Error Sending Compute Success to AWS, skipping\n");
    }

    close(serverB_fd);
    return 0;
}