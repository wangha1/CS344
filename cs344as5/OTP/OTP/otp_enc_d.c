
#include "protocol.h"

static pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
int fd, fd1, fd2;
char buf[SIZE];
char temp[SIZE];
int numRead, numWrite;
char port[100], c;
FILE *file_stream;



static struct addrinfo *addr_list;

void handler(int num);
static void * threadFunc(void *arg);

//signal handler
void handler(int num){
    
    freeaddrinfo(addr_list); // free the linked-list
    exit(num);
}

//thread function
static void * threadFunc(void *arg){
    fd = *(int *)arg;
    int i = 0;
    
    pthread_detach(pthread_self());
    
    for(; ;){
        
        //initializing
        memset(buf, 0, SIZE*sizeof(char));
        memset(temp, 0, SIZE*sizeof(char));
        
        //receive from client
        numRead = recv(fd, buf, SIZE, 0);
        if(numRead == 0){
            close(fd);
            pthread_exit(NULL);
        }
        //sleep(10);
        
        //printf(":::%s", buf);
        
//        char qolon = '\n';//character to search
//        char *quotPtr = strchr(buf, qolon);
//
//        int position = quotPtr - buf;
//        char* attrValue = (char*) malloc((position + 1) * sizeof(char));
//        memcpy(attrValue, buf, position);
//        attrValue[position] = '\0';
//        
//        printf(":::!%s\n", attrValue);
        
        
        
        //parsing the string received
        char * separator = "\n";
        char * b = strtok(buf, separator);
        //printf("b: %s\n", b);
        char * c = strtok(NULL, "");
        //printf("c: %s\n", c);
        
        //perform the actual encoding
        for(i = 0; i < strlen(b); i++){
            if (c[i] == ' ') {
                if (b[i] == 32 )
                    b[i] = 91;
                b[i] = ((b[i]-65+26)%27)+65;
                if (b[i] == 91 )
                    b[i] = 32;
            }
            
            else{
                if (b[i] == 32 )
                    b[i] = 91;
                b[i] = ((b[i]-65+(c[i]-65))%27)+65;
                if (b[i] == 91 )
                    b[i] = 32;
            }
            
        }
        
        
        //send the encrypted code to client
        numWrite = send(fd, b, strlen(buf), 0);
        
    }
}

int main(int argc, char *argv[]){
    int status;
    struct addrinfo hints;
    int sockfd, new_fd;
    struct sockaddr_storage their_addr;
    socklen_t len;
    
    //signal handling
    signal(SIGINT, handler);
    signal(SIGPIPE, SIG_IGN);
    
    //initializing
    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
    memset(port, 0, 100 * sizeof(char));
    
    //set port number
    strncpy(port, argv[1], 100);
    
    //getaddrinfo
    if((status = getaddrinfo(NULL, port, &hints, &addr_list))!= 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }
    
    //socket
    sockfd = socket(addr_list->ai_family, addr_list->ai_socktype, addr_list->ai_protocol);
    
    if(bind(sockfd, addr_list->ai_addr, addr_list->ai_addrlen)== -1){
        perror("listen socket bind");
        exit(EXIT_FAILURE);
    }
    listen(sockfd, 5);
    char ipstr[INET6_ADDRSTRLEN];
    
    void *addr;
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)addr_list->ai_addr;
    addr = &(ipv4->sin_addr);
    inet_ntop(addr_list->ai_family, addr, ipstr, sizeof ipstr);
    //printf(" :: %s\n", ipstr);
    
    //thread
    for(; ;){
        pthread_t thread;
        len = sizeof their_addr;
        new_fd = accept(sockfd,(struct sockaddr *)&their_addr, &len);
        
        pthread_create(&thread, NULL, threadFunc, &new_fd);
        sleep(1);
        
    }
    
}