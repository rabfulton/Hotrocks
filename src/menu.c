
#include "main.h"

extern SDL_Window* mywindow;				// the usual window
extern screen_t screen;
extern int MAX_PARTICLES;
extern int NO_OF_EXPLOSIONS;
extern int FSAA;
extern int FULLSCREEN;
extern int WINDOW_WIDTH;
extern int WINDOW_HEIGHT;
void set_keys(GLuint tex);
void save_keys();
void load_defaults();
void set_detail(GLuint tex);
void set_mouse(GLuint tex);
void set_audio(GLuint tex);
void set_resolution(GLuint tex);

int main_menu(GLuint tex){

	char *s1[] = {"Start Game", "Options", "High Scores", "Quit"};
	char *sc[] = {"Game Mode", "Controls", "Video", "Audio", "Back"};
	char video_mode_label[24] = "Fullscreen";
	char *sv[] = {video_mode_label, "Resolution", "Detail", "Back"};
	char *sm[] = {"1 Player", "2 Players", "Death Duel", "Back"};
	char *sp[] = {"Player Keys", "mouse", "Defaults", "Back"};
	char **s;
	static int mode = 0;
	s = s1;
	int colour = 0;
	int selected = 0;
	int quit = 0;
	int launch_game = 0;
	SDL_Event e;
	int menu_mode = 0;

	SDL_SetRelativeMouseMode(0);
	SDL_ShowCursor(SDL_ENABLE);

	enum{
		MENU_MAIN,
		MENU_CONFIG,
		MENU_GAME,
		MENU_VIDEO,
		MENU_CONTROLS,
		MENU_AUDIO,
	};
	
	while (!quit){
		int item_count = (menu_mode == MENU_CONFIG) ? 5 : 4;
		int y_top = (int)(screen.height * 0.32f);
		int y_bottom = (int)(screen.height * 0.82f);
		int y_step = 0;
		if (item_count > 1){
			y_step = (y_bottom - y_top) / (item_count - 1);
		}

		glClear (GL_COLOR_BUFFER_BIT);
		draw_background(tex);
		glColor4f(1,1,1,1);
		if (FULLSCREEN)
			snprintf(video_mode_label, sizeof(video_mode_label), "Windowed");
		else
			snprintf(video_mode_label, sizeof(video_mode_label), "Fullscreen");

		draw_menu_item(screen.width/2, screen.height/7, "HOT ROCKS", 2, 88);

		for (int i = 0; i < item_count; ++i){
			if (i == selected)
				colour = 1;
			else
				colour = 2;

			int y = y_top + y_step * i;
			draw_menu_item(screen.width/2, y, s[i], colour, 48);
		}

		while (SDL_PollEvent(&e) != 0){
			if (e.type == SDL_MOUSEMOTION){
				if (y_step > 0){
					int mx = 0;
					int my = 0;
					SDL_GetMouseState(&mx, &my);
					int idx = (my - y_top + y_step / 2) / y_step;
					if (idx < 0) idx = 0;
					if (idx >= item_count) idx = item_count - 1;
					selected = idx;
				}
			}
			if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT){
				e.type = SDL_KEYDOWN;
				e.key.keysym.sym = SDLK_RETURN;
			}
	
			// Main menu options here.
			//--------------------------
			if(e.type == SDL_KEYDOWN && menu_mode == MENU_MAIN){

				switch(e.key.keysym.sym){
					
				case SDLK_RETURN: case SDLK_SPACE:
					play_sound(FIRE, 0);
					if (selected == 0){
						launch_game = 1;
						quit = 1;
					}
					else if (selected == 1){		// goto config menu
						s = sc;
						menu_mode = MENU_CONFIG;
					}
					else if (selected == 2){		// goto highscores
						high_scores(0, 0, tex);
						menu_mode = MENU_MAIN;
						s = s1;
					}
					else if (selected == 3){
						SDL_Delay(700);				// Delay to let sample finish
						quit = 1;
						mode = 99;
					}						
					selected = 0;					// start at top of new menu
					break;
					
				case SDLK_DOWN:
					++selected;
					play_sound(SELECTS, 0);
					break;
					
				case SDLK_UP:
					--selected;
					play_sound(SELECTS, 0);
					break;
				}
			break;	
			}
			
			// Config Menu Here
			//------------------
			if(e.type == SDL_KEYDOWN && menu_mode == MENU_CONFIG){

				switch(e.key.keysym.sym){
					
				case SDLK_RETURN: case SDLK_SPACE:
					play_sound(FIRE, 0);
					if (selected == 0){
						s = sm;
						menu_mode = MENU_GAME;
					}
					else if (selected == 1){
						s = sp;
						menu_mode = MENU_CONTROLS;
					}
					else if (selected == 2){
						s = sv;
						menu_mode = MENU_VIDEO;
					}
					else if (selected == 3){
						set_audio(tex);
					}
					else if (selected == 4){
						menu_mode = MENU_MAIN;
						s = s1;
					}
					selected = 0;
					break;
					
				case SDLK_DOWN:
					++selected;
					play_sound(SELECTS, 0);
					break;
					
				case SDLK_UP:
					--selected;
					play_sound(SELECTS, 0);
					break;				
				}
			break;	
			}
			
			// Video Menu Here
			//-------------------
			if(e.type == SDL_KEYDOWN && menu_mode == MENU_VIDEO){

				switch(e.key.keysym.sym){
					
				case SDLK_RETURN: case SDLK_SPACE:
					play_sound(FIRE, 0);
					if (selected == 0){
						set_fullscreen(!FULLSCREEN);
						save_keys();
					}
					else if (selected == 1){						// select aspect ratio
						set_resolution(tex);
					}									
					else if (selected == 2){						// set detail defailts
						set_detail(tex);						
					}
					else if (selected == 3){
						menu_mode = MENU_CONFIG;
						s = sc;
					}
					selected = 0;
					break;

				case SDLK_DOWN:
					++selected;
					play_sound(SELECTS, 0);
					break;

				case SDLK_UP:
					--selected;
					play_sound(SELECTS, 0);
					break;
			
				}
			break;
			}
			
			// Game Mode Menu Here
			//--------------------------
			if(e.type == SDL_KEYDOWN && menu_mode == MENU_GAME){
				switch(e.key.keysym.sym){
					
				case SDLK_RETURN: case SDLK_SPACE:
					play_sound(FIRE, 0);
					if (selected == 0){
						mode = 0;					//set 1player game
						menu_mode = MENU_MAIN;
						s = s1;
					}
					else if (selected == 1){
						mode = 1;					//set two players
						menu_mode = MENU_MAIN;
						s = s1;
					}
					else if (selected == 2){
						mode = 2;					//set death duel mode
						menu_mode =MENU_MAIN;
						s = s1;
					}
					else if (selected == 3){
						menu_mode = MENU_CONFIG;
						s = sc;
					}
					selected = 0;
					break;
					
				case SDLK_DOWN:
					++selected;
					play_sound(SELECTS, 0);
					break;
					
				case SDLK_UP:
					--selected;
					play_sound(SELECTS, 0);
					break;		
				}
			break;	
			}
			
			// Keyboard Config Menu Here
			//-----------------------------
			if(e.type == SDL_KEYDOWN && menu_mode == MENU_CONTROLS){
				switch(e.key.keysym.sym){
					
				case SDLK_RETURN: case SDLK_SPACE:
					play_sound(FIRE, 0);
					if (selected == 0){
						set_keys(tex);				// Players controls
					//	menu_mode = MENU_MAIN;
					//	s = s1;
					}
					else if (selected == 1){
						set_mouse(tex);				// Mouse enable
					}
						
					else if (selected == 2){
						load_defaults();			// Load defaults
						menu_mode = MENU_MAIN;
						s = s1;
					}
					else if (selected == 3){
						menu_mode = MENU_CONFIG;
						s = sc;
					}
					selected = 0;
					break;
					
				case SDLK_DOWN:
					++selected;
					play_sound(SELECTS, 0);
					break;
					
				case SDLK_UP:
					--selected;
					play_sound(SELECTS, 0);
					break;
				}
			break;		
			}						

			else if (e.type == SDL_QUIT)
				quit = 1;
		}
		
		if (selected < 0)
			selected = item_count - 1;
		if (selected >= item_count)
			selected = 0;
		SDL_GL_SwapWindow( mywindow );
		SDL_Delay(50);
	}

	SDL_ShowCursor(SDL_DISABLE);
	SDL_SetRelativeMouseMode(1);
	
	if (quit == 1)
		if (!launch_game)
			close_sdl();
	if (launch_game)
		return mode;
	return (mode);
}

void draw_menu_item(int x, int y, char text[], int colour, int size){

	texture_t t;

	if (text[0] == '\0')		// check for zero length string
		return;
	
	render_text(&t, text, colour, size);
	//glTexParameterf(GL_TEXTURE_2D, GL_CLAMP_TO_EDGE);
	//glTexParameterf(GL_TEXTURE_2D, GL_CLAMP_TO_EDGE);
	GLfloat vertex[8] = {	x-t.w/2, y-t.h/2,
							x+t.w/2, y-t.h/2,
							x+t.w/2, y+t.h/2,
							x-t.w/2, y+t.h/2 };
							

	GLfloat texver[8] = {0, 0, 1, 0, 1, 1, 0, 1};
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glMatrixMode(GL_MODELVIEW);	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, t.t);

	glVertexPointer(2, GL_FLOAT, 0, vertex);
	glTexCoordPointer(2, GL_FLOAT, 0, texver);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDeleteTextures(1, &t.t);
	glDisable(GL_TEXTURE_2D);
	
}

void set_keys(GLuint tex){

	int colour = 0;
	int selected = 0;
	int quit = 0;
	int readkey = 0;
	SDL_Event e;
	char * label[] = {
	    "P1 Rotate Left",
	    "P1 Rotate Right",
	    "P1 Thrust",
	    "P1 Fire",
	    "P1 Shield",
	    "P2 Rotate Left",
	    "P2 Rotate Right",
	    "P2 Thrust",
	    "P2 Fire",
	    "P2 Shield"
	};
	int row_count = 10;
	int y_top = (int)(screen.height * 0.30f);
	int y_bottom = (int)(screen.height * 0.88f);
	int y_step = (y_bottom - y_top) / (row_count - 1);
	if (y_step < 36)
		y_step = 36;
	int text_size = (screen.height > 1000) ? 25 : 22;
	
	while (!quit){
		
		glClear (GL_COLOR_BUFFER_BIT);
		draw_background(tex);
		glColor4f(1,1,1,1);
		draw_menu_item(screen.width/2, 150, "HOTKEYS", 2, 88);
			
		while(readkey && SDL_WaitEvent(&e)){
			if(e.type == SDL_KEYDOWN){
				play_sound(FIRE, 0);
				layout[selected] = e.key.keysym.sym;
				readkey = 0;
				break;
			}
		}
                

                
		while (SDL_PollEvent(&e) != 0){		// check for key event
	
			if(e.type == SDL_KEYDOWN){
				//play_sound(FIRE, 0);
				switch(e.key.keysym.sym){
				case SDLK_RETURN: case SDLK_SPACE:
					layout[selected] = 46;
					readkey = 1;
					break;
				case SDLK_DOWN:
					++selected;
					selected = selected % 10;
					play_sound(SELECTS, 0);
					break;
				case SDLK_UP:
					if (selected){
						--selected;
					}
					else{
						selected = 9;
					}
					play_sound(SELECTS, 0);
					break;
				case SDLK_ESCAPE:
					play_sound(FIRE, 0);
					save_keys();
					return;
				}
			break;			
			}	
		}

		for (int i = 0; i < 10; ++i){
			if (i == selected)
				colour = 1;
			else
				colour = 2;

			int y = y_top + i * y_step;
			draw_menu_item(screen.width/3, y, label[i], colour, text_size);
			draw_menu_item(screen.width - screen.width/4, y, (char*)SDL_GetKeyName(layout[i]), colour, text_size); 
		}
		
		SDL_GL_SwapWindow(mywindow);
		SDL_Delay(50);
		
	}

}

void load_keys(int a){

	int i, check = 0;
	FILE * config;							// pointer to config.dat

	if(a == 0)
		config = fopen("config.dat", "r");		// open for reading, use rb for non text
	else
		config = fopen("default.dat", "r");
	if (config == NULL){					// check file opens
		SDL_Log("Unable to open config.dat\n");
		return;
	}
	// try to read keyboard layout
	
	for(i = 0; i < 11; ++i){
		check = fscanf(config, "%*s %d", &layout[i]);	// read/ignore a string, eat whitespace, store int

		if (check != 1){
			SDL_Log("error reading keybinds from config.dat\n");			
			break;
		}
	}

	// try to read misc settings

	check = fscanf(config, "%*s %d", &MAX_PARTICLES);
	if (check != 1){
		SDL_Log("error reading max_particles from config.dat\n");
		MAX_PARTICLES = 2048;
	}
	check = fscanf(config, "%*s %d", &NO_OF_EXPLOSIONS);
	if (check != 1){
		SDL_Log("error reading NO_OF_EXPLOSIONS from config.dat\n");
		NO_OF_EXPLOSIONS = 16;
	}
	check = fscanf(config, "%*s %d", &FSAA);
	if (check != 1){
		SDL_Log("error reading FSAA from config.dat\n");
		FSAA = 0;
	}
	check = fscanf(config, "%*s %d", &SFX_VOLUME);
	if (check != 1){
		SFX_VOLUME = MIX_MAX_VOLUME;
	}
	check = fscanf(config, "%*s %d", &MUSIC_VOLUME);
	if (check != 1){
		MUSIC_VOLUME = MIX_MAX_VOLUME;
	}
	check = fscanf(config, "%*s %d", &FULLSCREEN);
	if (check != 1){
		FULLSCREEN = 1;
	}
	check = fscanf(config, "%*s %d", &WINDOW_WIDTH);
	if (check != 1){
		WINDOW_WIDTH = 1280;
	}
	check = fscanf(config, "%*s %d", &WINDOW_HEIGHT);
	if (check != 1){
		WINDOW_HEIGHT = 720;
	}
	
	fclose (config);
	set_sfx_volume(SFX_VOLUME);
	set_music_volume(MUSIC_VOLUME);
	save_keys();
}

void save_keys(){

	int i, check = 0;
	FILE * config;		/* pointer to highscore.dat */

	config = fopen("config.dat", "w");		/* open for writing, use wb for binary format */
	
	for(i = 0; i < 10; ++i){
		
		check = fprintf(config, "keybind= %d\n", layout[i]);

		if (check == 0){
			printf("error writing config.dat\n");			
			break;
		}
	}

	fprintf(config, "MOUSE= %d\n", layout[MOUSE_ENABLE]);
	fprintf(config, "MAX_PARTICLES= %d\n", MAX_PARTICLES);
	fprintf(config, "NO_OF_EXPLOSIONS= %d\n", NO_OF_EXPLOSIONS);
	fprintf(config, "FSAA= %d\n", FSAA);
	fprintf(config, "SFX_VOLUME= %d\n", SFX_VOLUME);
	fprintf(config, "MUSIC_VOLUME= %d\n", MUSIC_VOLUME);
	fprintf(config, "FULLSCREEN= %d\n", FULLSCREEN);
	fprintf(config, "WINDOW_WIDTH= %d\n", WINDOW_WIDTH);
	fprintf(config, "WINDOW_HEIGHT= %d\n", WINDOW_HEIGHT);
	fclose (config);		/* remember to close the file */
	
}

void load_defaults(){
	load_keys(1);
	save_keys();
}

void set_detail(GLuint tex){

	int colour = 0;
	int selected = 0;
	int quit = 0;
	SDL_Event e;
	char * label[] = {
	    "LOW",
	    "MEDIUM",
	    "HIGH", 
	    "Antialiasing ON", 
	    "Antialiasing OFF"
	};
	int row_count = 5;
	int y_top = (int)(screen.height * 0.36f);
	int y_bottom = (int)(screen.height * 0.88f);
	int y_step = (y_bottom - y_top) / (row_count - 1);
	if (y_step < 72)
		y_step = 72;
	int text_size = (screen.height > 1000) ? 48 : 40;
	
	while (!quit){
		
		glClear (GL_COLOR_BUFFER_BIT);
		draw_background(tex);
		glColor4f(1,1,1,1);
		draw_menu_item(screen.width/2, 150, "DETAILS", 2, 88);

                
		while (SDL_PollEvent(&e) != 0){		// check for key event
	
			if(e.type == SDL_KEYDOWN){
				
				switch(e.key.keysym.sym){
					
				case SDLK_RETURN: case SDLK_SPACE:
					play_sound(FIRE, 0);
					if (selected == 0){
						NO_OF_EXPLOSIONS = 8;		// Must be divisor of MAX_PARTICLES
						MAX_PARTICLES = 512;		// Must be multiple of NO_OF_EXPLOSIONS
						allocate_memory();
						quit = 1;
					}
					else if (selected == 1){
						NO_OF_EXPLOSIONS = 8;		// Must be divisor of MAX_PARTICLES
						MAX_PARTICLES = 1024;		// Must be multiple of NO_OF_EXPLOSIONS
						allocate_memory();
						quit = 1;
					}
					else if (selected == 2){
						NO_OF_EXPLOSIONS = 16;		// Must be divisor of MAX_PARTICLES
						MAX_PARTICLES = 2048;		// Must be multiple of NO_OF_EXPLOSIONS
						allocate_memory();
						quit = 1;
					}
					else if (selected == 3){
						toggle_anti_aliasing(1);
						quit = 1;
					}
					else if (selected == 4){
						toggle_anti_aliasing(0);
						quit = 1;
					}

					break;
					
				case SDLK_DOWN:
					++selected;
					play_sound(SELECTS, 0);
					break;
					
				case SDLK_UP:
					--selected;
					play_sound(SELECTS, 0);
					break;
					
				case SDLK_ESCAPE:
					play_sound(FIRE, 0);
					save_keys();
					return;
				}
			break;			
			}	
		}

		for (int i = 0; i < 5; ++i){
			
			if (i == selected)
				colour = 1;
			else
				colour = 2;

			int y = y_top + y_step * i;
			draw_menu_item(screen.width/2, y, label[i], colour, text_size); 
		}
		
		selected = (selected + 5) % 5;
		SDL_GL_SwapWindow(mywindow);
		SDL_Delay(50);		
	}
	save_keys();
}

void set_mouse(GLuint tex){

	int colour = 0;
	int selected = 0;
	int quit = 0;
	SDL_Event e;
	char* label[2] = {
	    "DISABLED",
	    "ENABLED",
	};
	int y_top = (int)(screen.height * 0.50f);
	int y_step = (int)(screen.height * 0.14f);
	if (y_step < 80)
		y_step = 80;
	int text_size = (screen.height > 1000) ? 48 : 40;
	
	while (!quit){
		
		glClear (GL_COLOR_BUFFER_BIT);
		draw_background(tex);
		glColor4f(1,1,1,1);
		draw_menu_item(screen.width/2, 150, "MR MOUSE", 2, 88);

                
		while (SDL_PollEvent(&e) != 0){		// check for key event
	
			if(e.type == SDL_KEYDOWN){
				
				switch(e.key.keysym.sym){
					
				case SDLK_RETURN: case SDLK_SPACE:
					play_sound(FIRE, 0);
					if (selected == 0){
						layout[MOUSE_ENABLE] = 0;
						quit = 1;
					}
					else if (selected == 1){
						layout[MOUSE_ENABLE] = 1;
						quit = 1;
					}
					
					break;
					
				case SDLK_DOWN:
					++selected;
					play_sound(SELECTS, 0);
					break;
					
				case SDLK_UP:
					--selected;
					play_sound(SELECTS, 0);
					break;
					
				case SDLK_ESCAPE:
					play_sound(FIRE, 0);
					save_keys();
					return;
				}
			break;			
			}	
		}

		for (int i = 0; i < 2; ++i){
			
			if (i == selected)
				colour = 1;
			else
				colour = 2;
			
			int y = y_top + y_step * i;
			draw_menu_item(screen.width/2, y, label[i], colour, text_size); 
		}
		
		selected = (selected+2)%2;
		SDL_GL_SwapWindow(mywindow);
		SDL_Delay(50);
		
	}

	save_keys();
}

void set_audio(GLuint tex){

	int colour = 0;
	int selected = 0;
	int quit = 0;
	SDL_Event e;
	char *label[] = {
		"SFX Volume",
		"Music Volume",
		"Back"
	};
	char value[3][16];
	int label_size = (screen.height > 1000) ? 44 : 36;
	int value_size = (screen.height > 1000) ? 40 : 32;
	int y_top = (int)(screen.height * 0.42f);
	int y_step = (int)(screen.height * 0.14f);
	if (y_step < 72)
		y_step = 72;

	while (!quit){

		glClear (GL_COLOR_BUFFER_BIT);
		draw_background(tex);
		glColor4f(1,1,1,1);
		draw_menu_item(screen.width/2, 150, "AUDIO", 2, 88);

		snprintf(value[0], sizeof(value[0]), "%d", (SFX_VOLUME * 100) / MIX_MAX_VOLUME);
		snprintf(value[1], sizeof(value[1]), "%d", (MUSIC_VOLUME * 100) / MIX_MAX_VOLUME);
		value[2][0] = '\0';

		while (SDL_PollEvent(&e) != 0){

			if(e.type == SDL_KEYDOWN){
				switch(e.key.keysym.sym){

				case SDLK_RETURN: case SDLK_SPACE:
					play_sound(FIRE, 0);
					if (selected == 2){
						quit = 1;
					}
					break;

				case SDLK_LEFT:
					if (selected == 0){
						set_sfx_volume(SFX_VOLUME - 8);
						play_sound(SELECTS, 0);
					}
					else if (selected == 1){
						set_music_volume(MUSIC_VOLUME - 8);
						play_sound(SELECTS, 0);
					}
					break;

				case SDLK_RIGHT:
					if (selected == 0){
						set_sfx_volume(SFX_VOLUME + 8);
						play_sound(SELECTS, 0);
					}
					else if (selected == 1){
						set_music_volume(MUSIC_VOLUME + 8);
						play_sound(SELECTS, 0);
					}
					break;

				case SDLK_DOWN:
					++selected;
					play_sound(SELECTS, 0);
					break;

				case SDLK_UP:
					--selected;
					play_sound(SELECTS, 0);
					break;

				case SDLK_ESCAPE:
					play_sound(FIRE, 0);
					save_keys();
					return;
				}
			}
			else if (e.type == SDL_QUIT){
				quit = 1;
			}
		}

		for (int i = 0; i < 3; ++i){
			if (i == selected)
				colour = 1;
			else
				colour = 2;

			int y = y_top + i * y_step;
			draw_menu_item(screen.width * 0.32f, y, label[i], colour, label_size);
			if (i < 2){
				draw_menu_item(screen.width * 0.78f, y, value[i], colour, value_size);
			}
		}

		selected = (selected + 3) % 3;
		SDL_GL_SwapWindow(mywindow);
		SDL_Delay(50);
	}

	save_keys();
}

void set_resolution(GLuint tex){

	int colour = 0;
	int selected = 0;
	int quit = 0;
	SDL_Event e;
	const int widths[] = {1024, 1280, 1366, 1600, 1920, 2560};
	const int heights[] = {576, 720, 768, 900, 1080, 1440};
	const int count = 6;
	char label[7][32];
	int row_count = count + 1;
	int y_top = (int)(screen.height * 0.34f);
	int y_bottom = (int)(screen.height * 0.86f);
	int y_step = (y_bottom - y_top) / (row_count - 1);
	if (y_step < 64)
		y_step = 64;
	int text_size = (screen.height > 1000) ? 42 : 34;

	for (int i = 0; i < count; ++i){
		snprintf(label[i], sizeof(label[i]), "%dx%d", widths[i], heights[i]);
		if (WINDOW_WIDTH == widths[i] && WINDOW_HEIGHT == heights[i]){
			selected = i;
		}
	}
	snprintf(label[count], sizeof(label[count]), "Back");

	while (!quit){

		glClear (GL_COLOR_BUFFER_BIT);
		draw_background(tex);
		glColor4f(1,1,1,1);
		draw_menu_item(screen.width/2, 150, "RESOLUTION", 2, 88);
		if (FULLSCREEN){
			draw_menu_item(screen.width/2, 230, "applies in windowed mode", 2, 24);
		}

		while (SDL_PollEvent(&e) != 0){

			if(e.type == SDL_KEYDOWN){
				switch(e.key.keysym.sym){
				case SDLK_RETURN: case SDLK_SPACE:
					play_sound(FIRE, 0);
					if (selected == count){
						quit = 1;
					}
					else{
						set_window_resolution(widths[selected], heights[selected]);
						save_keys();
					}
					break;
				case SDLK_DOWN:
					++selected;
					play_sound(SELECTS, 0);
					break;
				case SDLK_UP:
					--selected;
					play_sound(SELECTS, 0);
					break;
				case SDLK_ESCAPE:
					play_sound(FIRE, 0);
					save_keys();
					return;
				}
			}
			else if (e.type == SDL_QUIT){
				quit = 1;
			}
		}

		for (int i = 0; i < row_count; ++i){
			if (i == selected)
				colour = 1;
			else
				colour = 2;

			int y = y_top + y_step * i;
			draw_menu_item(screen.width/2, y, label[i], colour, text_size);
		}

		selected = (selected + row_count) % row_count;
		SDL_GL_SwapWindow(mywindow);
		SDL_Delay(50);
	}

	save_keys();
}
