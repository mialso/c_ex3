TEST_FLAGS = -g -std=c11 -Wall -Wextra -Wpedantic
vpath %.c ./src
vpath %.h ./src
vpath %.c ./test_src
vpath %.h ./test_src

fsm_objects = t-fsm.o test_fsm.o tests.o
fsm_logger_objects = t-fsm_logger.o test_fsm_logger.o tests.o
server_objs = fsm_server.o
server_socket_objs = t-fsm_server_socket.o test_server_socket.o tests.o

all:
	@echo "all invoked, logic not implemented yet"
server: $(server_objs)
	$(CC) -o FSMserver $^

# server
fsm_server.o: fsm_server.c
	$(CC) -c $(TEST_FLAGS) $< -o $@


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
# fsm_server_socket tests
test_socket: $(server_socket_objs)
	$(CC) -o test $^
t-fsm_server_socket.o: fsm_server_socket.c fsm_server_socket.h
	$(CC) -c $(TEST_FLAGS) $< -o $@
test_server_socket.o: %.o: %.c
	$(CC) -c $(TEST_FLAGS) $< -o $@

.PHONY: clean
clean:
	-rm test *.o

