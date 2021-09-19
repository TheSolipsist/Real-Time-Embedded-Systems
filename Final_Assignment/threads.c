#define N_MACADDRESSES 1000

struct macaddress
{
    uint64_t val:48;
};

static struct macaddress recent_macaddress_list[24];
static struct macaddress macaddress_list[N_MACADDRESSES];

void init_macaddress_list()
{
    // TODO
    // Put some random macaddresses into macaddress_list
    // loop 1000 times: [create new address, and loop 48 times: [add 0/1 with 50% prob to address.val, left shift 1 bit]]
    // repeat N_MACADDRESSES times to fill macaddress_list
}

void periodic_macaddress()
{
    sleep(10);
}

void BTnearMe(struct macaddress random_macaddress)
{
    // get random value i from 0 to 999
    // change random_macaddress to macaddress_list[i]
}

int main()
{
    pthread_t mac_search, covid_test;

    return 0;
}