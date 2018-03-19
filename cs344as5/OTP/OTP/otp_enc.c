

#include "protocol.h"

int fd, fd1, fd2;
char buffer[SIZE];
char temp[SIZE];
char cwd[SIZE], c;
int numRead, numWrite;
char port[100];
char addr[100];

static struct addrinfo *addr_list;

void handler(int num);
char *read_from_file(const char *filename);


//signal handler
void handler(int num){
    
    freeaddrinfo(addr_list); // free the linked-list
    exit(num);
}

//function to read from file
char *read_from_file(const char *filename)
{
    long int size = 0;
    FILE *file = fopen(filename, "r");
    
    if(!file) {
        fputs("File error.\n", stderr);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    rewind(file);
    
    char *result = (char *) malloc(size);
    if(!result) {
        fputs("Memory error.\n", stderr);
        return NULL;
    }
    
    if(fread(result, 1, size, file) != size) {
        fputs("Read error.\n", stderr);
        return NULL;
    }
    
    fclose(file);
    return result;
}


int main(int argc, char *argv[]){
    int status, i;
    struct addrinfo hints;
    
   
    signal(SIGINT, handler);
    //initializing
    memset(port, 0, 100 * sizeof(char));
    memset(addr, 0, 100 * sizeof(char));
    memset(buffer, 0, SIZE*sizeof(char));
    memset(temp, 0, SIZE*sizeof(char));
    
    
    //get port number
    strncpy(port, argv[3], 100);
    
    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    
    //getaddrinfo
    if((status = getaddrinfo("0.0.0.0", port, &hints, &addr_list))!= 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }
    
    //socket
    fd = socket(addr_list->ai_family, addr_list->ai_socktype, addr_list->ai_protocol);
    
    if(connect(fd, addr_list->ai_addr, addr_list->ai_addrlen)== -1){
        perror("could not connect");
        exit(EXIT_FAILURE);
    }
    
    while(1){
        
        //fgets(buffer, sizeof(buffer), stdin);
        
        
        char *str = read_from_file(argv[1]);
        char *str2 = read_from_file(argv[2]);
        
        
        
        //error handling
        if (strlen(str) > strlen(str2)) {
            printf("Error: key %s is too short", argv[2]);
            exit(1);
        }
        
        //error handling
        for (i = 33; i < 65; i++) {
            if(strchr(str, i)){
                errno = 1;
                perror("otp_enc error: input contains bad characters");
                exit(1);
            }
            
        }
        //error handling
        for (i = 92; i < 128; i++) {
            if(strchr(str, i)){
                errno = 1;
                perror("otp_enc error: input contains bad characters");
                exit(1);
            }
            
        }
        
        str2[strlen(str)-1] = '\0';
        
        
        
        
        //str[strlen(str)-1] = '\0';
        
        strcat(str, str2);
        
        
        
        //printf("send: %s567\n", str);
        
        //send to server
        numWrite = send(fd, str, strlen(str), 0);
        
        
        //receive from server
        numRead = recv(fd, buffer, SIZE, 0);
        if(numRead == 0){
            close(fd);
            pthread_exit(NULL);
        }
        printf("%s\n", buffer);
        
        
        memset(buffer, 0, SIZE*sizeof(char));
        memset(temp, 0, SIZE*sizeof(char));
        break;
    }
    return 0;
    
}