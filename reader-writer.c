#include<stdio.h>
#include<pthread.h>
#include<semaphore.h>
#include<unistd.h>
#include<stdlib.h>

pthread_mutex_t wr, mutex;
int data = 10;
int readcount = 0;


void *reader(void *arg){
    long int num;
    num = (long int) arg;
    printf("\n Reader %ld requires to access the shared resource", num);
    // Khi một luồng hoàn thành thao tác đọc và ghi trên biến count, nó sẽ gọi pthread_mutex_unlock(&mutex) để giải phóng mutex và cho phép các luồng khác có thể tiếp tục truy cập vào biến count.
    pthread_mutex_lock(&mutex);
    readcount++;

    if(readcount == 1){
        pthread_mutex_lock(&wr);
        printf("\nTIME FOR READING!!!");
    }

    pthread_mutex_unlock(&mutex);


    printf("\n Reader %ld is in critical section!", num);
    printf("\n Reader %ld is reading data as %d", num, data);
    
    // Critical section
    srand(time(NULL));
    int timesleep = rand()%10;
    sleep(timesleep);

    printf("\n Reader %ld leaves critical section!", num);

    pthread_mutex_lock(&mutex);
    // printf("\n Count %d", readcount);
    readcount--;
    pthread_mutex_unlock(&mutex);
    // Nếu không có pthread_mutex_lock and unlock thì sẽ dẫn đến hiện tượng race condition và bất đồng bộ trong việc truy cập biến readcount

    if(readcount == 0){
        pthread_mutex_unlock(&wr);
        printf("\nWRITER CAN EDIT FILE");
    }

}

void *writer(void *arg){
    long int num;
    num = (long int) arg;


    printf("\n Writer %ld requires access to critical section!", num);
    //lock wr variable
    pthread_mutex_lock(&wr);

    // Critical section
    printf("\n Writer %ld is in critical section!", num); 
    printf("\n Writer %ld has written data as %d", num, ++data); 
    sleep(1);

    //writer releases a lock on Writer
    pthread_mutex_unlock(&wr);
    printf("\n Writer %ld leaves critical section!", num); 

}

int main(){
    pthread_t r[10], w[10];
    long int i, j;
    int number_of_reader, number_of_writer;

    //initialize mutex variables
    pthread_mutex_init(&wr, NULL);
    pthread_mutex_init(&mutex, NULL);

    //Create reader and writer threads
    printf("Enter number of readers and writers: \n");
    scanf("%d %d", &number_of_reader, &number_of_writer);

    //Create reader and writer threads

    for (j=0; j<number_of_writer; j++){
        pthread_create(&w[j], NULL, writer, (void *)j);
    }

    for(i=0; i<number_of_reader; i++){
        pthread_create(&r[i], NULL, reader, (void *)i);
    }

    //Join threads
    for(i=0; i<number_of_reader; i++){
        pthread_join(r[i], NULL);
    }

    for (j=0; j<number_of_writer; j++){
        pthread_join(w[j], NULL);
    }

    return 0;
}