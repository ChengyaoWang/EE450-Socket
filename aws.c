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
#define sockaddr_s sizeof(struct sockaddr)

double tp, tt, tall;
double link_length = 0.;
int velocity = INT32_MAX;
int noise_power = INT32_MAX;
int signal_power = INT32_MAX;
int file_size = INT32_MAX;
int band_width = INT32_MAX;


char startsWith(const char *pre, const char *str){
    size_t lenpre = strlen(pre), lenstr = strlen(str);
    return lenpre > lenstr ? 0: memcmp(pre, str, lenpre) == 0;
}


// Local file Descriptor
int aws2client_fd, client_fd;
int aws2moniter_fd, moniter_fd;
int aws2udp_fd;

// Socket Addrs
struct sockaddr_in my_addr_client, client_addr;
struct sockaddr_in my_addr_moniter, moniter_addr;
struct sockaddr_in serverA_addr, serverB_addr;
struct sockaddr_in my_addr_udp;


int sockaddr_init(void){
    // Initialize All Socket Addr
    memset(&my_addr_client, 0, sizeof(struct sockaddr_in));
    my_addr_client.sin_family = AF_INET;
    my_addr_client.sin_addr.s_addr = INADDR_ANY;
    my_addr_client.sin_port = htons(24000 + 816);
    memset(&my_addr_moniter, 0, sizeof(struct sockaddr_in));
    my_addr_moniter.sin_family = AF_INET;
    my_addr_moniter.sin_addr.s_addr = INADDR_ANY;
    my_addr_moniter.sin_port = htons(25000 + 816);
    memset(&my_addr_udp, 0, sizeof(struct sockaddr_in));
    my_addr_udp.sin_family = AF_INET;
    my_addr_udp.sin_addr.s_addr = INADDR_ANY;
    my_addr_udp.sin_port = htons(23000 + 816);

    memset(&serverA_addr, 0, sizeof(struct sockaddr_in));
    serverA_addr.sin_family = AF_INET;
    serverA_addr.sin_addr.s_addr = INADDR_ANY;
    serverA_addr.sin_port = htons(21000 + 816);
    memset(&serverB_addr, 0, sizeof(struct sockaddr_in));
    serverB_addr.sin_family = AF_INET;
    serverB_addr.sin_addr.s_addr = INADDR_ANY;
    serverB_addr.sin_port = htons(22000 + 816);
    return 0;
}
int socket_init(void){
    aws2client_fd = socket(PF_INET, SOCK_STREAM, 0);
    aws2moniter_fd = socket(PF_INET, SOCK_STREAM, 0);
    aws2udp_fd = socket(PF_INET, SOCK_DGRAM, 0);

    assert(aws2client_fd >= 0 || aws2moniter_fd >= 0 || aws2udp_fd >= 0);
    return 0;
}

int main(int argc, char *argv[]){

    // Initiate All sockaddr
    assert(sockaddr_init() == 0);

    // Initialize All Sockets
    assert(socket_init() == 0);

    // Bind All Port
    assert(bind(aws2client_fd, (struct sockaddr *)&my_addr_client, sockaddr_s) >= 0);
    assert(bind(aws2moniter_fd, (struct sockaddr *)&my_addr_moniter, sockaddr_s) >= 0);
    assert(bind(aws2udp_fd, (struct sockaddr *)&my_addr_udp, sockaddr_s) >= 0);

    // Establish 2 TCP Connections
    socklen_t sin_size = sizeof(struct sockaddr_in);
    assert(listen(aws2moniter_fd, 5) >= 0);
    assert((moniter_fd = accept(aws2moniter_fd, (struct sockaddr *)&moniter_addr, &sin_size)) >= 0);
    assert(listen(aws2client_fd, 5) >= 0);
    assert((client_fd = accept(aws2client_fd, (struct sockaddr *)&client_addr, &sin_size)) >= 0);


    // Prepare itself
    int len, queryIdx, computeResult;
    char replyMessage[BUFSIZ], buf[BUFSIZ];

    fprintf(stdout, "The AWS is up and running\n");

    while(1){
        // Receive TCP Socket From client
        len = recv(client_fd, buf, BUFSIZ, 0);
        buf[len] = '\0';
        
        // Start Processing
        if(startsWith("write", buf)){
            // Parse the Data
            sscanf(buf, "%s %d %lf %d %d", replyMessage, &band_width, &link_length, &velocity, &noise_power);
            // Send to Server A
            sendto(aws2udp_fd, buf, strlen(buf), 0, (struct sockaddr *)&serverA_addr, sockaddr_s);
            // Waiting for Server A's ACK
            len = recvfrom(aws2udp_fd, buf, BUFSIZ, 0, (struct sockaddr *)&serverA_addr, &sin_size);
            buf[len] = '\0';
            
            send(client_fd, buf, strlen(buf), 0);
            send(moniter_fd, buf, strlen(buf), 0);
        }
        else if(startsWith("compute", buf)){
            // Parse the Data
            sscanf(buf, "%s %d %d %d", replyMessage, &queryIdx, &file_size, &signal_power);
            // Send to Server A
            sprintf(buf, "%s %d", replyMessage, queryIdx);
            sendto(aws2udp_fd, buf, strlen(buf), 0, (struct sockaddr *)&serverA_addr, sockaddr_s);
            // Waiting for Server A's ACK
            len = recvfrom(aws2udp_fd, buf, BUFSIZ, 0, (struct sockaddr *)&serverA_addr, &sin_size);
            buf[len] = '\0';
            // Parse the Data
            if(startsWith("1", buf)){
                sscanf( buf, "%d %d %d %lf %d %d",
                        &computeResult, &queryIdx, &band_width, &link_length, &velocity, &noise_power);
                // Compose Data & Contact Server B
                sprintf(buf, "%d %d %lf %d %d %d %d",
                        queryIdx, band_width, link_length, velocity, noise_power, file_size, signal_power);
                // Send to Server B
                sendto(aws2udp_fd, buf, strlen(buf), 0, (struct sockaddr *)&serverB_addr, sockaddr_s);
                // Waiting Server B's ACK & Parse Data
                len = recvfrom(aws2udp_fd, buf, BUFSIZ, 0, (struct sockaddr *)&serverB_addr, &sin_size);
                buf[len] = '\0';
                sscanf(buf, "%lf %lf %lf", &tp, &tt, &tall);

                // Forward End-to-End Delay
                sprintf(replyMessage, "%d %lf", 0, tall);
                send(client_fd, replyMessage, strlen(replyMessage), 0);

                // Forward All Delay Delay
                sprintf(replyMessage, "%d %s", 0, buf);
                send(moniter_fd, replyMessage, strlen(replyMessage), 0);
            }
            else{
                sscanf(buf, "%d %d", &computeResult, &queryIdx);
                sprintf(replyMessage, "Compute Failed, Link %d not found", queryIdx);
                send(client_fd, replyMessage, strlen(replyMessage), 0);

                // Forward End-to-End Delay
                send(moniter_fd, replyMessage, strlen(replyMessage), 0);
            }
        }
        else if(startsWith("exit", buf)){
            strcpy(buf, "exit");
            fprintf(stdout, "Stopping Signal Received, terminating server A......\n");
            sendto(aws2udp_fd, buf, strlen(buf), 0, (struct sockaddr *)&serverA_addr, sockaddr_s);
            
            fprintf(stdout, "Stopping Signal Received, terminating server B......\n");
            sendto(aws2udp_fd, buf, strlen(buf), 0, (struct sockaddr *)&serverB_addr, sockaddr_s);
            
            fprintf(stdout, "Stopping Signal Received, terminating monitor......\n");
            send(moniter_fd, buf, strlen(buf), 0);
            
            fprintf(stdout, "Stopping Signal Received, terminating aws......\n");
            break;
        }
        else if(startsWith("test", buf)){
            fprintf(stdout, "test Command Received From Client......\n");
            // Monitor
            fprintf(stdout, "Pinged Monitor ......");
            send(moniter_fd, "test", 5, 0);
            len = recv(moniter_fd, buf, BUFSIZ, 0);
            buf[len] = '\0';
            fprintf(stdout, "ACK from Monitor Received\n");
            
            // Server A
            fprintf(stdout, "Pinged Server A ......");
            sendto(aws2udp_fd, "test", 5, 0, (struct sockaddr *)&serverA_addr, sizeof(struct sockaddr));
            len = recvfrom(aws2udp_fd, buf, BUFSIZ, 0, (struct sockaddr *)&serverA_addr, &sin_size);
            buf[len] = '\0';
            fprintf(stdout, "ACK from Server A Received\n");

            // Server B
            fprintf(stdout, "Pinged Server B ......");
            sendto(aws2udp_fd, "test", 5, 0, (struct sockaddr *)&serverB_addr, sizeof(struct sockaddr));
            len = recvfrom(aws2udp_fd, buf, BUFSIZ, 0, (struct sockaddr *)&serverB_addr, &sin_size);
            buf[len] = '\0';
            fprintf(stdout, "ACK from Server B Received\n");

            send(client_fd, "Test Complete, ACKing Back to Client\n", 38, 0);
        }
        else{
            fprintf(stdout, "Data Received not recognized, skipping\n");
            send(client_fd, "Rua\n", 5, 0);
        }

    }


    // Close All FDs
    close(aws2client_fd);
    close(client_fd);
    close(aws2moniter_fd);
    close(moniter_fd);
    close(aws2udp_fd);
    return 0;
}