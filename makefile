NAME=dfs
DEPENDENCIES=
#  Compiler to use (gcc / g++)
COMPILER=g++
LIBS=-pthread
CFLG=-O3 -Wall
#  MinGW
ifeq "$(OS)" "Windows_NT"
CLEAN=del $(NAME).exe *.o *.a
else
#  OSX/Linux/Unix/Solaris
CLEAN=rm -f $(NAME) *.o *.a
endif

all : $(NAME)

#== Compile and link (customize name at top of makefile)
$(NAME) : $(NAME).cpp $(DEPENDENCIES)
	$(COMPILER) $(CFLG) -o $(NAME) $^ $(LIBS)

#== Clean current source dirictory and source directory for CSCIx229 library
clean:
	$(CLEAN)

#== Execute the compiled binary file
test:
	./$(NAME) 10001 20 # port 10001, 20 second timeout

#== Recycle: remove made files, compile, then run test recipe
recycle : $(NAME).cpp $(DEPENDENCIES)
	$(MAKE) clean
	$(MAKE)
	$(MAKE) test # Run the compiled binary file