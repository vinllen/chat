CC = g++
SOURCES = *.cpp *.h ../base.*
OBJECTS = main.o Server.o base.o
TARGET = server
CFLAG = -g

${TARGET} : ${SOURCES}
	${CC} ${CFLAG} -o $@ $^

.PHONY : clean
clean :
	rm $(TARGET) $(OBJECTS) 
