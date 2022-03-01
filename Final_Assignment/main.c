#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <inttypes.h>

// N_MACADDRESSES should be less than 2^16
// If a larger number is required, change the data type of random_index in BTnearMe()
#define N_MACADDRESSES 2000
#define SEARCH_INTERVAL 10

struct macaddress
{
    uint64_t value : 48;
};

static struct macaddress recent_macaddress_list[24];
static struct macaddress macaddress_list[N_MACADDRESSES];

void init_macaddress_list()
{
    // Initialize macaddress_list by filling it with random macaddresses
    for (int i = 0; i < N_MACADDRESSES; ++i)
    {
        macaddress_list[i].value = 0;
        for (int byte_i = 0; byte_i < 6; ++byte_i)
        {
            // Getting a random byte, since we don't know the limitations of RAND_MAX (minimum supported should be 0x7FFF)
            uint8_t rand_byte = rand();
            // The random byte is placed on its correct position (48-bit macaddress has 6 bytes)
            macaddress_list[i].value += rand_byte << (byte_i * 8);
            // Based on this approach, the probability of getting the same macaddress twice is negligible for the sample
            // size defined by N_MACADDRESSES (for 2000, the probability is around 10^-9)
        }
    }
}

struct macaddress BTnearMe()
{
    // Get a random macaddress from macaddress_list
    uint16_t random_index = rand() % N_MACADDRESSES;
    return macaddress_list[random_index];
}

// -------------------------------
// USED FOR TESTING PURPOSES ONLY!
void test_rand()
{
    uint16_t random_index;
    uint64_t occurences[N_MACADDRESSES] = {0};
    for (uint64_t i = 0; i < 100000000; ++i)
    {
        random_index = rand() % N_MACADDRESSES;
        occurences[random_index] += 1;
    }
    FILE *data;
    data = fopen("occurences.txt", "w");
    if (data == NULL)
    {
        fprintf(stderr, "Error opening file\n");
        exit(1);
    }
    printf("Writing occurences...\n");
    for (uint16_t i = 0; i < N_MACADDRESSES; ++i)
    {
        fprintf(data, "%" PRIu64 "\n", occurences[i]);
    }
    fclose(data);
}

void test_init()
{
    uint64_t NUM_TESTS = 10000;
    for (uint64_t i = 0; i < NUM_TESTS; ++i)
    {
        for (int j = 0; j < N_MACADDRESSES; ++j)
        {
            macaddress_list[j].value = 0;
        }
        init_macaddress_list();
        for (int j = 0; j < N_MACADDRESSES; ++j)
        {
            for (int z = j + 1; z < N_MACADDRESSES; ++z)
            {
                if (macaddress_list[j].value == macaddress_list[z].value)
                {
                    printf("Houston, we got a problem.\n");
                }
            }
        }
    }
}
// TESTING ENDS HERE!
// -------------------------------


int main()
{
    srand(time(NULL));

    pthread_t mac_search, covid_test;

    // init_macaddress_list();

    test_init();

    return 0;
}