#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#define N_MACADDRESSES 2000

struct macaddress
{
    uint64_t value : 48;
};

static struct macaddress recent_macaddress_list[24];
static struct macaddress macaddress_list[N_MACADDRESSES];

uint64_t lfsr_get_next(uint64_t lfsr_seed, uint8_t bits_to_shift)
{
    /*
    LFSR implementation for pseudo-random number generation
    Feedback function: XOR (output bit, next bit)
    Arguments:
        uint64_t lfsr_seed: the seed used for the generation of the next random number
        uint8_t bits_to_shift: the number of bits to shift left (max: 64)
    */

    uint64_t lfsr_next;

    uint8_t output_bit = lfsr_seed & 1;
    lfsr_next = lfsr_seed >> 1;
    lfsr_next += (output_bit ^ (lfsr_next & 1)) << bits_to_shift;

    return lfsr_next;
}

void init_macaddress_list()
{
    // Randomly initialize the seed
    uint64_t seed = 38483;
    // For 48-bit random numbers, the LFSR needs to shift the feedback bit 47 times to the left
    uint8_t bits_to_shift = 47;
    for (int i = 0; i < N_MACADDRESSES; ++i)
    {
        seed = lfsr_get_next(seed, 47);
        macaddress_list[i].value = seed;
    }
}

static uint64_t btnearme_seed = 138921;
// struct macaddress BTnearMe()
uint8_t BTnearMe()
{
    // Get a random macaddress from macaddress_list
    btnearme_seed = lfsr_get_next(btnearme_seed, 63);
    uint8_t random_index = btnearme_seed % N_MACADDRESSES;
    // return macaddress_list[random_index];
    return random_index;
}

int main()
{
    pthread_t mac_search, covid_test;

    init_macaddress_list();
    // for (int i = 0; i < N_MACADDRESSES; ++i)
    // {
    //     for (int j = i + 1; j < N_MACADDRESSES; ++j)
    //     {
    //         if (macaddress_list[i].value == macaddress_list[j].value)
    //         {
    //             printf("Problem!!\n");
    //         }
    //     }
    // }

    long int occurances[N_MACADDRESSES] = {0};
    uint8_t random_num;
    for (int i = 0; i < 100000; ++i)
    {
        random_num = BTnearMe();
        occurances[random_num] += 1;
    }
    for (int i = 0; i < N_MACADDRESSES; ++i)
    {
        printf("%ld\n", occurances[i]);
    }
    return 0;
}