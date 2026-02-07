//#include <unistd.h> 		// for file i/o
#include <string.h>			//
#include <ctype.h>			// for data validation
#include "main.h"

extern SDL_Window* mywindow;				// the usual window
extern screen_t screen;

typedef struct {
	char name[22];
	int points;
}ScoreData;

static void sort_it(ScoreData *s);			// sort the high score table
static void write_scores(ScoreData *s);		// output score table to file
static void get_player(ScoreData *s, char);	// get players name, integer for player number 1 0r 2

void high_scores(int p1score, int p2score, GLuint tex){

	int check, i, j;		// check - we can read data from scores.dat
	int quit = 0;
	char buffer[10];		// ascii score
	char c;
	int count;
	SDL_Event e;
	int row_count = 10;
	int y_top = (int)(screen.height * 0.30f);
	int y_bottom = (int)(screen.height * 0.88f);
	int y_step = (y_bottom - y_top) / (row_count - 1);
	if (y_step < 32)
		y_step = 32;
	int text_size = (screen.height > 1000) ? 24 : 20;
	ScoreData temp;			/* temporary struct */
	ScoreData s[10];		/* array of structs */
	ScoreData *s_ptr;
	SDL_RWops * highscores;
	play_sound(HYPER, 0);

	s_ptr = s;

	for (i = 0; i < 10 ;++i){			
		for (j = 0; j < 21; ++j)
			s[i].name[j] = '.';
		s[i].name[21] = '\0';
		s[i].points = 0;
	}

	highscores = SDL_RWFromFile("highscore.dat", "r");		/* open for reading, use rb for non text */

	if (highscores == NULL)								/* check file opens */
		return;

	// try to read up to 10 scores and names

	for (i = 0; i < 10; ++i) {
		count = 0;
		while (c != ' ') {
			check = SDL_RWread(highscores, &c, sizeof(char), 1);
			if (!check) break;
			s[i].name[count] = c;
			++count;
		}
		--count;
		s[i].name[count] = '\0';
		count = 0;
		while (c != '\n') {
			check = SDL_RWread(highscores, &c, sizeof(char), 1);
			if (!check) break;
			buffer[count] = c;
			++count;
		}
		--count;
		buffer[count] = '\0';
		s[i].points = atoi(buffer);
	}

	SDL_RWclose(highscores);

	// if players score is big enough take their name

	if ((p1score || p2score) > 0) {

		if (p1score > s[9].points) {
			for (j = 0; j < 21; ++j)	// init temp array
				temp.name[j] = '_';
			temp.name[21] = '\0';

			get_player(&temp, 1);
			strcpy(s[9].name, temp.name);
			s[9].points = p1score;
		}

		sort_it(s_ptr);	// re-sort with new player data now included

		if (p2score > s[9].points) {
			for (j = 0; j < 21; ++j)	// reinit temp array
				temp.name[j] = '_';
			temp.name[21] = '\0';

			get_player(&temp, 2);
			strcpy(s[9].name, temp.name);
			s[9].points = p2score;
		}

		sort_it(s_ptr); 	// re-sort with new player data now included

	}

	// underscore to space for display purposes

	for (i = 0; i < 10; ++i) {
		for (j = 0; j < 22; ++j) {
			if (s[i].name[j] == '_')
				s[i].name[j] = ' ';
		}
	}



	// Display score table
	
	while (!quit){
		
		glClear (GL_COLOR_BUFFER_BIT);
		draw_background(tex);
		glColor4f(1,1,1,1);
		draw_menu_item(screen.width/2, 150, "HOT SCORES", 2, 88);
		
		for (i = 0; i < 10; ++i){
				sprintf(buffer, "%d", s[i].points);
				int y = y_top + i * y_step;
				draw_menu_item(screen.width/3, y, s[i].name, 2, text_size);
				draw_menu_item(screen.width - screen.width/4, y, buffer, 2, text_size); 
			}
	
		while (SDL_PollEvent(&e) != 0){		// check for key event
	
			if(e.type == SDL_KEYDOWN){
				play_sound(FIRE, 0);
				quit = 1;
			}	
		}
		
		SDL_GL_SwapWindow(mywindow);
		SDL_Delay(50);
		
	}
	
	// underscore from space so scanf works 
	
	for (i = 0; i < 10 ;++i){
		for (j = 0; j < 22; ++j){
			if (s[i].name[j] == ' ')
				s[i].name[j] = '_';
		}			
	}
	
	Mix_FadeOutChannel(-1, 1000);
	write_scores(s_ptr);

}

// write score table to highscores.dat

void write_scores(ScoreData *s){
	
	int i;
	SDL_RWops * highscores;
	int size;
	char buffer[33];
	
	
	highscores = SDL_RWFromFile("highscore.dat", "w");
	
	for(i = 0; i < 10; ++i){
		size = sprintf(buffer, "%s %d\n", s[i].name, s[i].points);
		SDL_RWwrite(highscores, buffer, 1, size);	
	}
	
	SDL_RWclose(highscores);
}

// read players name
	
void get_player(ScoreData *s, char id){

	SDL_Event e;
	char buffer[14];
	int quit = 0;
	int current = 0;	// current position in name field

	sprintf(buffer, "ENTER NAME P%c", id);
	buffer[13]='\0';
	SDL_StartTextInput();	//Enable text input
	
	while (!quit){
		
		glClear (GL_COLOR_BUFFER_BIT);
//		draw_background(tex);
		glColor4f(1,1,1,1);
				
		

		while (SDL_PollEvent(&e) != 0){		// check for key event
			
			if(e.type == SDL_KEYDOWN){

				play_sound(FIRE, 0);
				if(e.key.keysym.sym == SDLK_BACKSPACE && current > 0){
					--current;					//lop off character
					s->name[current] = '_';
				}
				else if(e.key.keysym.sym == SDLK_RETURN){
					quit = 1;
					s->name[current] = '\0';
				}		
								
			}
			else if((e.type == SDL_TEXTINPUT) && (current < 21)){
					s->name[current] = e.text.text[0];
					++current;
			}	
		}
		draw_menu_item(screen.width/2, 150, buffer, 2, 64);
		draw_menu_item(screen.width/2 , screen.height/2, s->name, 2, 32);
		SDL_GL_SwapWindow(mywindow);
		SDL_Delay(50);
		
	}

	SDL_StopTextInput();

}

// sort the score table

void sort_it(ScoreData *s){

	ScoreData temp;			/* temporary struct */
	int sorted = 0, i;
	
	while (!sorted){
		sorted = 1;
		for (i = 0; i < 9; ++i){
			if (s[i].points < s[i+1].points){
				sorted = 0;
				temp = s[i];
				s[i] = s[i+1];
				s[i+1] = temp;
			}
		}
	}
}
