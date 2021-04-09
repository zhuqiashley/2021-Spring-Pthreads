/*
  Author: Zhuqi You
  date: 03/27/2021
  CS446-Project 5
  POSIX Threads (pthreads) 
*/

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>

//First, node type for the queue
typedef struct Node{
      long m_data;
      struct Node * m_next;
}Node;

//Queue composed of type nodes and a single linked list
typedef struct TaskQueue{
      struct Node *m_head;
}TaskQueue;

//Function prototypes
TaskQueue* createQueue(void);
void deleteQueue(TaskQueue* queue);
bool enqueue(TaskQueue* queue, long data);
long dequeue(TaskQueue* queue);
void* task(void* params);

//Pthread locks and conditional variable
pthread_mutex_t QueueLock = PTHREAD_MUTEX_INITIALIZER;

//Locks for each shared variable
pthread_mutex_t SharedLockSum = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t SharedLockOdd = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t SharedLockMin = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t SharedLockMax = PTHREAD_MUTEX_INITIALIZER;

//We need the condition variable to wake up the threads
pthread_cond_t NewTask = PTHREAD_COND_INITIALIZER;

//The next step is to define the aggregate variables
volatile long sum = 0;
volatile long odd = 0;
volatile long min = INT_MAX;
volatile long max = INT_MIN;
volatile bool done = false;

//Main function
int main (int argc, char* argv[])
{
      //We need to check and analyze the command line options
      if (argc != 3)
      {
            printf("Usage: par_sumsq <infile> <number of threads(default=1)>\n");
            exit(EXIT_FAILURE);
      }

      //Now, we need to create TaskQueue
      TaskQueue* taskqueue = createQueue();

      //To start worker thread
      int numThreads = atoi(argv[2]);
      if (numThreads < 1)
      {

            printf("Negative or zero number of workers exiting.");
            exit(EXIT_FAILURE);
      }

      pthread_t workers[numThreads];
      for (int i = 0; i < numThreads; i++)
      {

            pthread_create(&workers[i], NULL, task, (void*)taskqueue);
      }

      //Open the file
      char *fn = argv[1];
      FILE* fin = fopen(fn, "r");

      if(!fin)
      {
            printf("Failed to open the file");
            exit(EXIT_FAILURE);
      }

      //Read file and populate Queue
      char action;
      long num;

      while (fscanf(fin, "%c %ld\n", &action, &num) == 2)
      {

            if (action == 'p')
            {
                  //Adding to queue need to lock
                  pthread_mutex_lock(&QueueLock);
                  //Add the new task
                  enqueue(taskqueue, num);
                  //Done with the queue can unlock
                  pthread_mutex_unlock(&QueueLock);
                  //New task has been added to awaken threads
                  pthread_cond_signal(&NewTask);
            }
            else if (action == 'w')
            {
                  sleep(num);
            }
            else
            {
                  printf("ERROR: Unrecognized action: '%c'\n", action);
                  exit(EXIT_FAILURE);
            }
      }

      //Files has been read. So, no new tasks in the queue
      fclose(fin);

      //We need to wait until the queue is empty
      while(taskqueue->m_head)
      {}

      //Signal to other threads that will be no more new tasks
      done = true;
      pthread_mutex_lock(&QueueLock);
      //Awaken threads in the queue with done flag
      pthread_cond_broadcast(&NewTask);
      //This causes the threads to exit while loop and return exit success
      pthread_mutex_unlock(&QueueLock);
      
      //To check for errors
      void* worker_return_val;
      for(int i = 0; i < numThreads; i++)
      {
            //Joins worker threads
            pthread_join(workers[i], &worker_return_val);

            //We need to check for errors joining threads
            if((long)worker_return_val != EXIT_SUCCESS)
            {
                  printf("Error joining threads exiting...\n");
                  exit(EXIT_FAILURE);
            }
      }
      deleteQueue(taskqueue);

      //To print the results
      printf("%ld %ld %ld %ld\n", sum, odd, min, max);

      return (EXIT_SUCCESS);
}

TaskQueue* createQueue(void)
{

      TaskQueue * queue = (TaskQueue*)malloc(sizeof(struct TaskQueue));
      queue->m_head = NULL;
      
      return queue;
}

void deleteQueue(TaskQueue* queue)
{
      
      //If the queue does not exist
      if(!queue)
      {
            return;
      }

      //Remove all elements
      while(queue->m_head)
      {
            dequeue(queue);
      }
      free(queue);
}

bool enqueue(TaskQueue* queue, long data)
{
      Node * newNode = (Node*)malloc(sizeof(struct Node));
      newNode->m_data = data;
      newNode->m_next = NULL;

      //If the first element make it the head
      if(!queue->m_head)
      {
            queue->m_head = newNode;
      }
      //Find the back
      else
      {
            Node* temp = queue->m_head;
            while(temp->m_next)
            {
                  temp = temp->m_next;
            }
            temp->m_next = newNode;
      }
      return true;
}

long dequeue(TaskQueue* queue)
{
      if(!queue)
      {
            return 0;
      }
      else
      {
            Node* temp = queue->m_head;
            queue->m_head = temp->m_next;
            long data = temp->m_data;
            free(temp);
            return data;
      }     
}

void* task(void* params)
{
      TaskQueue* taskqueue = (TaskQueue*)params;
      long number;

      while(!done)
      {
            //Acquire lock for queue
            pthread_mutex_lock(&QueueLock);

            /*There is no task ready, but the read file is not finished. 
            We need to wait until the task reads from the file*/
            while(!done && !taskqueue->m_head)
            {
                  pthread_cond_wait(&NewTask, &QueueLock); //NewTask will signal whne a new task is added         
            }
            //To make sure were not done after waiting
            if(done)
            {
                  /*
                  If we're done unlocking and leaving the function, 
                  because there's nothing left to do, 
                  we still need newtask to signal there
                  */
                 pthread_mutex_unlock(&QueueLock);
                 break;
            }
      
            /*
            When we calculate the global data, 
            we can only arrive here when we hold the lock 
            and signal that there is a task in the queue
            */
            number = dequeue(taskqueue);

            /*Finished changing queue
            So, we can unlock it and let other threads use it*/
            pthread_mutex_unlock(&QueueLock);

            //Edit variables
            long the_square =  number * number;

            //Simulate task
            sleep(number);

            /*
            Before editing a global variable, 
            you need to wait for the corresponding lock. 
            After editing a local variable, you need to return the lock
            */ 
            pthread_mutex_lock(&SharedLockSum);
            sum += the_square;
            pthread_mutex_unlock(&SharedLockSum);

            if (number % 2 == 1)
            {
                  //Now, we need to wait for this lock only when the thread id is odd
                  pthread_mutex_lock(&SharedLockOdd);
                  odd++;
                  pthread_mutex_unlock(&SharedLockOdd);
            }

            if (number < min)
            {
                  pthread_mutex_lock(&SharedLockMin);
                  min = number;
                  pthread_mutex_unlock(&SharedLockMin);
            }

            if (number > max)
            {
                  pthread_mutex_lock(&SharedLockMax);
                  max = number;
                  pthread_mutex_unlock(&SharedLockMax);
            }
      }
      return EXIT_SUCCESS;
 }



















































































