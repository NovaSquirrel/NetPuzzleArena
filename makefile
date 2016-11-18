objlist := puzzle utility playfield draw frenzy avalanche reversiball
program_title = puzzle
 
CC := gcc
LD := g++
 
objdir := obj
srcdir := src
objlisto := $(foreach o,$(objlist),$(objdir)/$(o).o)
 
# FL4SHK updated this makefile to work on Linux.  Date of update:  Jun 1, 2016
ifeq ($(OS),Windows_NT)
  CFLAGS := -Wall -O2 -std=gnu99 -ggdb
  LDLIBS := -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer
  LDFLAGS := -Wl,-subsystem,windows
else
  CFLAGS := -Wall -O2 -std=gnu99 `sdl2-config --cflags` -ggdb
  LDLIBS := `sdl2-config --libs` -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer
  #LDFLAGS := -Wl
endif
 
puzzle: $(objlisto)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)
 
$(objdir)/%.o: $(srcdir)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
 
.PHONY: clean
 
clean:
	-rm $(objdir)/*.o
