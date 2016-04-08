TEST_FLAGS = -g -std=c11 -Wall -Wextra -Wpedantic -D_POSIX_C_SOURCE=200809L
vpath %.c ./src
vpath %.h ./src
vpath %.c ./test_src
vpath %.h ./test_src

fsm_objs = fsm.o test_fsm.o tests.o
fsm_logger_objs = fsm_logger.o test_fsm_logger.o tests.o
server_objs = server.o server_socket_list.o fsm.o fsm_logger.o
server_socket_objs = server_socket.o test_server_socket.o tests.o
server_socket_list_objs = server_socket_list.o test_server_socket_list.o tests.o

all:
	@echo "all invoked, no logic implemented yet"
server: $(server_objs)
	$(CC) -o FSMserver $^

# server
server.o: server.c server_error.h
	$(CC) -c $(TEST_FLAGS) $< -o $@
server_socket.o: server_socket.c server_socket.h
	$(CC) -c $(TEST_FLAGS) $< -o $@
server_socket_list.o: server_socket_list.c server_socket_list.h
	$(CC) -c $(TEST_FLAGS) $< -o $@
fsm.o: fsm.c fsm.h fsm_error.h
	$(CC) -c $(TEST_FLAGS) $< -o $@
fsm_logger.o: fsm_logger.c fsm_logger.h fsm_logger_error.h
	$(CC) -c $(TEST_FLAGS) $< -o $@

# tests common
tests.o: %.o: %.c
	$(CC) -c $(TEST_FLAGS) $< -o $@
# fsm related tests
test_fsm: $(fsm_objs)
	$(CC) -o bin/tests/test_fsm $^
	./bin/tests/test_fsm
	make clean
test_fsm.o: %.o: %.c
	$(CC) -c $(TEST_FLAGS) $< -o $@
# fsm_logger related tests
test_fsm_logger: $(fsm_logger_objs)
	$(CC) -o test $^
test_fsm_logger.o: %.o: %.c
	$(CC) -c $(TEST_FLAGS) $< -o $@
# server_socket tests
test_server_socket: $(server_socket_objs)
	$(CC) -o ./bin/tests/test_server_socket $^
	@clear
	./bin/tests/test_server_socket
	valgrind ./bin/tests/test_server_socket | tail
	make clean
test_server_socket.o: %.o: %.c
	$(CC) -c $(TEST_FLAGS) $< -o $@
# server_socket_list tests
test_server_socket_list: $(server_socket_list_objs)
	$(CC) -o test $^
test_server_socket_list.o: %.o: %.c
	$(CC) -c $(TEST_FLAGS) $< -o $@

.PHONY: clean
clean:
	-rm *.o
	-rm bin/tests/*

