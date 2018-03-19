


#include "protocol.h"

int main(int argc, char *argv[]){
    char str[SIZE];
    
    memset(str, 0, SIZE*sizeof(char));
    
    // Length of the key
    int length = atoi(argv[1]);
    
    // Seed number for rand()
    srand((unsigned int) time(0) + getpid());
    
    // ASCII characters
    while(length--) {
        char randomletter = "ABCDEFGHIJKLMNOPQRSTUVWXYZ "[random () % 27];
        size_t len = strlen(str);
        str[len++] = randomletter; // we overwriting the null-character with another one
        str[len] = '\0'; // to make the string null-terminated again
        srand(rand());
    }
    
    printf("%s\n", str);
    
    return EXIT_SUCCESS;
}