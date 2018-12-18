DEPENDENCIES_DFS=src/dfs/dfs.cpp src/dfs/driver.cpp
DEPENDENCIES_DFC=src/dfc/dfc.cpp src/dfc/driver.cpp
#  Compiler to use (gcc / g++)
COMPILER=g++ -std=c++11
LIBS=-pthread -lssl -lcrypto
CFLG=-O3 -Wall
#  MinGW
ifeq "$(OS)" "Windows_NT"
CLEAN=del dfs.exe dfc.exe *.o *.a
else
#  OSX/Linux/Unix/Solaris
CLEAN=rm -rf dfs dfc DFS1/* DFS2/* DFS3/* DFS4/* *.o *.a
endif

all : dfs dfc

dfs : $(DEPENDENCIES_DFS)
	$(COMPILER) $(CFLG) -o dfs $^ $(LIBS)

dfc : $(DEPENDENCIES_DFC)
	$(COMPILER) $(CFLG) -o dfc $^ $(LIBS)

#== Clean current source dirictory and source directory for CSCIx229 library
clean:
	$(CLEAN)

recycle: $(DEPENDENCIES_DFC) $(DEPENDENCIES_DFS)
	$(MAKE) clean
	$(MAKE)

#== Execute the compiled binary file
client: dfc
	./dfc
test1: dfs
	./dfs 10001 DFS1 600 # port 10001, folder DFS1, timeout of 10 minutes
test2: dfs
	./dfs 10002 DFS2 600 # port 10002, folder DFS2, timeout of 10 minutes
test3: dfs
	./dfs 10003 DFS3 600 # port 10003, folder DFS3, timeout of 10 minutes
test4: dfs
	./dfs 10004 DFS4 600 # port 10004, folder DFS4, timeout of 10 minutes
short: dfs
	./dfs 10001 DFS1 10 # port 10001, folder DFS1, timeout of 10 seconds
