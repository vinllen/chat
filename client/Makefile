CC = g++
SOURCES = *.cpp *.h ../base.*
TARGET = client 
CFLAG = -g

${TARGET} : ${SOURCES}
	${CC} ${CFLAG} -o $@ $^

.PHONY : clean
clean :
	rm $(TARGET)
