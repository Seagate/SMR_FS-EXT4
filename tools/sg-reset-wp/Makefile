#
# Builds sg-reset-wp
#

CC = g++
CC_FLAGS = -Wall

EXEC = sg-reset-wp
SOURCES = $(wildcard *.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

$(EXEC): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXEC)

%.o: %.cpp
	$(CC) -c $(CC_FLAGS) $< -o $@

clean:
	rm -f $(OBJECTS)

distclean: clean
	@rm -f $(EXEC)
