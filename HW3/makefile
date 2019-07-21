# Default rule is to build the executable called life
all: life addem

# Rule for building the lifeexecutable.
life: life.o twoD.o
	gcc -g life.o twoD.o -lpthread -o life

# Rule for building the addem executable.
addem:
	gcc -g addem.c -lpthread -o addem

# Rules for building each object file.
life.o: life.c twoD.h
	gcc -g -c life.c -lpthread

twoD.o: twoD.c twoD.h
	gcc -g -c twoD.c

# Give command "make clean" to remove object files,
#  executable, and documentation.
clean:
	rm -f *.o life addem
	rm -f -r html latex

# Give command "make docs" to create documentation.
docs:
	doxygen
