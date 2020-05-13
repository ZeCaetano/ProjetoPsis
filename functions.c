//generic functions

#include "functions.h"

Uint32 Event_Update;

void *checked_malloc(size_t size){
    void *ptr;
    ptr = malloc (size);
    if(ptr == NULL){
        printf("Malloc error\n");
        exit(0);
    }
    return ptr;

}


void init_character(char_data *character, int type, int id, int r, int g, int b){
    character->pos[0] = 0;
    character->pos[1] = 0;
    character->color[0] = r;
    character->color[1] = g;
    character->color[2] = b;
    character->id = id;
    character->type = type;    
}

void push_update(char_data update, char_data previous){
    SDL_Event new_event;
    char_data *event_data[2];

    event_data[0] = malloc(sizeof(char_data));
    event_data[0]->pos[0] = update.pos[0];
    event_data[0]->pos[1] = update.pos[1];
    event_data[0]->color[0] = update.color[0];
    event_data[0]->color[1] = update.color[1];
    event_data[0]->color[2] = update.color[2];
    event_data[0]->type = update.type;    
    event_data[0]->id = update.id;
	event_data[1] = malloc(sizeof(char_data));
	event_data[1]->pos[0] = previous.pos[0];
	event_data[1]->pos[1] = previous.pos[1];

    SDL_zero(new_event);
    new_event.type = Event_Update;
    new_event.user.data1 = event_data[0];
    new_event.user.data2 = event_data[1];
    SDL_PushEvent(&new_event);
}