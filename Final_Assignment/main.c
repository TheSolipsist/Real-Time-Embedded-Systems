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
#define MAX_CLOSE_CONTACTS 150000 // Max number of possible concurrent threads for close contacts (including duplicates)
#define MAX_BTNEARME_CALLS 300000
#define MAX_TESTCOVID_CALLS 500

#define SEC_PER_DAY 86400
#define SEC_PER_HOUR 3600
#define SEC_PER_MIN 60

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
} pthread_queue;

macaddress macaddress_list[N_MACADDRESSES];
btnearme_history_struct btnearme_history[MAX_BTNEARME_CALLS];
testCovid_history_struct testCovid_history[MAX_TESTCOVID_CALLS];
pthread_queue pt_new_mac;
pthread_queue pt_close_contacts;
// recent_contacts holds the number of times that each macaddress has been seen 4-20 minutes ago
uint16_t recent_contacts[N_MACADDRESSES] = {0};
uint32_t close_contacts[N_MACADDRESSES] = {0};

uint32_t btnearme_history_index = 0;
uint32_t testCovid_history_index = 0;

pthread_mutex_t contacts_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t orchestrator_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t program_timeout_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t update_contacts_started_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t pt_not_empty_cond = PTHREAD_COND_INITIALIZER;

bool program_ended = false;
bool update_contacts_started = false;

struct timespec program_start_time;
struct timespec next_BTnearMe_time;
struct timespec next_testCovid_time;

void timespec_diff(struct timespec *a, struct timespec *b, struct timespec *result) 
{
    result->tv_sec  = a->tv_sec  - b->tv_sec;
    result->tv_nsec = a->tv_nsec - b->tv_nsec;
    if (result->tv_nsec < 0) 
    {
        --result->tv_sec;
        result->tv_nsec += 1000000000L;
    }
}

void timespec_add(struct timespec *a, struct timespec *b, struct timespec *result)
{
    result->tv_sec = a->tv_sec + b->tv_sec;
    result->tv_nsec = a->tv_nsec + b->tv_nsec;
    if (result->tv_nsec > 999999999)
    {
        ++(result->tv_sec);
        result->tv_nsec -= 1000000000L;
    }
}

void timespec_to_time_format(struct timespec timestamp, time_format *converted_timestamp) {
    // Converts a timespec to time_format
    // Inputs: the timespec, a reference to the time_format that will store the result
    // Form the seconds of the day
    long hms = timestamp.tv_sec % SEC_PER_DAY;
    // mod `hms` to insure in positive range of [0...SEC_PER_DAY)
    hms = (hms + SEC_PER_DAY) % SEC_PER_DAY;

    // Tear apart hms into h:m:s
    int hour = hms / SEC_PER_HOUR;
    int min = (hms % SEC_PER_HOUR) / SEC_PER_MIN;
    int sec = (hms % SEC_PER_HOUR) % SEC_PER_MIN; // or hms % SEC_PER_MIN
    
    converted_timestamp->hours = hour;
    converted_timestamp->mins = min;
    converted_timestamp->secs = sec;
    converted_timestamp->nanosecs = timestamp.tv_nsec; 
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

macaddress_time *BTnearMe()
{
    macaddress_time *newBT = malloc(sizeof(macaddress_time));
    // Get a random macaddress from macaddress_list
    uint16_t random_index = rand() % N_MACADDRESSES;
    // Time from the start of the program
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    timespec_diff(&current_time, &program_start_time, &(newBT->timestamp));
    // Macaddress
    newBT->cur_macaddress = macaddress_list[random_index];

    // Save info to history
    if(btnearme_history_index < MAX_BTNEARME_CALLS)
    {
        btnearme_history[btnearme_history_index].macaddress_time = newBT->timestamp;
        btnearme_history[btnearme_history_index++].generated_macaddress = newBT->cur_macaddress;
    }
    else
        printf("overflow in btnearme_history");
    
    return newBT;
}


bool testCOVID()
{
    // Get a random positive (10%) or negative(90%) covid test
    float res = (rand() % 10001) / 10000.0 ;
    bool test_res;
    test_res = res <= 0.1 ? true : false ;

    // Save info to history
    if(testCovid_history_index < MAX_TESTCOVID_CALLS)
    {
        // Time from the start of the program
        struct timespec current_time;
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        timespec_diff(&current_time, &program_start_time, &(testCovid_history[testCovid_history_index].testCovid_time));
        // Test result
        testCovid_history[testCovid_history_index++].testCovidResult = test_res;
    }
    else
        printf("overflow in testCovid_history");

    return test_res;
}

void write_history()
{
    // Writes the info from btnearme_history and testCovid_history to files
    // Write BTnearMe history
    FILE *ptr1;
    ptr1 = fopen("btnearme_history.bin", "ab");
    for(int i = 0; i < btnearme_history_index; i++)
    {   
        time_format converted_time;        
        timespec_to_time_format(btnearme_history[i].macaddress_time, &converted_time);
        fprintf(ptr1, "(%02d:%02d:%02d.%09ld) %" PRIu64 "\n", converted_time.hours, converted_time.mins, converted_time.secs, converted_time.nanosecs, btnearme_history[i].generated_macaddress.value);
    }
    fclose(ptr1);

    // Write testCOVID history
    FILE *ptr2;
    ptr2 = fopen("testcovid_history.bin", "ab");
    for(int i = 0; i < testCovid_history_index; i++)
    { 
        time_format converted_time;        
        timespec_to_time_format(testCovid_history[i].testCovid_time, &converted_time);
        fprintf(ptr2, "(%02d:%02d:%02d.%09ld) %d\n", converted_time.hours, converted_time.mins, converted_time.secs, converted_time.nanosecs, testCovid_history[i].testCovidResult);
    }
    fclose(ptr2);
}

void *new_macaddress_thread_func(void *macaddress_time_obj)
{
    macaddress_time *cur_macaddress_time_obj = (macaddress_time*)macaddress_time_obj;
    struct timespec next_time;
    pthread_mutex_lock(&contacts_mutex);
    if (program_ended)
    {
        free(macaddress_time_obj);
        pthread_mutex_unlock(&contacts_mutex);
        return NULL;
    }
    // Save to recent contacts
    timespec_add(&(cur_macaddress_time_obj->timestamp), &save_recent, &next_time);
    pthread_cond_timedwait(&program_timeout_cond, &contacts_mutex, &next_time);
    if (program_ended)
    {
        free(macaddress_time_obj);
        pthread_mutex_unlock(&contacts_mutex);
        return NULL;
    }
    recent_contacts[cur_macaddress_time_obj->cur_macaddress.value] += 1;
    // Delete from recent contacts
    timespec_add(&(cur_macaddress_time_obj->timestamp), &forget_recent, &next_time);
    pthread_cond_timedwait(&program_timeout_cond, &contacts_mutex, &next_time);
    recent_contacts[cur_macaddress_time_obj->cur_macaddress.value] -= 1;
    free(macaddress_time_obj);
    pthread_mutex_unlock(&contacts_mutex);
    return NULL;
}

void *orchestrator_new_macaddress_thread_func()
{
    pthread_mutex_lock(&orchestrator_mutex);
    if (pt_new_mac.empty)
    {
        pthread_cond_wait(&pt_not_empty_cond, &orchestrator_mutex);
    }
    pthread_mutex_unlock(&orchestrator_mutex);
    while (1)
    {
        pthread_join(pt_new_mac.pt_buf[pt_new_mac.head], NULL);
        pthread_mutex_lock(&orchestrator_mutex);
        pt_new_mac.head = (pt_new_mac.head + 1) % MAX_CONCURRENT_BTNEARME;
        if (pt_new_mac.head == pt_new_mac.tail)
        {
            pt_new_mac.empty = true;
            pthread_cond_wait(&pt_not_empty_cond, &orchestrator_mutex);
            if (program_ended)
            {
                pthread_mutex_unlock(&orchestrator_mutex);
                return NULL;
            }
        }
        pthread_mutex_unlock(&orchestrator_mutex);
    }
}

void *BTnearMe_thread_func()
{
    // Run 0 should occur immediately, so next_BTnearMe_time is initialized as program_start_time
    struct timespec next_BTnearMe_time = program_start_time;
    macaddress_time *macaddress_time_obj;
    pthread_mutex_lock(&contacts_mutex);
    while (1)
    {
        // thread waits until program shutdown signal or the search interval has passed
        pthread_cond_timedwait(&program_timeout_cond, &contacts_mutex, &next_BTnearMe_time);
        if (program_ended)
        {
            return NULL;
        }
        macaddress_time_obj = BTnearMe();
        if (recent_contacts[macaddress_time_obj->cur_macaddress.value] > 0)
        {
            close_contacts[macaddress_time_obj->cur_macaddress.value] += 1;
        }
        pthread_create(&pt_new_mac.pt_buf[pt_new_mac.tail], NULL, new_macaddress_thread_func, macaddress_time_obj);
        timespec_add(&search_interval, &next_BTnearMe_time, &next_BTnearMe_time);
        pthread_mutex_lock(&orchestrator_mutex);
        pt_new_mac.tail = (pt_new_mac.tail + 1) % MAX_CONCURRENT_BTNEARME;
        if (pt_new_mac.empty)
        {
            pt_new_mac.empty = false;
            pthread_cond_signal(&pt_not_empty_cond);
        }
        pthread_mutex_unlock(&orchestrator_mutex);
    }
}

void uploadContacts()
{
    FILE *fp;
    // Time from the start of the program
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    struct timespec time_from_start;
    timespec_diff(&current_time, &program_start_time, &time_from_start);

    // Write timestamp
    time_format converted_time;
    timespec_to_time_format(time_from_start, &converted_time);
    fp = fopen("upload_contacts.bin","ab");
    fprintf(fp, "(%02d:%02d:%02d.%09ld) ", converted_time.hours, converted_time.mins, converted_time.secs, converted_time.nanosecs);
    // Write close contacts
    for(int i = 0; i < N_MACADDRESSES; i++)
    {
        if(close_contacts[i] > 0)
        {
            fprintf(fp, "%" PRIu64 ",", macaddress_list[i].value);
        } 
    }
    fclose(fp);
}

void pthread_queue_init(size_t buffer_size, pthread_queue *pt_queue)
{
    pt_queue->pt_buf = malloc(buffer_size);
    pt_queue->head = 0;
    pt_queue->tail = 0;
    pt_queue->empty = true;
}

void pthread_queue_delete(pthread_queue *pt_queue)
{
    free(pt_queue->pt_buf);
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

    init_macaddress_list();

    pthread_queue_init(MAX_BTNEARME_CALLS * sizeof(pthread_t), &pt_new_mac);
    pthread_queue_init(MAX_CLOSE_CONTACTS * sizeof(pthread_t), &pt_close_contacts);

    pthread_t BTnearMe_t;
    pthread_t testCovid_t;
    pthread_t uploadContacts_t;
    pthread_t update_contacts_t;

    pthread_queue_delete(&pt_new_mac);
    pthread_queue_delete(&pt_close_contacts);
    
    return 0;
}