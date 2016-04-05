TEST_FLAGS = -g -std=c11 -Wall -Wextra -Wpedantic
vpath %.c ./src
vpath %.h ./src
vpath %.c ./test_src
vpath %.h ./test_src

objects = t-fsm.o test_fsm.o tests.o

all:
	@echo "all invoked, logic not implemented yet"

test: $(objects)
	$(CC) -o $@ $(objects)

t-fsm.o: fsm.c fsm.h
	$(CC) -c $(TEST_FLAGS) $< -o $@
test_fsm.o: %.o: %.c
	$(CC) -c $(TEST_FLAGS) $< -o $@
tests.o: %.o: %.c
	$(CC) -c $(TEST_FLAGS) $< -o $@

.PHONY: clean
clean:
	-rm test $(objects)

