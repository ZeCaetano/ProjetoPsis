#include <SDL2/SDL.h>
#include <pthread.h>
//gcc teste.c UI_library.c -o teste-UI -lpthread -lSDL2 -lSDL2_image

#include "UI_library.h"


// this variable will contain the identifier for our own event type
Uint32 Event_ShowSomething;
//this data will be sent with the event
typedef struct Event_ShowSomething_Data{
	int x, y;
} Event_ShowSomething_Data;


void * thread_something(void * arg){
	int x, y;
	SDL_Event event;
	Event_ShowSomething_Data * event_data;

	while(1){

		//define the position of the next thing
		x = random()%50;
		y = random()%20;

		//create the data that will contain the new thing position
		event_data = malloc(sizeof(Event_ShowSomething_Data));
		event_data->x = x;
		event_data->y = y;

		// clear the event data
		SDL_memset(&event, 0, sizeof(event)); /* or SDL_zero(event) */
		// define event type
		event.type = Event_ShowSomething;
		//assign the event data
		event.user.data1 = event_data;
		// send the event
		SDL_PushEvent(&event);
		usleep(5000);
	}
}

int main(){

	SDL_Event event;
	int done = 0;

	//creates a windows and a board with 50x20 cases
	create_board_window(50, 20);

	Event_ShowSomething =  SDL_RegisterEvents(1);

	//monster and packman position
	int x = 0;
	int y = 0;
	//variable that defines what color to paint the monsters
	int click = 0;

	pthread_t thread_id;
	pthread_create(&thread_id, NULL,thread_something, NULL);



	while (!done){
		while (SDL_PollEvent(&event)) {
			if(event.type == SDL_QUIT){
				done = SDL_TRUE;
			}
			// if the event is of type Event_ShowSomething
			if(event.type == Event_ShowSomething){
				// we get the data (created with the malloc)
				Event_ShowSomething_Data * data = event.user.data1;
				// retrieve the x and y
				int x = data->x;
				int y = data->y;
				// we paint a thing
				paint_powerpacman(data->x, data->y, 255, 7, 7);
			}
			//if the event is of type mousebuttondown
			if(event.type == SDL_MOUSEBUTTONDOWN){
				int x, y;
				get_board_place(event.button.x, event.button.y,	&x, &y);
				clear_place(x, y);
			}
		}
	}
	printf("fim\n");
	close_board_windows();
}
