/*
    This is the main file for the OS course work.
    Date: December 2022
    Author: Alexander Arzt


    Approach: 
        The approach is very straightforward. We have two semaphores - one for the producers (producer_token)
        and one for the consumers (consumer_token). They are used together to enable synchronization between the producers
        and the consumers. The value of consumer_token represents how many filled spots there are in the circular queue (cq), whereas
        the producer_token represents how many free spots there are. Let's say a consumer adds a job to the cq, then he decreases
        the value of producer_token by 1 and increases the consumer_token by 1 - and vice versa if a consumer takes an element from the cq.
        
        Moreover, we need two mutex locks in order to restric access to the cq (because it is vital that only one thread manipulates it at a time)
        and the outstream (in order to prevent cluttered outstream). 
        
        Lastly, the cq was implemented using a custom class. However, that was done for elegance purposes and was not strictly necessary 
        - using a linear queue and bounding the index using the %-operator would have done the trick as well. 






*/

// Necessary libraries ----------------------------------------------------------------------------------------------------------------------------------
#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <chrono>
#include <stdio.h>
#include <unistd.h>
#include <utility>
#include <random>
#include <time.h>

//Custom Class - Circular Queue -------------------------------------------------------------------------------------------------------------------------
//Custom Class - Circular Queue -------------------------------------------------------------------------------------------------------------------------
using tup = std::pair<int,int>; 

class Circular_Queue{
   private:
        tup* items; 
        int front, end;
        int size;
        bool empty; 
   public:
        Circular_Queue(){
			empty = true; 
            front = 0;
            end = 0; 
        }

        void set_size(int _size){
          size = _size; 
          items = new tup[size];
        }
        

        // For consumers to add a job
        bool add_job(int duration) {
                 if (empty) empty = false;  
                 else end = (end + 1) % size;
                 items[end] = {end, duration}; //package the tuple and store in array
                 return true;                 
			}


        int get_end(){
          return end; 
        }
    
    // For producers to consume a job at the front of the cq
    tup consume_job() {
          tup item = items[front];
          if (front != end) front = (front + 1) % size; 
          else empty = true; 
          return item;
    }
    
    
    void print_q(){
		std::cout << "Start index: " << front << "     End index: " << end << std::endl; 
		
	if (front < end){
		for (int i = front; i <= end; ++i){
			std::cout << "Index:" << i << " - Value: {" << items[i].first <<"," << items[i].second << "}"<< std::endl; 
		}
	}
	
	if (front > end){
		for (int i = front; i < size; ++i) {std::cout << "Index: " << i << " - Value: {" << items[i].first <<"," << items[i].second << "}"<< std::endl; }
		for (int i = 0; i <= end; ++i) {std::cout << "Index: " << i << " - Value: {" << items[i].first <<"," << items[i].second << "}"<< std::endl; }
	}
	
	if (front == end && !empty) std::cout << "Index: " << front << " - Value: {" << items[front].first <<"," << items[front].second << "}"<< std::endl; 
	if (front == end && empty) std::cout << "CQ is empty" << std::endl; 
	
	}

};



//forward declarations for main() ----------------------------------------------------------------------------------------------------------------------------------------------------------
void *producer (void *id);
void *consumer (void *id);
int random_no(int to); 
void print_header(int size_of_q);

//global variables (accessed by all workers) -----------------------------------------------------------------------------------------------------------------------------------------------
sem_t producer_token;
sem_t consumer_token;
struct timespec tm; 
pthread_mutex_t q_access; 
pthread_mutex_t os_access; 

Circular_Queue cq = Circular_Queue(); // size is not known yet
int no_of_jobs;
int no_jobs_left;
int number_of_producers;
int number_of_consumers;



// Main Program ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//argc (argument counter) and argv (char array / argument vector) are how command line arguments are passed to the main function
int main(int argc, char* argv[]){ 
    //initialize the semaphores to prevent simultaneous access to shared data and enable synchronization 
    sem_init(&consumer_token, 0, 0);
    int size_of_q = atoi(argv[1]); 
    sem_init(&producer_token, 0, size_of_q);
    pthread_mutex_init(&q_access, NULL);
    pthread_mutex_init(&os_access, NULL);
    
    //setting seed
    srand((unsigned int)time(NULL));

    //sanity checks for the args
    if (argc != 5){ 
        std::cout << "Invalid number of arguments" << std::endl; 
        exit(1);
    }

    for (int i = 1; i < argc; i++){
        if (!isdigit(*argv[i]) || atoi(argv[i]) < 0) {
            std::cout << "Arguments passed are not valid - Put in positive integers only!" << std::endl; 
            exit(1);
        }
    }

    //set variables for the system ------------------------------------------------------------------------------
    cq.set_size(size_of_q);

    no_of_jobs = atoi(argv[2]); // number of jobs to be generated by the producers - each producer will generate the same amount of jobs

    number_of_producers = atoi(argv[3]);
    int* prod_ids = new int[number_of_producers]; // array holding producer ids
    
    number_of_consumers = atoi(argv[4]);
    int* cons_ids = new int[number_of_consumers]; // array holding consumer ids
    
    no_jobs_left = no_of_jobs*number_of_producers;

    pthread_t producers[number_of_producers]; // array of threads for producers
    pthread_t consumers[number_of_consumers]; // array of threads for consumers

    //print head for output
    print_header(size_of_q);

    //create the threads (i.e. producers and consumers) ---------------------------------------------------------
    for(int i = 0; i < number_of_producers; i++){
      prod_ids[i] = i+1;
      // initialize each thread with producer routine
      if(pthread_create(&producers[i], NULL, &producer, &prod_ids[i]) != 0){
        std::cout << "Cannot initialize producer thread no: " << i;
        exit(1);
      } 
    }

    for(int i = 0; i < number_of_consumers; i++){
      cons_ids[i] = i+1; 
      //initialize each thread with consumer routine
      if(pthread_create(&consumers[i], NULL, &consumer, &cons_ids[i])){
        std::cout << "Cannot initialize consumer thread no: " << i;
        exit(1);
      } 
    }


    //wait for threads to terminate
    for (int i = 0; i < number_of_producers; ++i){
      if(pthread_join(producers[i], NULL) != 0){
        std::cout << "Error: Cannot terminate producer thread: " << i << std::endl;
        exit(1);
      }
    }
     
    for (int i = 0; i < number_of_consumers; ++i){
        if(pthread_join(consumers[i], NULL) != 0){
        std::cout << "Error: Cannot terminate consumer thread: " << i << std::endl;
        exit(1);
      }
    }

    // free the resources //
    sem_destroy(&consumer_token);
    sem_destroy(&producer_token);
    pthread_mutex_destroy(&q_access);
    pthread_mutex_destroy(&os_access);

    std::cout << "Simulation finished successfully ..... " << std::endl; 
    
    return 0; 
}






/*  This function serves to create the producer threads.
    Parameters:
      prod_id (void*): The producer ID.
    Returns:
      Void.
*/
void* producer (void* prod_id){
    int no_jobs = no_of_jobs;
    
    
    while (no_jobs != 0){
        
        clock_gettime(CLOCK_REALTIME, &tm);
        tm.tv_sec += 20; 
        
        if (sem_timedwait(&producer_token, &tm) != 0) break; // exit if cannot get token after 20 sec
        pthread_mutex_lock(&q_access); pthread_mutex_lock(&os_access); // lock access to ostream and lock access to cq
        
        // ----------------- Begin Critical Section --------------- 
        int duration = random_no(10);
        if(cq.add_job(duration)){
            std::cout << "Producer" << "(" << *(int*)prod_id << "): Job id " << cq.get_end() << " duration " << duration << std::endl; 
            no_jobs--;
            if (no_jobs == 0) std::cout << "Producer" << "(" << *(int*)prod_id << "): I am done producing jobs...." << std::endl;
        }
        // ----------------- End Critical Section ---------------
        
        pthread_mutex_unlock(&q_access); pthread_mutex_unlock(&os_access); // free access
        sem_post(&consumer_token); // sync with consumers
        
        
        sleep(random_no(5));
    }
    return NULL; 
}


      
/*  This function serves to create the consumer threads.
    Parameters:
      cons_id (void*): The consumer ID.
    Returns:
      Void.
*/
void* consumer (void* cons_id){
    
    
    while (no_jobs_left != 0){

        clock_gettime(CLOCK_REALTIME, &tm); 
        tm.tv_sec += 20; 
        
        if (sem_timedwait(&consumer_token, &tm) != 0) break;
        pthread_mutex_lock(&q_access); pthread_mutex_lock(&os_access); // lock access  
        
        // ------------ Begin Critical Section -----------------
        tup elem = cq.consume_job();      
        int job_id = elem.first;
        int duration = elem.second;
        std::cout << "Consumer" << "(" << *(int*)cons_id << "): Job id " << job_id << " executing sleep duration " << duration << std::endl;
        no_jobs_left--;
        if (no_jobs_left == 0) std::cout << "No new jobs coming in... will finish what is being processed at the moment and then consumers are done... " << std::endl;  
        // ----------------- End Critical Section ---------------
        
        pthread_mutex_unlock(&q_access); pthread_mutex_unlock(&os_access);// free access to cq  
        sem_post(&producer_token); // sync
        
        
        sleep(duration); //process the job
        
        pthread_mutex_lock(&os_access);
        std::cout << "Consumer" << "(" << *(int*)cons_id << "): Job id " << job_id << " completed" << std::endl;
        pthread_mutex_unlock(&os_access);
        
    }
    return NULL;
}



/*  This function generates a pseudo-random number
    between 1 and an upper bound.
    Parameters:
      to (int): The upper bound.
    Returns: 
      Integer.
*/
int random_no(int to) {
    return (rand()%to) + 1;
}



/*  This function prints the header for the output
    Parameters:
      size_of_q (int): The size of the circular queue.
    Returns: 
      Void.
*/
void print_header(int size_of_q){
    std::cout << "\n----------------------------------------------------------------------------------------------------------\n"; 
    std::cout << "System with: " << "Queue size: " << size_of_q << " | Num. of items prod. per producer: " << no_of_jobs 
    << " | No. Producers: " << number_of_producers << " | No. of Consumers: " << number_of_consumers  
    << "\n----------------------------------------------------------------------------------------------------------" << std::endl; 
}
