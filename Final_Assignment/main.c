#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <inttypes.h>

// If a larger number is required, change the data type of random_index in BTnearMe()
#define N_MACADDRESSES 2000
#define SEARCH_INTERVAL 10

struct macaddress
{
    uint64_t value : 48;
};

static struct macaddress macaddress_list[N_MACADDRESSES];
static struct macaddress *recent_contacts;
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