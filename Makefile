TEST_FLAGS = -g -std=c11 -Wall -Wextra -Wpedantic
vpath %.c ./src
vpath %.h ./src
vpath %.c ./test_src
vpath %.h ./test_src

fsm_objects = t-fsm.o test_fsm.o tests.o
fsm_logger_objects = t-fsm_logger.o test_fsm_logger.o tests.o

all:
	@echo "all invoked, logic not implemented yet"

# tests common
tests.o: %.o: %.c
	$(CC) -c $(TEST_FLAGS) $< -o $@
# fsm related tests
test_fsm: $(fsm_objects)
	$(CC) -o test $^
t-fsm.o: fsm.c fsm.h
	$(CC) -c $(TEST_FLAGS) $< -o $@
test_fsm.o: %.o: %.c
	$(CC) -c $(TEST_FLAGS) $< -o $@
# fsm_logger related tests
test_logger: $(fsm_logger_objects)
	$(CC) -o test $^
t-fsm_logger.o: fsm_logger.c fsm_logger.h
	$(CC) -c $(TEST_FLAGS) $< -o $@
test_fsm_logger.o: %.o: %.c
	$(CC) -c $(TEST_FLAGS) $< -o $@

.PHONY: clean
clean:
	-rm test *.o

