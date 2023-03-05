#include<pthread.h>
#include "so_scheduler.h"
#include<semaphore.h>
#include<stdio.h>
#include<stdlib.h>

#define RUN 2
#define WAIT 3
#define FINISH 4

typedef struct {
    tid_t tid;
    unsigned int priority;
    int time;
    so_handler *handler_func;
    int ready;
    int max_case;
    sem_t semaphores;
} thread;

typedef struct {
    unsigned int events_no;
    unsigned int quantum;
    int init;
    int threads_no;
    thread **q;
    thread **threads;
    thread *actual_threads;
    pthread_mutex_t mutex;
    int size;
    sem_t sem_finish;
} scheduler;

static scheduler actual_scheduler;

/* se adauga un element in coada de prioritati*/
static void push(thread *th)
{
    int i = 0;
    int j;

    for (i = 0; i < actual_scheduler.size; i++) {
        if ((*actual_scheduler.q[i]).priority < th->priority)
            break;
    }
   if (i != actual_scheduler.size)
        for (j = actual_scheduler.size; j > i; j--)
            actual_scheduler.q[j] = actual_scheduler.q[j-1];
    actual_scheduler.q[i] = th;
    (*actual_scheduler.q[i]).ready = 1;
    actual_scheduler.size++;
}

/*se sterge un element din coada de prioritati*/
static thread *pop()
{
    int i = 0;

    while (i <= actual_scheduler.size - 2) {
        actual_scheduler.q[i] = actual_scheduler.q[i+1];
        i++;
    }

    thread *copy =  actual_scheduler.q[actual_scheduler.size - 1];
    actual_scheduler.q[actual_scheduler.size - 1] = NULL;
    return copy;
}

int so_init(unsigned int time_quantum, unsigned int io)
{
    if (actual_scheduler.init)
        return -1;
    else {
        if (time_quantum <= 0 || io > SO_MAX_NUM_EVENTS)
            return -1;
    }

    actual_scheduler.init = 1;
    actual_scheduler.quantum = time_quantum;
    actual_scheduler.events_no = io;
    actual_scheduler.threads_no = 0;
    actual_scheduler.actual_threads = NULL;
    actual_scheduler.threads = malloc(sizeof(thread *)*1030);
    int i;

    for (i = 0; i < 1030; i++)
        actual_scheduler.threads[i] = malloc(sizeof(thread));

    actual_scheduler.q = malloc(sizeof(thread *)*1030);

    actual_scheduler.size = 0;
    pthread_mutex_init(&actual_scheduler.mutex, NULL);
    sem_init(&actual_scheduler.sem_finish, 0, 1);
    return 0;
}

void so_end()
{
    int i;

    if (actual_scheduler.init == 0)
        return;
    sem_wait(&actual_scheduler.sem_finish);
    for (i = 0; i < actual_scheduler.threads_no; i++) {
        pthread_join(actual_scheduler.threads[i]->tid, NULL);
        sem_destroy(&actual_scheduler.threads[i]->semaphores);
        free(actual_scheduler.threads[i]);
    }

    for (i = actual_scheduler.threads_no; i < 1030; i++)
        free(actual_scheduler.threads[i]);

    free(actual_scheduler.threads);
    free(actual_scheduler.q);
    pthread_mutex_destroy(&actual_scheduler.mutex);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
    actual_scheduler.init = 0;
    sem_destroy(&actual_scheduler.sem_finish);
}

/* porneste threadul primul din coada */
static void execute_current_thread()
{
    actual_scheduler.actual_threads = actual_scheduler.q[0];
    actual_scheduler.actual_threads->time = actual_scheduler.quantum;
    actual_scheduler.actual_threads->ready = RUN;
    /* eliminam din coada threadul pe care l-am pornit*/
    thread *copy = pop();

    actual_scheduler.size--;
    /* se pun in executie threadul tocmai pornit */
    sem_post(&actual_scheduler.actual_threads->semaphores);
}

/* se planifica threadul care urmeaza */
void do_scheduler(void)
{
    thread *this_thread =  actual_scheduler.actual_threads;
    /*coada este goala */
    if (actual_scheduler.size == 0) {
        /* i-a expirat cuanta si nu exista un thread mai bun, se actualizeaza tot el*/
        if (this_thread->time <= 0)
           this_thread->time = actual_scheduler.quantum;
        if (this_thread->ready == FINISH)
            sem_post(&actual_scheduler.sem_finish);
        int error = sem_post(&this_thread->semaphores);
        if (error != 0)
            exit(EXIT_FAILURE);
        return;
   }
    thread *next_th = actual_scheduler.q[0];

    /*Daca coada nu e goala si nu ruleaza vreun thread, se cheama primul thread din q */
    if (!this_thread || this_thread->ready == FINISH || this_thread->ready == WAIT) {
        execute_current_thread();
        return;
    }

    /*Daca threadul din coada are prioritatea mai mare, el devine threadul curent*/
    if (next_th->priority > this_thread->priority) {
        push(this_thread);
        execute_current_thread();
        return;
    }
    /*A expirat cuanta si exista un thread in coada cu aceeasi proprietate*/
    if (this_thread->time <= 0) {
        if (this_thread->priority == next_th->priority) {
            push(this_thread);
            execute_current_thread();
            return;
        }
        (*this_thread).time = actual_scheduler.quantum;
    }
    sem_post(&this_thread->semaphores);
}

void so_exec()
{
    thread *this_thread =  actual_scheduler.actual_threads;
    /*scadem cuanta de timp*/
    (*this_thread).time--;
    do_scheduler();
    sem_wait(&(*this_thread).semaphores);
}

static void *begin_thread(void *arguments)
{
    thread *this_thread = (thread *) arguments;
    /* asteapta sa se planifice threadul */
    int error = sem_wait(&this_thread->semaphores);

    if (error != 0)
        exit(EXIT_FAILURE);

    this_thread->handler_func(this_thread->priority);
    this_thread->ready = FINISH;
    do_scheduler();
    return 0;
}
/*Creeaza nou thread */
static thread *new_thread(so_handler *func, unsigned int priority)
{
    int nr = actual_scheduler.threads_no;

    actual_scheduler.threads[nr]->tid = nr; 
    actual_scheduler.threads[nr]->priority = priority;
    actual_scheduler.threads[nr]->time = actual_scheduler.quantum;
    actual_scheduler.threads[nr]->handler_func = func;
    actual_scheduler.threads[nr]->max_case = SO_MAX_NUM_EVENTS;
    actual_scheduler.threads[nr]->ready  = 0;

    int error = sem_init(&actual_scheduler.threads[nr]->semaphores, 0, 0);

    if (error != 0)
        exit(EXIT_FAILURE);
    /*porneste thread-ul */
    error = pthread_create(&actual_scheduler.threads[nr]->tid, NULL,
    &begin_thread, (void *)actual_scheduler.threads[nr]);

    if (error != 0)
        exit(EXIT_FAILURE);

    return actual_scheduler.threads[nr];
}

tid_t so_fork(so_handler *func, unsigned int priority)
{
    if (func ==  NULL)
        return INVALID_TID;
    else {
        if (priority > SO_MAX_PRIO)
            return INVALID_TID;
    }
    /* punem semaforul sa astepte daca suntem la primul thread*/
    if (actual_scheduler.threads_no == 0)
        sem_wait(&actual_scheduler.sem_finish);
    /*adaugam datele threadului*/
    int nr = actual_scheduler.threads_no;
    thread *new_th = new_thread(func, priority);

    pthread_mutex_lock(&actual_scheduler.mutex);
    push(actual_scheduler.threads[nr]);
    actual_scheduler.threads_no++;
    pthread_mutex_unlock(&actual_scheduler.mutex);
    if (!actual_scheduler.actual_threads)
       do_scheduler();
    else
        so_exec();
    return actual_scheduler.threads[nr]->tid;
}

int so_wait(unsigned int io)
{
    if (io < 0)
        return -1;
    else {
        if (io >=  actual_scheduler.events_no)
            return -1;
    }
    /*punem sa astepte threadul curent*/
    actual_scheduler.actual_threads->ready = WAIT;
    actual_scheduler.actual_threads->max_case = io;
    so_exec();

    return 0;
}

int so_signal(unsigned int io)
{
    if (io < 0)
        return -1;
    else {
        if (io >=  actual_scheduler.events_no)
            return -1;
    }
    int i, deblocked_no = 0;
    /*ducem in starea de ready threadurile */
    for (i = 0; i < actual_scheduler.threads_no; i++) {
        if (actual_scheduler.threads[i]->ready == WAIT && actual_scheduler.threads[i]->max_case == io) {
            actual_scheduler.threads[i]->ready = 1;
            actual_scheduler.threads[i]->max_case = SO_MAX_NUM_EVENTS;
            push(actual_scheduler.threads[i]);
            deblocked_no++;
        }
    }
    so_exec();
    return deblocked_no;
}
