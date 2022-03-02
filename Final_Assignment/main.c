#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <inttypes.h>

#define N_MACADDRESSES 2000 // If a much larger number is required, check the data type of random_index in BTnearMe()

// Time interval parameters
const double speed_factor = 0.01; // 0.01 means the code will run 100x faster
const double search_interval = 10 * speed_factor;
const double save_recent = 4 * 60 * speed_factor;
const double forget_recent = 16 * 60 * speed_factor;
const double forget_contacts = 14 * 24 * 60 * 60 * speed_factor;
const double test_interval = 4 * 60 * 60 * speed_factor;

struct macaddress
{
    uint64_t value : 48;
};

typedef struct
{
    struct macaddress *buffer;
    uint16_t head, tail, size;

} queue;

static struct macaddress macaddress_list[N_MACADDRESSES];
static queue *recent_contacts;
static queue *close_contacts;
static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void init_macaddress_list()
{
    // Initialize macaddress_list by filling it with different macaddresses
    for (int i = 0; i < N_MACADDRESSES; ++i)
    {
        // The macaddress values are consecutive integers to ensure uniqueness
        macaddress_list[i].value = i;
    }
}

struct macaddress BTnearMe()
{
    // Get a random macaddress from macaddress_list
    uint16_t random_index = rand() % N_MACADDRESSES;
    return macaddress_list[random_index];
}

int main()
{
    srand(time(NULL));

    pthread_t mac_search;
    pthread_t covid_test;


    init_macaddress_list();

    return 0;
}