#include <stdlib.h>
#include <string.h>
#include <stdio.h> 
#include <ctype.h> 
#include <stdint.h> 
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int open_clientfd(char *hostname, char *port);
int receive(int clientfd);
void quit();
void loop(int clientfd);

int open_clientfd(char *hostname, char *port)
{
    int clientfd;
    struct addrinfo hints, *listp, *p;
    
    // get a list of potential server addresses
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_NUMERICSERV; // numeric
    hints.ai_flags |= AI_ADDRCONFIG; // config
    getaddrinfo(hostname, port, &hints, &listp);

    // walk the list for one that we can successfully connect to
    for (p = listp; p; p = p->ai_next)
    {
        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
        {
            // socket failed, try next
            continue;
        }
        
        // socket succeeded

        // connect to server
        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1)
        {
            // success
            break;
        }
        
        // connect failed, try another
        close(clientfd);
    }
    
    // clean up
    freeaddrinfo(listp);
    if (!p)
    {
        // all connects failed, return -1
        return -1;
    }
    else
    {
        // last connect succeeded
        return clientfd;
    }
}

int receive(int clientfd)
{
    // buffer for incoming message
    // len "message"
    char buffer[256];

    ssize_t bytes_received = recv(clientfd, buffer, sizeof(buffer), 0);

    if (bytes_received <= 0)
    {
        // data not received
        printf("Error receiving data\n");
    }
    else
    {
        // data received
        int length = buffer[0];

        if (bytes_received != length + 1)
        {
            printf("Invalid message format\n");
        }
        else
        {
            // valid message format with length in the first byte
            char server_message[256];
            strncpy(server_message, &buffer[1], length);
            server_message[length] = '\0'; // Null-terminate the string

            // print server response
            printf("%s\n", server_message);
        }
    }
}

void quit()
{
    exit(0);
}

void loop(int clientfd) 
{
    char input[256];                // whole input that will be tokenized
    char input_copy[256];           // copy whole input to save original input
    char *words[256] = {NULL};      // args[0] = word, args[1] = word ...
    char *command;                  // = args[0] or First word of input
    char *arg;

    while (1) 
    {
        for(int i = 0; i < 256; i++) // initialize input each iteration to avoid null problem
        {
            input[i] = '\0';
        }
        
        printf("> "); // print out "> "

        
        fgets(input, sizeof(input), stdin);
        strcpy(input_copy, input); // copy input to input_copy
        arg = strtok(input, " \n\t");

        int i = 0;
        int max_index = 0;

        while (arg != NULL) // tokenize input to words,
        {
            words[i] = arg;
            max_index = i;
            i++;
            arg = strtok(NULL, " \n\t");
        }

        command = words[0]; // command = first word of input

        if (command == NULL)  // if it is just enter or nothing, then retry
        {
            // printf("Invalid syntax\n");
            continue;
        }

        // 22 Prices MSFT 2021-11-08
        size_t len = strlen(input_copy);

        char formatted_message[256];
        formatted_message[0] = (char)len; // Byte 0: Length of the string
        strncpy(&formatted_message[1], input_copy, len); // Bytes 1 - n: Characters of the string

        // Remove the last character '\n' if present
        if (formatted_message[len] == '\n') {
            formatted_message[len] = '\0'; // Replace '\n' with the null terminator
            len--; // Update the length
            formatted_message[0] = (char)len; // Update the length in byte 0
        }

        ssize_t bytes_sent;

        if (strcmp(command, "List") == 0) // List command
        {
            if (write(clientfd, formatted_message, len + 1) < 0)
            {
                // if everything isn't sent
                printf("Failed to send message\n");
                exit(1);
            }
            else
            {
                // sent succesfully. receive response from server.
                // receive(clientfd);
                bzero(input_copy, 256);
                read(clientfd, input_copy, sizeof(input_copy));

                size_t payload_length = (size_t)input_copy[0];
                // printf("payload_length = %ld\n", payload_length);

                char payload[256]; // Adjust size accordingly
 

                memcpy(payload, input_copy + 1, payload_length);
                payload[payload_length] = '\0'; // Null-terminate if it's a string

                printf("%s\n", payload);
            }
        } 
        else if (strcmp(command, "Prices") == 0) // Prices command
        {
            if (words[2] == NULL)
            {
                printf("Invalid Systax\n");
                continue;
            }

            int year, month, day;

            if(sscanf(words[2], "%d-%d-%d", &year, &month, &day) == 3 || sscanf(words[2], "%d-%02d-%02d", &year, &month, &day) == 3)
            {
                if (month > 12 || month < 1 || day > 31 || day < 1 || year < 1)
                {
                    printf("Invalid syntax\n");
                    continue;
                }

                if (write(clientfd, formatted_message, len + 1) < 0)
                {
                    // if everything isn't sent
                    printf("Failed to send message\n");
                    exit(1);
                }
                else
                {
                    // sent succesfully. receive response from server.
                    // receive(clientfd);
                    bzero(input_copy, 256);
                    read(clientfd, input_copy, sizeof(input_copy));
                    printf("%s\n", input_copy);
                }
            }
            else
            {
                printf("Invalid Systax\n");
            }
        } 
        else if (strcmp(command, "MaxProfit") == 0) // MaxProfit command
        {
            if (words[2] == NULL || words[3] == NULL)
            {
                printf("Invalid Systax\n");
                continue;
            }

            int year, month, day;
            if((sscanf(words[2], "%d-%d-%d", &year, &month, &day) == 3 || sscanf(words[2], "%d-%02d-%02d", &year, &month, &day) == 3)
            && (sscanf(words[3], "%d-%d-%d", &year, &month, &day) == 3 || sscanf(words[3], "%d-%02d-%02d", &year, &month, &day) == 3))
            {
                if (write(clientfd, formatted_message, len + 1) < 0)
                {
                    // if everything isn't sent
                    printf("Failed to send message\n");
                    exit(1);
                }
                else
                {
                    // sent succesfully. receive response from server.
                    // receive(clientfd);
                    bzero(input_copy, 256);
                    read(clientfd, input_copy, sizeof(input_copy));
                    printf("%s\n", input_copy);
                }
            }
            else
            {
                printf("Invalid Systax\n");
            }
        }
        else if (strcmp(command, "quit") == 0) // quit command
        {
            if (write(clientfd, formatted_message, len + 1) < 0)
            {
                // if everything isn't sent
                printf("Failed to send message\n");
                exit(1);
            }
            quit();
        } 
        else
        {
            printf("Invalid syntax\n");
        }
    }
}

int main(int argc, char** argv) {
    char* hostname = argv[1];
    char* port = argv[2];

    int clientfd;

    // open_clientfd returns clientfd
    if ((clientfd = open_clientfd(hostname, port)) == -1)
    {
        exit(1); // connection failed
    }

    // client is connected to the server
    loop(clientfd); // do infinite loop until typed "quit"
    return 0;
}