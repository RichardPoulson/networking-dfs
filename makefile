NAME=dfs
DEPENDENCIES=dfs.cpp driver.cpp
#  Compiler to use (gcc / g++)
COMPILER=g++
LIBS=-pthread
CFLG=-O3 -Wall
#  MinGW
ifeq "$(OS)" "Windows_NT"
CLEAN=del dfs.exe dfc.exe *.o *.a
else
#  OSX/Linux/Unix/Solaris
CLEAN=rm -f dfs dfc *.o *.a
endif

all :
	cd src/dfs && $(MAKE)
	cd src/dfc && $(MAKE)

#== Clean current source dirictory and source directory for CSCIx229 library
clean:
	cd src/dfs && $(CLEAN)
	cd src/dfc && $(CLEAN)
