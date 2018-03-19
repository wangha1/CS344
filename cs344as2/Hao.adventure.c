//
//  Hao.adventure.c
//  344_P2
//
//  Created by Hao on 16/10/20.
//  Copyright © 2016 assignment2. All rights reserved.
//

#include <dirent.h>
#include <pwd.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>

#define MIN_CONN 3
#define MAX_CONN 6
#define NUM_ROOMS 7
#define NUM_ROOM_NAMES 10
#define NAME_BUFFER_LEN 16
#define NUM_THREADS 2

const char *room_names[NUM_ROOM_NAMES] = {
    "Winterfell",//John Snow for the king!
    "Riverrun",
    "Casterly Rock",
    "Highgarden",
    "Harrenhal",
    "Castle Black",
    "Nightfort",
    "King's Landing",
    "Valyria",
    "Volantis"
};


enum room_type {
    START_ROOM,
    END_ROOM,
    MID_ROOM
};

// The room struct. A single node in the graph.
struct room {
    enum room_type type;
    const char *name;
    int cap_conns;
    int num_conns;
    struct room *connections[NUM_ROOMS];
};

struct room rooms_list[NUM_ROOMS];//global accessed room list

void players_interface();
struct room *generate_rooms();
int create_rooms_dir(char *dir_name);
void write_rooms(struct room rooms[NUM_ROOMS]);
bool connect_room(int room1, int room2,struct room rooms_list[NUM_ROOMS]);
void print_all_connections(struct room *r);
struct room* read_rooms();
void destroy_rooms(struct room *rooms);
char* get_dir_name();



void writeTimeFunction(){
    pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&myMutex);
    char outstr[200];
    time_t t;
    struct tm *tmp;
    t = time(NULL);
    tmp = localtime(&t);
    if (tmp == NULL) {
        perror("localtime");
        exit(EXIT_FAILURE);
    }
    if (strftime(outstr, sizeof(outstr), "%I:%M%p, %A, %B %d, %Y", tmp) == 0) {
        fprintf(stderr, "strftime returned 0");
        exit(EXIT_FAILURE);
    }
    char *dir_name = get_dir_name();
    chdir(dir_name);
    FILE *fp = fopen("currentTime.txt", "w");
    fprintf(fp, "%s\n", outstr);
    fclose(fp);
    chdir("..");
    free(dir_name);
    pthread_mutex_unlock(&myMutex);
    return;
}

int main() {
    srand(time(0));
    generate_rooms();
    write_rooms(rooms_list);
    struct room *readed = read_rooms();
    players_interface();
    destroy_rooms(readed);
    return 0;
}

//Just call this function whenever games over
//print everything you want!
void ending_message(int num_steps, struct room **visited, int visited_index) {
    printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
    printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", num_steps);
    int i = 0;
    for (i; i < visited_index; i++) {
        printf("%s\n", visited[i]->name);
    }
}

//players interface function, I would like to call it main function actually
void players_interface() {
    //start initialize
    struct room *current_room = &rooms_list[0];//set the current room as first room(always the start room
                                                //the order of room is randomly assigned
    
    struct room **visited = malloc(sizeof(struct room*) * NUM_ROOMS);// this will save your path
    int visited_index = 0;
    int visited_cap = NUM_ROOMS;
    char buffer[NAME_BUFFER_LEN];
    int num_steps = 0;
    //end initialize
    
    while (1) {//loop forever
        
    Top:
        if (current_room->type == END_ROOM) {
            ending_message(num_steps, visited, visited_index);
            free(visited);
            return;
            //return if we reach end room
        }
        printf("\nCURRENT LOCATION: %s\n", current_room->name);
        print_all_connections(current_room);
        printf("\nWHERE TO? >");
        char *ret = fgets(buffer, NAME_BUFFER_LEN, stdin);
        assert(ret != NULL);
        buffer[strlen(buffer)-1] = '\0';
        //delete the newline on the end
        if(strncmp(buffer, "time", NAME_BUFFER_LEN) == 0){//open a new thread here
            pthread_t thread1;
            pthread_create(&thread1, NULL, writeTimeFunction, NULL);
            pthread_join(thread1, NULL);//thread done here
            char *dir_name = get_dir_name();
            chdir(dir_name);
            FILE *fp = fopen("currentTime.txt", "r");//read in data
            char line[256];
            while (fgets(line, sizeof(line), fp)){
                
            }
            printf("\n%s", line);//print last line
            fclose(fp);
            
        }
        else{
            int i = 0;
            for (i; i < current_room->num_conns; i++) {
                //search your room list to find if there's a room users wanted
                if (strncmp(buffer, current_room->connections[i]->name, NAME_BUFFER_LEN) == 0) {
                    current_room = current_room->connections[i];//if there is one room, move onto it
                
                    if (visited_index >= visited_cap) {
                        visited_cap = visited_cap * 2;// reallocate memory if the list is no large enough
                                                    //means you take more than 7 steps
                                                    //what a luck
                        visited = realloc(visited,visited_cap * sizeof(struct room*));
                        assert(visited != NULL);
                    }
                    visited[visited_index] = current_room;//save your path
                    visited_index++;
                    num_steps++;
                    goto Top;//go back to Top
                }
            }
        printf("\nHUH? I DON’T UNDERSTAND THAT ROOM. TRY AGAIN.\n");//if you can't find that room
                                                                    //someone must be kidding you
        }
    }
}

//the function will generate and connect the rooms
struct room *generate_rooms() {
    bool taken_names[NUM_ROOMS];
    memset(&taken_names, false, NUM_ROOMS * sizeof(bool));//initial at false, if name taken, set to true
    int i = 0;
    for (i; i < NUM_ROOMS; i++) {
        rooms_list[i].num_conns = 0;
        int cap_conns = 3 + (int) (rand() % 4);//a random number between 3 - 6 (3 + (0 - 3))
        rooms_list[i].cap_conns = cap_conns;
        while (1) {
            int room_index = rand() % NUM_ROOM_NAMES;//pick a name from your 10 items name list
            if (taken_names[room_index] == false) {
                taken_names[room_index] = true;//set the picked one as true. prevent same name
                rooms_list[i].name = room_names[room_index];//assign the name
                break;
            }
            
        }
        rooms_list[i].type = MID_ROOM;//set all the room as mid room
                                    //will set first room as first room
                                    //and last room as last room(end room) at the end
                                    //sounds funny :)
    }
    //the for loop here is all the connection happended
    i = 0;
    int j = 0;
    for (i; i < NUM_ROOMS; i++) {
        for (j; j < rooms_list[i].cap_conns; j++) {//go through all your list
            int random_room = rand() % NUM_ROOMS;//pick a random room and connect them
            while (!connect_room(i, random_room, rooms_list)) {
                random_room = rand() % NUM_ROOMS;
            }
        }
    }
    
    rooms_list[0].type = START_ROOM;//because the name is randomly assigned, so the start and end room
                                    //is also randomly assigned
    
    rooms_list[NUM_ROOMS - 1].type = END_ROOM;
    
    return rooms_list;
}

//return true if already connected
bool already_connected(unsigned int room1, unsigned int room2) {
    if (room1 == room2) {//exactly same room
        return true;
    }
    int i = 0;
    for (i; i < rooms_list[room1].num_conns; i++) {
        if (rooms_list[room1].connections[i] == &rooms_list[room2] && rooms_list[room1].connections[i] != NULL) {// already connected
            return true;
        }
    }
    return false;
}

//connect room, return true after connected, false if something bad happended so you can't connect them
bool connect_room(int room1, int room2, struct room rooms_list[NUM_ROOMS]) {
    struct room *r1 = &rooms_list[room1];
    struct room *r2 = &rooms_list[room2];
    //if the room already reach its capabilty of connections
    //automatically finish connect
    if (r1->num_conns == r1->cap_conns) {
        return true;
    }
    //if already connected, you don't need to connect them again
    if (already_connected(room1, room2)) {
        return false;
    }
    //don't connect over 6 times
    //it looks like never happended because I check the number of connections and capability before
    //but it do happend
    //magic
    if (r1->num_conns >= MAX_CONN || r2->num_conns >= MAX_CONN) {
        return false;
    }
    
    assert(r1 != NULL);
    assert(r2 != NULL);
    //here's what the connection really happended
    r1->connections[r1->num_conns] = r2;
    r2->connections[r2->num_conns] = r1;
    r1->num_conns++;
    r2->num_conns++;
    assert(r1->connections[r1->num_conns-1] != NULL);
    assert(r2->connections[r2->num_conns-1] != NULL);
    return true;
}

//just make things easier
char* get_dir_name() {
    pid_t pid = getpid();
    uid_t uid = getuid();
    struct passwd *user_info = getpwuid(uid);
    unsigned long buffer_max_len = strlen(".rooms.") + strlen(user_info->pw_name) + 10;
    char *dir_name = malloc(buffer_max_len * sizeof(char));
    assert(dir_name != NULL);
    sprintf(dir_name, "%s.rooms.%d", user_info->pw_name, pid);
    
    return dir_name;
}

//write your room into file
void write_rooms(struct room rooms[NUM_ROOMS]) {
    char *dir_name = get_dir_name();
    mkdir(dir_name, 0777);
    chdir(dir_name);
    int i = 0;
    for (i; i < NUM_ROOMS; i++) {
        FILE *fp = fopen(rooms[i].name, "w");
        fprintf(fp, "ROOM NAME: %s\n", rooms[i].name);
        int j = 0;
        for (j; j < rooms[i].num_conns; j++) {
            fprintf(fp, "CONNECTION %d: %s\n", j + 1, rooms[j].name);
        }
        
        switch (rooms[i].type) {
            case END_ROOM:
                fprintf(fp, "ROOM TYPE: END_ROOM");
                break;
            case MID_ROOM:
                fprintf(fp, "ROOM TYPE: MID_ROOM");
                break;
            case START_ROOM:
                fprintf(fp, "ROOM TYPE: START_ROOM");
                break;
        }
        fclose(fp);
    }
    chdir("..");
    free(dir_name);
}

//helper function when you are going to read the room back
const char *pick_right_name(char *in) {
    int i = 0;
    for (i; i < MAX_CONN; i++) {
        if (strcmp(in, room_names[i]) == 0) {
            return room_names[i];
        }
    }
    return NULL;
}

//another helper function
struct room *pick_right_room(char *in) {
    int i = 0;
    for (i; i < NUM_ROOMS; i++) {
        if (strcmp(in, rooms_list[i].name) == 0) {
            return &rooms_list[i];
        }
    }
    return NULL;
}


void iter_rooms(struct room *r, void (*func)(struct room *)) {
    int i = 0;
    for (i; i < r->num_conns; i++) {
        func((r->connections[i]));
    }
}


void print_all_connections(struct room *r) {
    printf("POSSIBLE CONNECTIONS: ");
    int i = 0;
    for (i; i < r->num_conns - 1; i++) {
        printf("%s, ", r->connections[i]->name);
    }
    printf("%s.", r->connections[r->num_conns - 1]->name);
}


struct room read_room_helper(char *name) {
    struct room r;
    FILE *file = fopen(name, "r");

    char received_name[NAME_BUFFER_LEN];
    fscanf(file, "ROOM NAME: %s\n", name);
    r.name = pick_right_name(name);
    
    int read;
    int conn_number;
    while ((read = fscanf(file, "CONNECTION %d: %s\b", &conn_number, received_name)) != 0 && read != EOF) {
        r.connections[conn_number-1] = pick_right_room(received_name);
    }
    r.num_conns = conn_number - 1;
    fscanf(file, "ROOM TYPE: %s\n", received_name);
    if (strcmp(name, "START_ROOM") == 0) {
        r.type = START_ROOM;
    } else if (strcmp(name, "END_ROOM") == 0) {
        r.type = END_ROOM;
    } else if (strcmp(name, "MID_ROOM") == 0) {
        r.type = MID_ROOM;
    } else {
    
    }
    fclose(file);
    return r;
}

//read room back to program
struct room* read_rooms() {
    struct room *rooms = malloc(NUM_ROOMS * sizeof(struct room));
    unsigned int room_count = 0;
    char *dir_name = get_dir_name();
    chdir(dir_name);
    DIR *dp;
    struct dirent *dir;
    dp = opendir (".");
    assert(dp != NULL);
    
    while ((dir = readdir (dp))) {
        rooms[room_count] = read_room_helper(dir->d_name);
    }
    
    closedir (dp);
    
    free(dir_name);
    chdir("..");
    return rooms;
}

void destroy_rooms(struct room *rooms) {
    free(rooms);
}
