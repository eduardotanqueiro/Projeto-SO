#include "edge_server.h"

// TESTE
int x;

int EdgeServer(int edge_server_number)
{

    sigset_t block_mask, original_mask;
    struct sigaction action;
    // signal(SIGINT,SIG_IGN);

    action.sa_handler = end_sig;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);

    // Ignore all signals on all threads
    // não é preciso se for feito no main?
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGINT);
    sigaddset(&block_mask, SIGTSTP);
    pthread_sigmask(SIG_BLOCK, &block_mask, &original_mask);

#ifdef DEBUG
    printf("ES_name: %s, CPU1_CAP: %d, CP2_CAP: %d\n", edge_server_list[edge_server_number].SERVER_NAME, edge_server_list[edge_server_number].CPU1_CAP, edge_server_list[edge_server_number].CPU2_CAP);
    x = edge_server_number;
#endif

    end_system = 0;
    pthread_mutex_init(&read_end, NULL);

    // Create vCPU's
    pthread_create(&cpu_threads[0], NULL, vCPU1, 0);
    pthread_create(&cpu_threads[1], NULL, vCPU2, 0);

    // Resume CTRL+C handling on main thread
    // signal(SIGINT,end_sig);
    // sigemptyset(&mask);
    // sigaddset(&mask,SIGINT);

    pthread_sigmask(SIG_SETMASK, &original_mask, NULL);

    printf("Unblocked\n");

// TODO
// RECEIVE TASKS AND MANAGE ACTIVE CPUS
// UNNAMED PIPES
#ifdef DEBUG
    pause();
#endif

    // end_sig();

    return 0;
}

void *vCPU1()
{

#ifdef DEBUG
    printf("CPU1 Working...\n");
#endif

    while (1)
    {

        pthread_mutex_lock(&read_end);
        if (end_system == 1)
            break;
        pthread_mutex_unlock(&read_end);

        // DO WORK
    }

    printf("fora loop thread1 edge server!\n");

    pthread_exit(NULL);
}

void *vCPU2()
{

#ifdef DEBUG
    printf("CPU2 Working...\n");
#endif

    while (1)
    {

        pthread_mutex_lock(&read_end);
        if (end_system == 1)
            break;
        pthread_mutex_unlock(&read_end);

        // DO WORK
    }

    printf("fora loop thread2 edge server!\n");

    pthread_exit(NULL);
}

void end_sig()
{

    // write_screen_log("Cleaning up one edge server");
    printf("Cleaning up edge server no %d\n", x);

    // TODO
    // SIGNAL CPU THREADS TO END WORK AND FINISH
    // pthread_cancel(cpu_threads[0]);
    // pthread_cancel(cpu_threads[1]);

    pthread_mutex_lock(&read_end);
    end_system = 1;
    pthread_mutex_unlock(&read_end);

    // wait for CPU threads to end work
    for (int i = 0; i < 2; i++)
        pthread_join(cpu_threads[i], NULL);

    pthread_mutex_destroy(&read_end);

    printf("Edge server no %d ended cleanup\n", x);
    // write_screen_log("An edge server just completed cleanup");
    exit(0);
}
