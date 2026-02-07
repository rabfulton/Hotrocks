CFLAGS = -std=c99 -Wall -g `sdl2-config --cflags` 
LFLAGS = `sdl2-config --libs` -L/usr/local/include/ -lGLEW -lSDL2_image -lm -lSDL2_mixer -lSDL2_ttf -lopenmpt -lGL -lGLU 
OBJS   = main.o gfx.o menu.o enemy.o scores.o
PROG = HotRocks
CXX = clang

# top-level rule to create the program.
all: $(OBJS) $(PROG)

# compiling other source files.
%.o: %./c %./h
	$(CXX) $(CFLAGS) -c -s $<
# linking the program.
$(PROG): $(OBJS)
	$(CXX) $(OBJS) -o $(PROG) $(LFLAGS)
# cleaning everything that can be automatically recreated with "make".
clean:
	rm -f $(PROG) *.o
