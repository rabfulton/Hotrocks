CFLAGS = -std=c99 -Wall -g `sdl2-config --cflags` 
LFLAGS = `sdl2-config --libs` -L/usr/local/include/ -lGLEW -lSDL2_image -lm -lSDL2_mixer -lSDL2_ttf -lopenmpt -lGL -lGLU 
OBJS   = src/main.o src/gfx.o src/menu.o src/enemy.o src/scores.o
PROG = HotRocks
CXX = clang

# top-level rule to create the program.
all: $(OBJS) $(PROG)

# compiling other source files.
src/%.o: src/%.c src/main.h
	$(CXX) $(CFLAGS) -c $< -o $@
# linking the program.
$(PROG): $(OBJS)
	$(CXX) $(OBJS) -o $(PROG) $(LFLAGS)
# cleaning everything that can be automatically recreated with "make".
clean:
	rm -f $(PROG) src/*.o
