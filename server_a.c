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

# define sockaddr_s sizeof(struct sockaddr)

int data_in_record = 0;
double link_length = 0.;
int velocity = INT32_MAX;
int noise_power = INT32_MAX;
int signal_power = INT32_MAX;
int file_size = INT32_MAX;
int band_width = INT32_MAX;

const char filename[] = "database.txt";

char startsWith(const char *pre, const char *str){
    size_t lenpre = strlen(pre), lenstr = strlen(str);
    return lenpre > lenstr ? 0: memcmp(pre, str, lenpre) == 0;
}


// Add Record to the database
int addRecord(const char* line){
    FILE* fd = fopen(filename, "a");
    fputs(line , fd);
    fputs("\n", fd);
    fclose(fd);
    return 0;
}

// Fetch Record from the database
int fetchRecord(char* line, int queryIdx){
    if(queryIdx > data_in_record)
        return 0;
    FILE* fd = fopen(filename, "r");
    for(int i = 0; i < queryIdx; ++i){
        fgets(line, BUFSIZ, fd);
    }
    fclose(fd);
    return 1;
}

int main(int argc, char *argv[]){
    int serverA_fd;
    struct sockaddr_in my_addr;
    struct sockaddr_in aws_addr;
    char buf[BUFSIZ];

    // Initialize Socket Addr
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(21000 + 816);
    memset(&aws_addr, 0, sizeof(aws_addr));
    aws_addr.sin_family = AF_INET;
    aws_addr.sin_addr.s_addr = INADDR_ANY;
    aws_addr.sin_port = htons(23000 + 816);


    // Initialize Server_a Sockets & Bind Port for Receiving
    if((serverA_fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0){
        perror("Server A Socket Initialization Error, aborting ......");    return 1;
    }
    if(bind(serverA_fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) < 0){
        perror("Server A Port Binding Error, aborting ......");    return 1;
    }
    
    // Prepare itself
    socklen_t sin_size;
    int len, queryIdx, computeResult;
    char replyMessage[BUFSIZ];
    fprintf(stdout, "The Server A is up and running using UDP on port 21816\n");

    while(1){
        // Receive UDP Socket
        len = recvfrom(serverA_fd, buf, BUFSIZ, 0, (struct sockaddr *)&aws_addr, &sin_size);
        buf[len] = '\0';
        
        // Start Processing
        if(startsWith("write", buf)){
            fprintf(stdout, "The Server A received input for writing\n");
            sscanf(buf, "%s %d %lf %d %d", replyMessage, &band_width, &link_length, &velocity, &noise_power);

            // Add to Record
            data_in_record++;
            sprintf(buf, "%d %d %f %d %d", data_in_record, band_width, link_length, velocity, noise_power);
            addRecord(buf);
            fprintf(stdout, "The Server A wrote link %d to database\n", data_in_record);

            // Reply yo AWS
            sprintf(replyMessage, "%d", data_in_record); 
            len = sendto(serverA_fd, replyMessage, strlen(replyMessage), 0, (struct sockaddr *)&aws_addr, sockaddr_s);
        }
        else if(startsWith("compute", buf)){
            sscanf(buf, "%s %d", replyMessage, &queryIdx);
            fprintf(stdout, "The Server A received input %d for computing\n", queryIdx);
            computeResult = fetchRecord(buf, queryIdx);

            // Compose ACK Message
            if(computeResult == 1){
                sprintf(replyMessage, "%d %s", computeResult, buf);
                fprintf(stdout, "Link ID Found\n");
            }
            else{
                sprintf(replyMessage, "%d %d", computeResult, queryIdx);
                fprintf(stdout, "Link ID not found\n");
            }
            
            // Send Back to AWS
            len = sendto(serverA_fd, replyMessage, strlen(replyMessage), 0, (struct sockaddr *)&aws_addr, sockaddr_s);
        }
        else if(startsWith("exit", buf)){
            fprintf(stdout, "Stopping Signal Received, terminating server A......\n");
            break;
        }
        else if(startsWith("test", buf)){
            fprintf(stdout, "test Command Received from AWS, Pinging Back\n");
            sendto(serverA_fd, "ACK", 4, 0, (struct sockaddr *)&aws_addr, sizeof(struct sockaddr));
        }
        else{
            fprintf(stdout, "Data Received not recognized, skipping\n");
        }

    }

    close(serverA_fd);
    return 0;
}