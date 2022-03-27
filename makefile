FLAGS = -Wall -g -pthread
CC = gcc
PROG = offloading
OBJS = main.o sys_manager.o edge_server.o maintenance_manager.o monitor.o task_manager.o

all: ${PROG}

clean:
		rm ${OBJS} ${PROG} *~

${PROG}:	${OBJS}
		${CC} ${FLAGS} ${OBJS} -o $@

.c.o:
		${CC} ${FLAGS} $< -c

##################

main.o:	main.c sys_manager.h

sys_manager.o: sys_manager.c sys_manager.h

edge_server.o: edge_server.c edge_server.h

maintenance_manager.o: maintenance_manager.c maintenance_manager.h

monitor.o: monitor.c monitor.h

task_manager.o: task_manager.c task_manager.h


offloading: main.o sys_manager.o edge_server.o maintenance_manager.o monitor.o task_manager.o

