#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <inttypes.h>
#include <stdbool.h>

#define N_MACADDRESSES 200 // If a much larger number is required, check the data type of random_index in BTnearMe()

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

macaddress macaddress_list[N_MACADDRESSES];
queue *recent_contacts;
queue *close_contacts; 
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t program_timeout_cond = PTHREAD_COND_INITIALIZER;

struct timespec program_start_time;
struct timespec next_BTnearMe_time;
struct timespec next_testCovid_time;
struct timespec next_;


void init_macaddress_list()
{
    // Initialize macaddress_list by filling it with different macaddresses
    for (int i = 0; i < N_MACADDRESSES; ++i)
    {
        // The macaddress values are consecutive integers to ensure uniqueness
        macaddress_list[i].value = i;
    }
}

bool testCovid()
{
}

void *BTnearMe_thread_func(void *args)
{
    struct timespec next_BTnearMe_time = {.tv_sec = 0, .tv_nsec = 0};
    pthread_mutex_lock(&mut);
    while (1)
    {
        // timespecadd()
    }
}

macaddress BTnearMe()
{
    // Get a random macaddress from macaddress_list
    uint16_t random_index = rand() % N_MACADDRESSES;
    return macaddress_list[random_index];
}

void timespec_add(struct timespec *a, struct timespec *b, struct timespec *result) {
    result->tv_sec = a->tv_sec + b->tv_sec;
    result->tv_nsec = a->tv_nsec + b->tv_nsec;
    if (result->tv_nsec > 999999999)
    {
        ++(result->tv_sec);
        result->tv_nsec -= 1000000000L;
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

    init_macaddress_list();
    clock_gettime(CLOCK_MONOTONIC, &program_start_time);

    printf("%jd.%09ld\n", (intmax_t)search_interval.tv_sec, search_interval.tv_nsec);

    return 0;
}