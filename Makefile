TEST_FLAGS = -g -std=c11 -Wall -Wextra -Wpedantic -D_POSIX_C_SOURCE=200809L
LIBS = -lrt
vpath %.c ./src
vpath %.h ./src
vpath %.c ./test_src
vpath %.h ./test_src

# main targets
server_objs = server.o server_socket.o server_signal.o server_signal_timer.o server_socket_list.o fsm.o fsm_logger.o
client_objs = client.o client_socket.o

# test targets
fsm_objs = fsm.o test_fsm.o tests.o
fsm_logger_objs = fsm_logger.o test_fsm_logger.o tests.o
server_socket_objs = server_socket.o test_server_socket.o tests.o
server_socket_list_objs = server_socket_list.o test_server_socket_list.o tests.o
client_socket_objs = client_socket.o server_socket.o test_client_socket.o tests.o

all:
	@echo "all invoked, no logic implemented yet, try 'server' or 'client'"

server: $(server_objs)
	@mkdir -p ./bin/
	$(CC) -o ./bin/FSMserver $^ $(LIBS)
client: $(client_objs)
	@mkdir -p ./bin/
	$(CC) -o ./bin/FSMclient $^

# server
server.o: server.c server_error.h
	$(CC) -c $(TEST_FLAGS) $< -o $@
server_socket.o: server_socket.c server_socket.h
	$(CC) -c $(TEST_FLAGS) $< -o $@
server_signal.o: server_signal.c server_signal.h server_signal_error.h
	$(CC) -c $(TEST_FLAGS) $< -o $@
server_signal_timer.o: server_signal_timer.c server_signal_timer.h
	$(CC) -c $(TEST_FLAGS) $< -o $@
server_socket_list.o: server_socket_list.c server_socket_list.h
	$(CC) -c $(TEST_FLAGS) $< -o $@
fsm.o: fsm.c fsm.h fsm_error.h
	$(CC) -c $(TEST_FLAGS) $< -o $@
fsm_logger.o: fsm_logger.c fsm_logger.h fsm_logger_error.h
	$(CC) -c $(TEST_FLAGS) $< -o $@

# client
client.o: client.c
	$(CC) -c $(TEST_FLAGS) $< -o $@
client_socket.o: client_socket.c client_socket.h
	$(CC) -c $(TEST_FLAGS) $< -o $@


# tests common
tests.o: %.o: %.c
	$(CC) -c $(TEST_FLAGS) $< -o $@

# fsm related tests
test_fsm: $(fsm_objs)
	@mkdir -p ./bin/tests/
	$(CC) -o ./bin/tests/test_fsm $^
	./bin/tests/test_fsm
	make clean_tests
test_fsm.o: %.o: %.c
	$(CC) -c $(TEST_FLAGS) $< -o $@

# fsm_logger related tests
test_fsm_logger: $(fsm_logger_objs)
	@mkdir -p ./bin/tests/
	$(CC) -o ./bin/tests/test_fsm_logger $^
	@echo "... test with some delay ... about 5 seconds, please be patient ..."
	./bin/tests/test_fsm_logger
	cat ./bin/fsm.log
	rm ./bin/fsm.log
	make clean_tests
test_fsm_logger.o: %.o: %.c
	$(CC) -c $(TEST_FLAGS) $< -o $@

# server_socket tests
test_server_socket: $(server_socket_objs)
	@mkdir -p ./bin/tests/
	$(CC) -o ./bin/tests/test_server_socket $^
	./bin/tests/test_server_socket
	make clean_tests
test_server_socket.o: %.o: %.c
	$(CC) -c $(TEST_FLAGS) $< -o $@

# client_socket tests
test_client_socket: $(client_socket_objs)
	@mkdir -p ./bin/tests/
	$(CC) -o ./bin/tests/test_client_socket $^
	./bin/tests/test_client_socket
	make clean_tests
test_client_socket.o: %.o: %.c
	$(CC) -c $(TEST_FLAGS) $< -o $@
	
# server_socket_list tests
test_server_socket_list: $(server_socket_list_objs)
	@mkdir -p ./bin/tests/
	$(CC) -o ./bin/tests/test_server_socket_list $^
	./bin/tests/test_server_socket_list
	make clean_tests
test_server_socket_list.o: %.o: %.c
	$(CC) -c $(TEST_FLAGS) $< -o $@

.PHONY: clean_tests
clean_tests:
	#-rm *.o
	find . -name "*.o" -exec rm {} \;
	-rm -rf ./bin/tests
.PHONY: clean
clean:
	find . -name "*.o" -exec rm {} \;
	-rm -rf ./bin

