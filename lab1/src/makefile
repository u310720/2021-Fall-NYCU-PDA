C = gcc
G = g++

exeName = Lab1
Head_and_Cpp = main.cpp sub.h declare.h

$(exeName): $(Head_and_Cpp)
	$(G) -std=c++11 $(Head_and_Cpp) -o $(exeName)

clean:
	rm output.txt *.gch *.o $(exeName)