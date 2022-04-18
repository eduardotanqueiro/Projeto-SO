FLAGS = -Wall -g -pthread
CC = gcc


PROG1 = offloading
OBJS1 = main.o sys_manager.o edge_server.o maintenance_manager.o monitor.o task_manager.o

PROG2 = mobile_node
OBJS2 = mobile_node.o

all: ${PROG1} clean1
all: ${PROG2} clean2

clean1:
		rm -f ${OBJS1}
		
clean2:
		rm -f ${OBJS2}

${PROG1}:	${OBJS1}
		${CC} ${FLAGS} ${OBJS1} -o $@

${PROG2}:	${OBJS2}
		${CC} ${FLAGS} ${OBJS2} -o $@		

.c.o:
		${CC} ${FLAGS} $< -c

##################

main.o:	main.c sys_manager.h

sys_manager.o: sys_manager.c sys_manager.h

edge_server.o: edge_server.c edge_server.h

maintenance_manager.o: maintenance_manager.c maintenance_manager.h

monitor.o: monitor.c monitor.h

task_manager.o: task_manager.c task_manager.h

mobile_node.o: mobile_node.c std.h


mobile_node: mobile_node.o

offloading: main.o sys_manager.o edge_server.o maintenance_manager.o monitor.o task_manager.o

