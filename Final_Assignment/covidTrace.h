#define N_MACADDRESSES 500 // If a much larger number is required, check the data type of random_index in BTnearMe()
#define MAX_CONCURRENT_BTNEARME 5000 // Max number of possible concurrent threads for new macaddresses
#define MAX_CONCURRENT_CLOSE_CONTACTS 150000 // Max number of possible concurrent threads for close contacts (including duplicates)
#define MAX_BTNEARME_CALLS 300000
#define MAX_TESTCOVID_CALLS 500

// Time interval parameters
const double speed_factor = 0.01; // The code will run (1 / speed_factor) times faster (0.01 -> 100x)
const double sec_search_interval = 10 * speed_factor;
const double sec_save_recent = 4 * 60 * speed_factor;
const double sec_forget_recent = 20 * 60 * speed_factor;
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
    // hh:mm:ss:nnnnnnnnn
    int hours;
    int mins;
    int secs;
    long nanosecs;
} time_format;

typedef struct
{
    macaddress cur_macaddress;
    struct timespec timestamp;
} macaddress_time;

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
    pthread_t *pt_buf;
    uint16_t head, tail;
    bool empty;
    pthread_mutex_t mut;
    pthread_cond_t not_empty_cond;
} pthread_queue;

typedef struct
{
    pthread_queue *new_mac_queue;
    pthread_queue *close_contacts_queue;
} two_queues;

struct timespec seconds_to_timespec(float seconds);
void timespec_diff(struct timespec *a, struct timespec *b, struct timespec *result);
void timespec_add(struct timespec *a, struct timespec *b, struct timespec *result);
void timespec_to_time_format(struct timespec timestamp, time_format *converted_timestamp);
void init_macaddress_list();
macaddress_time *BTnearMe();
bool testCOVID();
void write_history();
void *new_macaddress_thread_func(void *macaddress_time_obj);
void *close_contact_thread_func(void *macaddress_time_obj);
void *thread_joiner_thread_func(void *queue);
void *BTnearMe_thread_func(void *queues);
void uploadContacts();
void *testCovid_thread_func(void *queue);
void *program_timeout_thread_func(void *queues);
void pthread_queue_init(size_t buffer_size, pthread_queue *pt_queue);
void pthread_queue_delete(pthread_queue *pt_queue);
