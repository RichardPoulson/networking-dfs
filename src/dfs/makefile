NAME=dfs
DEPENDENCIES=dfs.cpp driver.cpp
#  Compiler to use (gcc / g++)
COMPILER=g++ -std=c++11
LIBS=-pthread -lssl -lcrypto
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
test1:
	./$(NAME) 10001 DFS1 600 # port 10001, folder DFS1, timeout of 10 minutes
test2:
	./$(NAME) 10002 DFS2 600 # port 10001, folder DFS1, timeout of 10 minutes
test3:
	./$(NAME) 10003 DFS3 600 # port 10001, folder DFS1, timeout of 10 minutes
test4:
	./$(NAME) 10004 DFS4 600 # port 10001, folder DFS1, timeout of 10 minutes

#== Recycle: remove made files, compile, then run test recipe
recycle : $(NAME).cpp $(DEPENDENCIES)
	$(MAKE) clean
	$(MAKE)
	$(MAKE) test1 # Run the compiled binary file
