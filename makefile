EXE = tmpl85.00a.exe
SRC = \
   game.cpp \
   surface.cpp \
   template.cpp \
   counters.cpp \
   threads.cpp
INC = \
   -Ilib/FreeImage/inc \
   -Ilib \
   -Ilib/OpenGL \
   -Ilib/SDL2-x64/include
OBJ = $(SRC:.cpp=.o)
DEP = $(OBJ:.o=.d)
LIBDIR = \
   -Llib/FreeImage/lib64 \
   -Llib/lib/SDL2-x64/lib
SYSLIB = \
   -lwinmm -limm32 -lole32 -loleaut32 \
   -lversion -luuid -lopengl32
LIBS = \
   -lmingw32 \
   lib/FreeImage/lib64/FreeImage.lib \
   lib/SDL2-x64/lib/libSDL2main.a \
   lib/SDL2-x64/lib/libSDL2.a \
   $(SYSLIB)

CC=g++
WARNING=-Wall -Wno-strict-aliasing -Wno-write-strings -Wno-unused-function
CFLAGS=$(WARNING) -m64 -Ofast -flto -march=native -funroll-loops -fno-builtin
LDFLAGS=-mwindows -m64 -lmingw32
RM=rm

%.o: %.cpp
	$(CC) $(CFLAGS) $(INC) -o $@ -c $<

.PHONY : all
.PHONY : clean

all: $(EXE)

$(EXE): $(OBJ)
	$(CC) $(LDFLAGS) $(LIBDIR) $(OBJ) -o $@ $(LIBS)

clean:
	-$(RM) $(OBJ) core