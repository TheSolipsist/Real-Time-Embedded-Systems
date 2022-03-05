#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <inttypes.h>
#include <stdbool.h>

#define N_MACADDRESSES 2000 // If a much larger number is required, check the data type of random_index in BTnearMe()
#define MAX_CONCURRENT_BTNEARME 5000 // Max number of possible concurrent threads for new macaddresses
#define MAX_CLOSE_CONTACTS 5000
#define MAX_BTNEARME_CALLS 300000
#define MAX_TESTCOVID_CALLS 500

struct timespec seconds_to_timespec(float seconds)
{
    struct timespec result;
    result.tv_sec = (int) seconds;
    result.tv_nsec = (int)(1E9 * (seconds - result.tv_sec));
    return result;
}

// Time interval parameters
const double speed_factor = 0.01; // The code will run (1 / speed_factor) times faster (0.01 -> 100x)
const double sec_search_interval = 10 * speed_factor;
const double sec_save_recent = 4 * 60 * speed_factor;
const double sec_forget_recent = 16 * 60 * speed_factor;
const double sec_forget_contacts = 14 * 24 * 60 * 60 * speed_factor;
const double sec_test_interval = 4 * 60 * 60 * speed_factor;
const double sec_program_timeout = 30 * 24 * 60 * 60 * speed_factor;

// struct timespec variables that correspond to the above parameters, initialized in main()
struct timespec search_interval;
struct timespec save_recent;
struct timespec forget_recent;
struct timespec forget_contacts;
struct timespec test_interval;
struct timespec program_timeout;

typedef struct
{
    uint64_t value : 48;
} macaddress;

typedef struct
{
    macaddress *buffer;
    uint16_t head, tail, size;
} queue;

typedef struct
{
    macaddress generated_macaddress; 
    struct timespec macaddress_time;
} btnearme_history_struct;

typedef struct
{
    bool testCovidResult;
    struct timespec testCovid_time;
} testCovid_history_struct;

typedef struct
{
    pthread_t pt_arr[MAX_CONCURRENT_BTNEARME];
    uint16_t head, tail;
    bool empty;
} pthread_struct;

macaddress macaddress_list[N_MACADDRESSES];
btnearme_history_struct btnearme_history[MAX_BTNEARME_CALLS];
testCovid_history_struct testCovid_history[MAX_TESTCOVID_CALLS];
pthread_struct pthreads_new_macaddress[MAX_CONCURRENT_BTNEARME];
// recent_contacts holds the number of times that each macaddress has been seen 4-20 minutes ago
uint16_t recent_contacts[N_MACADDRESSES] = {0};
bool close_contacts[N_MACADDRESSES];

pthread_mutex_t contacts_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t orchestrator_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t program_timeout_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t new_macaddress_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t update_contacts_started_cond = PTHREAD_COND_INITIALIZER;

bool program_ended = false;
bool update_contacts_started = false;

struct timespec program_start_time;
struct timespec next_BTnearMe_time;
struct timespec next_testCovid_time;

void timespec_add(struct timespec *a, struct timespec *b, struct timespec *result) {
    result->tv_sec = a->tv_sec + b->tv_sec;
    result->tv_nsec = a->tv_nsec + b->tv_nsec;
    if (result->tv_nsec > 999999999)
    {
        ++(result->tv_sec);
        result->tv_nsec -= 1000000000L;
    }
}

void init_macaddress_list()
{
    // Initialize macaddress_list by filling it with different macaddresses
    for (int i = 0; i < N_MACADDRESSES; ++i)
    {
        // The macaddress values are consecutive integers to ensure uniqueness
        macaddress_list[i].value = i;
    }
}

macaddress BTnearMe()
{
    // Get a random macaddress from macaddress_list
    uint16_t random_index = rand() % N_MACADDRESSES;
    return macaddress_list[random_index];
}

bool testCovid()
{
}


void *update_contacts_func()
{
    pthread_mutex_lock(&contacts_mutex);
    while (1)
    {
        update_contacts_started = true;
        pthread_cond_signal(&update_contacts_started_cond);
        pthread_cond_wait(&new_macaddress_cond, &contacts_mutex);
    }
}

void *orchestrator_new_macaddress_thread_func()
{

}

void *BTnearMe_thread_func()
{
    // Run 0 should occur immediately, so next_BTnearMe_time is initialized as program_start_time
    struct timespec next_BTnearMe_time = program_start_time;
    macaddress new_macaddress;
    if (!update_contacts_started)
    {
        pthread_cond_wait(&update_contacts_started_cond, &contacts_mutex);
    }
    pthread_mutex_lock(&contacts_mutex);
    while (1)
    {
        // thread waits until program shutdown signal or the search interval has passed
        pthread_cond_timedwait(&program_timeout_cond, &contacts_mutex, &next_BTnearMe_time);
        new_macaddress = BTnearMe();
        pthread_cond_signal(&new_macaddress_cond);
        timespec_add(&search_interval, &next_BTnearMe_time, &next_BTnearMe_time);


    }
}

int main()
{
    srand(time(NULL));

    search_interval = seconds_to_timespec(sec_search_interval);
    save_recent = seconds_to_timespec(sec_save_recent);
    forget_recent = seconds_to_timespec(sec_forget_recent);
    forget_contacts = seconds_to_timespec(sec_forget_contacts);
    test_interval = seconds_to_timespec(sec_test_interval);
    program_timeout = seconds_to_timespec(sec_program_timeout);

    pthread_t BTnearMe_t;
    pthread_t testCovid_t;
    pthread_t uploadContacts_t;
    pthread_t update_contacts_t;

    init_macaddress_list();
    clock_gettime(CLOCK_MONOTONIC, &program_start_time);

    printf("%jd.%09ld\n", (intmax_t)search_interval.tv_sec, search_interval.tv_nsec);

    return 0;
}