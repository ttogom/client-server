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
#include <math.h>
#include <stdbool.h>

#define MAXLINE 256
#define LISTENQ 1

#define MAX_LINE_SIZE 1024 // total row size
#define MAX_FIRST_WORD_LENGTH 255 // YYYY-MM-DD
#define MAX_FIFTH_WORD_LENGTH 255 // xxx.xxxx
#define NUM_ROWS 301
#define MAX(x, y) (((x) < (y)) ? (y) : (x))

bool tsla = false;
bool msft = false;

struct Csv 
{
    char date[MAX_FIRST_WORD_LENGTH];
    char closing_price[MAX_FIFTH_WORD_LENGTH];
    int row_number;
};

struct Csv MSFT[NUM_ROWS];
struct Csv TSLA[NUM_ROWS];

typedef struct sockaddr SA;

int open_listenfd(char *port);
void process(int connfd);
char* find_price_MSFT(char *date);
char* find_price_TSLA(char *date);
char* max_profit_MSFT(char *start_date, char *end_date);
bool parsecsv(char *file);


int open_listenfd(char *port)
{
    struct addrinfo hints, *listp, *p;
    int listenfd, optval=1;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    hints.ai_flags |= AI_NUMERICSERV;
    getaddrinfo(NULL, port, &hints, &listp);

    for (p = listp; p; p = p->ai_next)
    {
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
        {
            continue;
        }

        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
        {
            break;
        }
        close(listenfd);
    }

    freeaddrinfo(listp);
    if (!p)
    {
        return -1;
    }

    if (listen(listenfd, LISTENQ) < 0)
    {
        close(listenfd);
        return -1;
    }

    return listenfd;
}

void process(int connfd)
{
    size_t n;
    char buf[MAXLINE];
    

    while ((n = read(connfd, buf, MAXLINE)) > 0)
    {
        /* GET PAYLOAD */
        // printf("server received %d bytes\n", (int)n);

        size_t payload_length = (size_t)buf[0];
        char payload[MAXLINE]; // Adjust size accordingly
        // Copy payload to a new buffer, excluding the length byte
        memcpy(payload, buf + 1, payload_length);
        payload[payload_length] = '\0'; // Null-terminate if it's a string

        // Now use the payload...
        if (strcmp(payload, "quit") != 0)
        {
            printf("%s\n", payload);
        }
        // printf("%s\n", payload);
        /* END GET PAYLOAD */


        /* PROCESS PAYLOAD */
        char *words[256] = {NULL};      // args[0] = word, args[1] = word ...
        char *command;                  // = args[0] or First word of input
        char *arg;

        arg = strtok(payload, " \n\t");

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
            continue;
        }
        /* END PROCESS PAYLOAD */


        /* EXECUTE COMMAND*/
        if (strcmp(command, "List") == 0) // List command
        {
            char msg[256];

            /* EXECUTE */
            if (tsla == true && msft == true)
            {
                strcpy(msg, "TSLA | MSFT");
            }
            else if (tsla == true && msft == false)
            {
                // printf("HELLO\n");
                // fflush(stdout);
                strcpy(msg, "TSLA");
            }
            else if (tsla == false && msft == true)
            {
                strcpy(msg, "MSFT");
            }

            /* SEND MESSAGE */
            size_t len = strlen(msg);

            char formatted_message[256];
            formatted_message[0] = (char)len; // Byte 0: Length of the string
            strncpy(&formatted_message[1], msg, len); // Bytes 1 - n: Characters of the string
            formatted_message[len + 1] = '\0'; // null terminate

            ssize_t bytes_sent;

            if (write(connfd, formatted_message, len + 1) < 0)
            {
                printf("Error writing to socket\n");
                break;
            }
        } 
        else if (strcmp(command, "Prices") == 0) // Prices command
        {
            char msg[256];
            char rounded_string[32];
            char date[MAX_FIRST_WORD_LENGTH];
            bool date_found = false;

            if ((strcmp(words[1], "TSLA") == 0) && tsla)
            {
                // strcpy(msg, find_price_TSLA(words[2])); 
                for(int rowCounter = 0; rowCounter < NUM_ROWS; rowCounter ++)
                {
                    if(strcmp(TSLA[rowCounter].date, words[2]) == 0)
                    {
                        date_found = true;

                        double original_price = strtod(TSLA[rowCounter].closing_price, NULL);
                        double rounded_price = round(original_price * 100) / 100;
                        // char rounded_string[32];
                        sprintf(rounded_string, "%.2f", rounded_price);

                        // return rounded_string;        
                    }
                }

                if (!date_found)
                {
                    strcpy(msg, "Unknown");
                }
                else
                {
                    strcpy(msg, rounded_string);
                }
            }
            else if ((strcmp(words[1], "MSFT") == 0) && msft)
            {
                // strcpy(msg, find_price_TSLA(words[2])); 
                for(int rowCounter = 0; rowCounter < NUM_ROWS; rowCounter ++)
                {
                    if(strcmp(MSFT[rowCounter].date, words[2]) == 0)
                    {
                        date_found = true;

                        double original_price = strtod(MSFT[rowCounter].closing_price, NULL);
                        double rounded_price = round(original_price * 100) / 100;
                        // char rounded_string[32];
                        sprintf(rounded_string, "%.2f", rounded_price);

                        // return rounded_string;        
                    }
                }

                if (!date_found)
                {
                    strcpy(msg, "Unknown");
                }
                else
                {
                    strcpy(msg, rounded_string);
                }
            }
            else if ((strcmp(words[1], "TSLA") == 0) && !tsla)
            {
                strcpy(msg, "Unknown");
            }
            else if ((strcmp(words[1], "MSFT") == 0) && !msft)
            {
                strcpy(msg, "Unknown");
            }
            else if ((strcmp(words[1], "TSLA") != 0) && (strcmp(words[1], "MSFT") != 0))
            {
                strcpy(msg, "Unknown");
            }

            /* SEND MESSAGE */
            size_t len = strlen(msg);

            char formatted_message[256];
            formatted_message[0] = (char)len; // Byte 0: Length of the string
            strncpy(&formatted_message[1], msg, len); // Bytes 1 - n: Characters of the string
            formatted_message[len + 1] = '\0'; // null terminate

            ssize_t bytes_sent;

            if (write(connfd, formatted_message, len + 1) < 0)
            {
                printf("Error writing to socket\n");
                break;
            }
        } 
        else if (strcmp(command, "MaxProfit") == 0) // MaxProfit command
        {
            char msg[256];
            char rounded_string[32];
            char date1[MAX_FIRST_WORD_LENGTH];
            char date2[MAX_FIRST_WORD_LENGTH];
            double rounded_price = 0;
            bool date1_found = false;
            bool date2_found = false;

            if ((strcmp(words[1], "TSLA") == 0) && tsla)
            {
                double max_profit = 0;
                int start_row, end_row;

                for(int i = 0; i < NUM_ROWS; i ++)
                {
                    if(strcmp(TSLA[i].date, words[2]) == 0)
                    {
                        start_row = i;
                        date1_found = true;
                    }

                    if(strcmp(TSLA[i].date, words[3]) == 0)
                    {
                        end_row = i;
                        date2_found = true;
                    }
                }

                for(int i = start_row; i < end_row + 1; i ++)
                {
                    double buying_price = strtod(TSLA[i].closing_price, NULL);

                    for(int j = i; j < end_row + 1; j ++)
                    {
                        double selling_price = strtod(TSLA[j].closing_price, NULL);

                        if(selling_price > buying_price)
                        {
                            double profit = selling_price - buying_price;
                            max_profit = MAX(max_profit, profit);
                        }
                    }
                }

                rounded_price = round(max_profit * 100) / 100;
                sprintf(rounded_string, "%.2f", rounded_price);
                
                if (!date1_found || !date2_found)
                {
                    strcpy(msg, "Unknown");
                }
                else if(start_row > end_row)
                {
                    strcpy(msg, "Unknown");
                }
                else
                {
                    strcpy(msg, rounded_string);
                }
            }
            else if ((strcmp(words[1], "MSFT") == 0) && msft)
            {
                double max_profit = 0;
                int start_row, end_row;

                for(int i = 0; i < NUM_ROWS; i ++)
                {
                    if(strcmp(MSFT[i].date, words[2]) == 0)
                    {
                        start_row = i;
                        date1_found = true;
                    }

                    if(strcmp(MSFT[i].date, words[3]) == 0)
                    {
                        end_row = i;
                        date2_found = true;
                    }
                }

                for(int i = start_row; i < end_row + 1; i ++)
                {
                    double buying_price = strtod(MSFT[i].closing_price, NULL);

                    for(int j = i; j < end_row + 1; j ++)
                    {
                        double selling_price = strtod(MSFT[j].closing_price, NULL);

                        if(selling_price > buying_price)
                        {
                            double profit = selling_price - buying_price;
                            max_profit = MAX(max_profit, profit);
                        }
                    }
                }

                rounded_price = round(max_profit * 100) / 100;
                sprintf(rounded_string, "%.2f", rounded_price);

                if (!date1_found || !date2_found)
                {
                    strcpy(msg, "Unknown");
                }
                else if(start_row > end_row)
                {
                    strcpy(msg, "Unknown");
                }
                else
                {
                    strcpy(msg, rounded_string);
                }
            }
            else if ((strcmp(words[1], "TSLA") == 0) && !tsla)
            {
                strcpy(msg, "Unknown");
            }
            else if ((strcmp(words[1], "MSFT") == 0) && !msft)
            {
                strcpy(msg, "Unknown");
            }
            else if ((strcmp(words[1], "TSLA") != 0) && (strcmp(words[1], "MSFT") != 0))
            {
                strcpy(msg, "Unknown");
            }
            
            /* SEND MESSAGE */
            size_t len = strlen(msg);

            char formatted_message[256];
            formatted_message[0] = (char)len; // Byte 0: Length of the string
            strncpy(&formatted_message[1], msg, len); // Bytes 1 - n: Characters of the string
            formatted_message[len + 1] = '\0'; // null terminate

            ssize_t bytes_sent;

            if (write(connfd, formatted_message, len + 1) < 0)
            {
                printf("Error writing to socket\n");
                break;
            }
        }
        else if (strcmp(command, "quit") == 0) // quit command
        {
            close(connfd);
            exit(0);
        }
        /* END EXECUTE COMMAND*/
    }

    if (n < 0) {
        printf("Error reading from socket\n");
    }
}

bool parsecsv(char *file_name)
{
    /* parse csv file here*/
    FILE *file = fopen(file_name, "r");

    char line[MAX_LINE_SIZE];
    char *file_words[100] = {NULL};

    if (file == NULL) 
    {
        printf("Error opening file\n");
        fflush(stdout);
        return false;
    }

    if (strcmp(file_name, "MSFT.csv") == 0)
    {
        for (int rowCounter = 0; rowCounter < NUM_ROWS; rowCounter++) 
        {
            if (fgets(line, sizeof(line), file) == NULL) // reading line, if fails, error message
            {
                printf("Error reading line %d\n", rowCounter + 1);
                fflush(stdout);
                fclose(file);
                return false;
            }

            line[strcspn(line, "\n")] = '\0';  // Remove the newline character

            char *token = strtok(line, ",");
            int file_words_index = 0;

            while (token != NULL) // tokenize input to words,
            {
                file_words[file_words_index] = token;
                token = strtok(NULL, ",");
                file_words_index++;
            }

            strcpy(MSFT[rowCounter].date, file_words[0]);
            strcpy(MSFT[rowCounter].closing_price, file_words[4]);
            MSFT[rowCounter].row_number = rowCounter;
        }

        fclose(file); // close file for MSFT
    }
    else if (strcmp(file_name, "TSLA.csv") == 0)
    {
    
        for (int rowCounter = 0; rowCounter < NUM_ROWS; rowCounter++) 
        {
            if (fgets(line, sizeof(line), file) == NULL) // reading line, if fails, error message
            {
                printf("Error reading line %d\n", rowCounter + 1);
                fflush(stdout);
                fclose(file);
                return false;
            }

            line[strcspn(line, "\n")] = '\0';  // Remove the newline character

            char *token = strtok(line, ",");
            int file_words_index = 0;

            while (token != NULL) // tokenize input to words,
            {
                file_words[file_words_index] = token;
                
                // printf("%s\n", file_words[file_words_index]);

                token = strtok(NULL, ",");
                file_words_index++;
            }

            strcpy(TSLA[rowCounter].date, file_words[0]);
            strcpy(TSLA[rowCounter].closing_price, file_words[4]);
            TSLA[rowCounter].row_number = rowCounter;
        }

        fclose(file);
    }

    return true;
}

int main(int argc, char** argv) 
{
    int files_count = 0;
    if ((strcmp(argv[1], "TSLA.csv") == 0))
    {
        files_count++;
        tsla = true;
    }
    else if ((strcmp(argv[1], "MSFT.csv") == 0))
    {
        files_count++;
        msft = true;
    }

    if ((strcmp(argv[2], "TSLA.csv") == 0))
    {
        files_count++;
        tsla = true;
    }
    else if ((strcmp(argv[2], "MSFT.csv") == 0))
    {
        files_count++;
        msft = true;
    }

    /* parse csv file */
    if (files_count == 1)
    {
        if (parsecsv(argv[1]) == false)
        {
            printf("Error parsing file: %s\n", argv[1]);
        }
    }
    else if (files_count == 2)
    {
        if (parsecsv(argv[1]) == false)
        {
            printf("Error parsing file.%s\n", argv[1]);
        }
        if (parsecsv(argv[2]) == false)
        {
            printf("Error parsing file.%s\n", argv[2]);
        }
    }

    // get the port number
    char *port = argv[argc - 1];

    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE];
    char client_port[MAXLINE];

    listenfd = open_listenfd(port);

    while (1)
    {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
        getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        
        printf("server started\n");
        fflush(stdout);

        process(connfd);
        close(connfd);
    }

    return 0;
}