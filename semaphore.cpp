#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <fstream>

const int MAX_SIZE = 100;

// USING SEMAPHORE //
// The POSIX threads (or pthread) libraries are a standards-based thread API for C/C++
struct process
{
    process *next;
    int ID;
    bool state = true;
    //  state represents whether the process is blocked or active
};

// The FIFO Structure: Queue //

class blockedQueue
{
    // int size = 0;
    process *front, *back;
    process nullprocess = {nullptr, -1, false};
    //  null process to be returned when the queue is empty

public:
    int size = 0;

    void push(int id)
    {
        //  I have made this function in such a way that it blocks the entering process as well

        if (size == MAX_SIZE)
        {
            return;
        }

        process *P = new process();
        P->ID = id;
        P->state = false;
        P->next = nullptr;
        //  the process is blocked before pushing into the blockedQueue
        //  in reality this is done using system calls

        ++size;

        if (size == 1)
        {
            front = P;
            back = P;
            return;
        }
        back->next = P;
        back = P;
    }

    process *getFront()
    {
        if (size == 0)
        {
            return &nullprocess;
        }

        return front;
    }

    process *pop()
    {
        if (size == 0)
        {
            return &nullprocess;
        }

        process *nextProcess = front;
        front = front->next;
        --size;

        nextProcess->next = NULL; // Set next pointer of the popped process to NULL
        nextProcess->state = true;

        return nextProcess;
    }
};

struct semaphore
{
    int value = 1;
    blockedQueue *blocked_queue = new blockedQueue();
    //  this queue would store the list of proessess waiting to acquire the semaphore

    semaphore(int n)
    {
        value = n;
        //  here n is the number of resources of that type available
    }
};

// INITIALIZATION //
int data = 1;
int resources = 1;
semaphore *rw_mutex = new semaphore(resources);
//  a writer with this semaphore has access to the critical section
//  also the first reader has to acquire this and the last reader has to let go of this
int read_count = 0;
semaphore *read_mutex = new semaphore(resources);
//  The above three data structures are the same as above in the classical solution
semaphore *entry_mutex = new semaphore(resources); // The new semaphore
//  This semaphore is used at the begining of both the reader and writer codes
//  A reader/writer first has to acquire this semaphore in order to enter the critical section
//  Both the reader and writer have equal priority for obtaining this semaphore

void wakeUp(process *p)
{
    p->state = true;
}

void signal(semaphore *sem)
{
    ++sem->value;

    if (sem->value <= 0)
    {
        // process *nextProcess = sem->blocked_queue->pop();
        process *nextProcess = sem->blocked_queue->getFront();

        wakeUp(nextProcess);
        // wakeUp the next process such that it's ready for entering critical section
    }
}

void wait(semaphore *sem, int id)
{

    --sem->value;

    if (sem->value < 0)
    {
        sem->blocked_queue->push(id);
        std::cout << "Process " << id << " is added to the block queue." << std::endl;

        while (true)
        {

            // check condition to get out of the block queue 
            if (sem->blocked_queue->getFront()->ID == id && sem->blocked_queue->getFront()->state == true)
            {
                sem->blocked_queue->pop();
                break;
            }
        }
    }
}

//  READER'S CODE
void classicalReader(int processId)
{
    // this function would be called everytime a new reader arrives

    wait(read_mutex, processId);
    //  the next section executes if the process with processId is not blocked

    ++read_count;

    if (read_count == 1)
    { // this implies that the first reader tries to access
        wait(rw_mutex, processId);
        std::cout << "TIME FOR READING!!!" << std::endl;
        //  this will wait until the process is activated
        //  due to freeing of the rw_mutex by a signal from writer
    }

    signal(read_mutex); // this call happens here, as unless a reader enters, others shouldn't modify the read_count


    // ***** CRITICAL SECTION ***** //
    std::cout << "Reader " << processId << " is in the critical section." << std::endl;
    std::cout << "Reader " << processId << " is reading data as " << data << std::endl;

    // Simulate some work being done in the critical section
    srand(time(NULL));
    int timesleep = rand() % 10;
    usleep(timesleep * 100000);

    std::cout << "Reader " << processId << " exited the critical section." << std::endl;
    //  this can be accessed by readers directly if read_count != 1 (they are not the first reader)
    //  also after the first reader has acquired the rw_mutex

    wait(read_mutex, processId);
    --read_count;
    signal(read_mutex);

    if (read_count == 0)
    {
        signal(rw_mutex);
        std::cout << "WRITER CAN EDIT FILE" << std::endl;
        // the last reader frees the rw_mutex
    }
}

//  WRITER'S CODE
void classicalWriter(int processId)
{
    // std::cout << "Writer " << processId << " requires to access the critical section" << std::endl;
    wait(rw_mutex, processId);
    //  the writer process would be blocked while waiting
    //  once free it will acquire the rw_mutex and enter the critical section


    // ***** CRITICAL SECTION ***** //
    std::cout << "Writer " << processId << " is in the critical section." << std::endl;
    std::cout << "Writer " << processId << " edits data to " << ++data << std::endl;

    // Simulate some work being done in the critical section
    srand(time(NULL));
    int timesleep = rand() % 10;
    usleep(timesleep * 100000);

    std::cout << "Writer " << processId << " exited the critical section." << std::endl;

    //  the critical section can be accessed by a writer only if they have acquired the rw_mutex
    signal(rw_mutex);
}

int main()
{
    int processID = 0;
    int nor, now;

    std::cout << "Enter the number of reader and writer: ";
    std::cin >> nor >> now;

    // std::ios_base::sync_with_stdio(false);
    // std::cin.tie(0);
    // freopen("output.out","w",stdout);

    pthread_t readerThread[MAX_SIZE], writerThread[MAX_SIZE];

    for (int j = 0; j < now; j++)
    {
        pthread_create(&writerThread[j], NULL, (void *(*)(void *))classicalWriter, reinterpret_cast<void *>(static_cast<intptr_t>(++processID)));
    }
    
    for (int i = 0; i < nor; i++)
    {
        pthread_create(&readerThread[i], NULL, (void *(*)(void *))classicalReader, reinterpret_cast<void *>(static_cast<intptr_t>(++processID)));
    }

    for (int i = 0; i < nor; i++)
    {
        pthread_join(readerThread[i], NULL);
    }

    for (int j = 0; j < now; j++)
    {
        pthread_join(writerThread[j], NULL);
    }

    return 0;
}