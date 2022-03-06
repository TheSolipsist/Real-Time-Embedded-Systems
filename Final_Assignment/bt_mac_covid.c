#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <inttypes.h>
#include <stdbool.h>
#include "covidTrace.h"

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

macaddress macaddress_list[N_MACADDRESSES];
btnearme_history_struct *btnearme_history;
testCovid_history_struct *testCovid_history;
// recent_contacts holds the number of times that each macaddress has been seen 4-20 minutes ago
uint16_t recent_contacts[N_MACADDRESSES] = {0};
uint32_t close_contacts[N_MACADDRESSES] = {0};

uint32_t btnearme_history_index = 0;
uint32_t testCovid_history_index = 0;

pthread_mutex_t recent_contacts_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t close_contacts_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t program_timeout_cond = PTHREAD_COND_INITIALIZER;

bool program_ended = false;

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

void timespec_to_time_format(struct timespec timestamp, time_format *converted_timestamp)
{
    // Form the seconds of the day
    long hms = timestamp.tv_sec % SEC_PER_DAY;
    // mod `hms` to ensure it is in positive range of [0...SEC_PER_DAY)
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
    macaddress_time newBT;
    macaddress_time *result;
    result = malloc(sizeof(macaddress_time));
    // Get a random macaddress from macaddress_list
    uint16_t random_index = rand() % N_MACADDRESSES;
    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);
    timespec_diff(&current_time, &program_start_time, &(newBT.timestamp));
    newBT.cur_macaddress = macaddress_list[random_index];
    result->timestamp = current_time;
    result->cur_macaddress = newBT.cur_macaddress;
    // Save info to history
    if(btnearme_history_index < MAX_BTNEARME_CALLS)
    {
        btnearme_history[btnearme_history_index].macaddress_time = newBT.timestamp;
        btnearme_history[btnearme_history_index++].generated_macaddress = newBT.cur_macaddress;
    }
    else
    {
        printf("Overflow in btnearme_history");
        fflush(stdout);
    }
    return result;
}


bool testCOVID()
{
    // Get a random positive (10%) or negative(90%) covid test
    float res = (rand() % 10001) / 10000.0;
    bool test_res;
    test_res = res <= COVID_POSITIVE_RATE ? true : false;

    // Save info to history
    if(testCovid_history_index < MAX_TESTCOVID_CALLS)
    {
        // Time from the start of the program
        struct timespec current_time;
        clock_gettime(CLOCK_REALTIME, &current_time);
        timespec_diff(&current_time, &program_start_time, &(testCovid_history[testCovid_history_index].testCovid_time));
        // Test result
        testCovid_history[testCovid_history_index++].testCovidResult = test_res;
    }
    else
    {
        printf("Overflow in testCovid_history\n");
        fflush(stdout);
    }
    return test_res;
}

void write_history()
{
    // Writes the info from btnearme_history and testCovid_history to files
    // Write BTnearMe history
    FILE *ptr1;
    ptr1 = fopen("btnearme_history", "wb");
    for(int i = 0; i < btnearme_history_index; i++)
    {   
        time_format converted_time;        
        timespec_to_time_format(btnearme_history[i].macaddress_time, &converted_time);
        fprintf(ptr1, "(%02d:%02d:%02d.%09ld) %" PRIu64 "\n", converted_time.hours, converted_time.mins, converted_time.secs, converted_time.nanosecs, btnearme_history[i].generated_macaddress.value);
    }
    fclose(ptr1);

    // Write testCOVID history
    FILE *ptr2;
    ptr2 = fopen("testcovid_history", "wb");
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
    pthread_mutex_lock(&recent_contacts_mutex);
    if (program_ended)
    {
        free(macaddress_time_obj);
        pthread_mutex_unlock(&recent_contacts_mutex);
        return NULL;
    }
    // Save to recent contacts
    timespec_add(&(cur_macaddress_time_obj->timestamp), &save_recent, &next_time);
    pthread_cond_timedwait(&program_timeout_cond, &recent_contacts_mutex, &next_time);
    if (program_ended)
    {
        free(macaddress_time_obj);
        pthread_mutex_unlock(&recent_contacts_mutex);
        return NULL;
    }
    recent_contacts[cur_macaddress_time_obj->cur_macaddress.value] += 1;
    // Delete from recent contacts
    timespec_add(&(cur_macaddress_time_obj->timestamp), &forget_recent, &next_time);
    pthread_cond_timedwait(&program_timeout_cond, &recent_contacts_mutex, &next_time);
    recent_contacts[cur_macaddress_time_obj->cur_macaddress.value] -= 1;
    free(macaddress_time_obj);
    pthread_mutex_unlock(&recent_contacts_mutex);

    return NULL;
}

void *close_contact_thread_func(void *macaddress_time_obj)
{
    pthread_mutex_lock(&recent_contacts_mutex);
    // The following 2 pointers are used because macaddress_time_obj may have its memory freed from new_macaddress_thread_func
    // (Although it's highly unlikely that the thread won't be active for 20 minutes)
    macaddress_time *cur_macaddress_time_obj = malloc(sizeof(macaddress_time));
    macaddress_time *arg_macaddress_time_obj = (macaddress_time*) macaddress_time_obj;
    cur_macaddress_time_obj->cur_macaddress.value = arg_macaddress_time_obj->cur_macaddress.value;
    cur_macaddress_time_obj->timestamp = arg_macaddress_time_obj->timestamp;
    pthread_mutex_unlock(&recent_contacts_mutex);
    struct timespec delete_contact_time;
    pthread_mutex_lock(&close_contacts_mutex);
    if (program_ended)
    {
        free(cur_macaddress_time_obj);
        pthread_mutex_unlock(&close_contacts_mutex);
        return NULL;
    }
    // Save to close contacts
    close_contacts[cur_macaddress_time_obj->cur_macaddress.value] += 1;
    // Delete from close contacts
    timespec_add(&(cur_macaddress_time_obj->timestamp), &forget_contacts, &delete_contact_time);
    pthread_cond_timedwait(&program_timeout_cond, &close_contacts_mutex, &delete_contact_time);
    close_contacts[cur_macaddress_time_obj->cur_macaddress.value] -= 1;
    free(cur_macaddress_time_obj);
    pthread_mutex_unlock(&close_contacts_mutex);

    return NULL;
}

void *thread_joiner_thread_func(void *queue)
{
    pthread_queue *pt_queue = (pthread_queue *)queue;
    pthread_mutex_lock(&(pt_queue->mut));
    if (pt_queue->empty)
    {
        pthread_cond_wait(&(pt_queue->not_empty_cond), &(pt_queue->mut));
    }
    pthread_mutex_unlock(&(pt_queue->mut));
    while (1)
    {
        pthread_join(pt_queue->pt_buf[pt_queue->head], NULL);
        pthread_mutex_lock(&(pt_queue->mut));
        pt_queue->head = (pt_queue->head + 1) % pt_queue->max_size;
        if (pt_queue->head == pt_queue->tail)
        {
            if (program_ended)
            {
                pthread_mutex_unlock(&(pt_queue->mut));
                return NULL;
            }
            pt_queue->empty = true;
            pthread_cond_wait(&(pt_queue->not_empty_cond), &(pt_queue->mut));
            if (program_ended)
            {
                pthread_mutex_unlock(&(pt_queue->mut));
                return NULL;
            }
        }
        pthread_mutex_unlock(&(pt_queue->mut));
    }
}

void *BTnearMe_thread_func(void *queues)
{
    pthread_queue *pt_new_mac = ((two_queues *)queues)->new_mac_queue;
    pthread_queue *pt_close_contacts = ((two_queues *)queues)->close_contacts_queue;
    // Run 0 should occur immediately, so next_BTnearMe_time is initialized as program_start_time
    struct timespec next_BTnearMe_time = program_start_time;
    macaddress_time *macaddress_time_obj;
    pthread_mutex_lock(&recent_contacts_mutex);
    while (1)
    {
        // Thread waits until program shutdown signal or until the search interval has passed
        pthread_cond_timedwait(&program_timeout_cond, &recent_contacts_mutex, &next_BTnearMe_time);
        if (program_ended)
        {
            pthread_mutex_unlock(&recent_contacts_mutex);
            return NULL;
        }
        macaddress_time_obj = BTnearMe();
        if (recent_contacts[macaddress_time_obj->cur_macaddress.value] > 0)
        {
            pthread_create(&(pt_close_contacts->pt_buf[pt_close_contacts->tail]), NULL, close_contact_thread_func, macaddress_time_obj);
            pthread_mutex_lock(&(pt_close_contacts->mut));
            pt_close_contacts->tail = (pt_close_contacts->tail + 1) % pt_close_contacts->max_size;
            if (pt_close_contacts->empty)
            {
                pt_close_contacts->empty = false;
                pthread_cond_signal(&(pt_close_contacts->not_empty_cond));
            }
            pthread_mutex_unlock(&(pt_close_contacts->mut));
        }
        pthread_create(&(pt_new_mac->pt_buf[pt_new_mac->tail]), NULL, new_macaddress_thread_func, macaddress_time_obj);
        timespec_add(&search_interval, &next_BTnearMe_time, &next_BTnearMe_time);
        pthread_mutex_lock(&(pt_new_mac->mut));
        pt_new_mac->tail = (pt_new_mac->tail + 1) % pt_new_mac->max_size;
        if (pt_new_mac->empty)
        {
            pt_new_mac->empty = false;
            pthread_cond_signal(&(pt_new_mac->not_empty_cond));
        }
        pthread_mutex_unlock(&(pt_new_mac->mut));
    }
}

void uploadContacts()
{
    FILE *fp;
    // Time from the start of the program
    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);
    struct timespec time_from_start;
    timespec_diff(&current_time, &program_start_time, &time_from_start);

    // Write timestamp
    time_format converted_time;
    timespec_to_time_format(time_from_start, &converted_time);
    fp = fopen("upload_contacts","ab");
    fprintf(fp, "(%02d:%02d:%02d.%09ld) ", converted_time.hours, converted_time.mins, converted_time.secs, converted_time.nanosecs);
    // Write close contacts
    for(int i = 0; i < N_MACADDRESSES; i++)
    {
        if(close_contacts[i] > 0)
        {
            fprintf(fp, "%" PRIu64 ",", macaddress_list[i].value);
        } 
    }
    fprintf(fp, "\n");
    fclose(fp);
}

void *testCovid_thread_func(void *queue)
{
    pthread_queue *pt_close_contacts = (pthread_queue *) queue;
    struct timespec next_testCovid_time = program_start_time;
    bool result;
    pthread_mutex_lock(&(pt_close_contacts->mut));
    while (1)
    {
        timespec_add(&next_testCovid_time, &test_interval, &next_testCovid_time);
        pthread_cond_timedwait(&program_timeout_cond, &(pt_close_contacts->mut), &next_testCovid_time);
        if (program_ended)
        {
            pthread_mutex_unlock(&(pt_close_contacts->mut));
            return NULL;
        }
        result = testCOVID();
        if (result)
        {
            uploadContacts();
        }
    }
}

void *program_timeout_thread_func(void *queues)
{
    pthread_mutex_t fakeMutex;
    pthread_cond_t fakeCond;
    pthread_mutex_init(&fakeMutex, NULL);
    pthread_cond_init(&fakeCond, NULL);
    struct timespec program_end_time;
    timespec_add(&program_start_time, &program_timeout, &program_end_time);

    two_queues *pt_queues = (two_queues *) queues;
    pthread_mutex_lock(&fakeMutex);
    pthread_cond_timedwait(&fakeCond, &fakeMutex, &program_end_time);
    pthread_mutex_unlock(&fakeMutex);

    pthread_mutex_lock(&recent_contacts_mutex);
    pthread_mutex_lock(&close_contacts_mutex);
    pthread_mutex_lock(&(pt_queues->close_contacts_queue->mut));
    pthread_mutex_lock(&(pt_queues->new_mac_queue->mut));

    program_ended = true;
    pthread_cond_broadcast(&program_timeout_cond);
    pthread_cond_signal(&(pt_queues->close_contacts_queue->not_empty_cond));
    pthread_cond_signal(&(pt_queues->new_mac_queue->not_empty_cond));

    pthread_mutex_unlock(&recent_contacts_mutex);
    pthread_mutex_unlock(&close_contacts_mutex);
    pthread_mutex_unlock(&(pt_queues->close_contacts_queue->mut));
    pthread_mutex_unlock(&(pt_queues->new_mac_queue->mut));

    pthread_mutex_destroy(&fakeMutex);
    pthread_cond_destroy(&fakeCond);
    
    return NULL;
}
void pthread_queue_init(size_t buffer_size, pthread_queue *pt_queue)
{
    pt_queue->pt_buf = (pthread_t *)malloc(buffer_size * sizeof(pthread_t));
    pt_queue->head = 0;
    pt_queue->tail = 0;
    pt_queue->max_size = buffer_size;
    pt_queue->empty = true;
    pthread_mutex_init(&(pt_queue->mut), NULL);
    pthread_cond_init(&(pt_queue->not_empty_cond), NULL);
}

void pthread_queue_delete(pthread_queue *pt_queue)
{
    free(pt_queue->pt_buf);
    pthread_mutex_destroy(&(pt_queue->mut));
    pthread_cond_destroy(&(pt_queue->not_empty_cond));
}

int main()
{
    srand(time(NULL));

    // If the upload_contacts file already exists, discard its contents
    FILE *fp;
    fp = fopen("upload_contacts","wb");
    fclose(fp);

    search_interval = seconds_to_timespec(sec_search_interval);
    save_recent = seconds_to_timespec(sec_save_recent);
    forget_recent = seconds_to_timespec(sec_forget_recent);
    forget_contacts = seconds_to_timespec(sec_forget_contacts);
    test_interval = seconds_to_timespec(sec_test_interval);
    program_timeout = seconds_to_timespec(sec_program_timeout);

    init_macaddress_list();

    pthread_queue *pt_new_mac = malloc(sizeof(pthread_queue));
    pthread_queue *pt_close_contacts = malloc(sizeof(pthread_queue));
    
    two_queues *queues;

    btnearme_history = malloc(MAX_BTNEARME_CALLS * sizeof(btnearme_history_struct));
    testCovid_history = malloc(MAX_TESTCOVID_CALLS * sizeof(btnearme_history_struct));

    pthread_queue_init(MAX_CONCURRENT_BTNEARME, pt_new_mac);
    pthread_queue_init(MAX_CONCURRENT_CLOSE_CONTACTS, pt_close_contacts);

    queues = malloc(sizeof(two_queues));
    queues->new_mac_queue = pt_new_mac;
    queues->close_contacts_queue = pt_close_contacts;

    pthread_t BTnearMe_thread;
    pthread_t joiner_new_macaddresses_thread;
    pthread_t joiner_close_contacts_thread;
    pthread_t testCOVID_thread;
    pthread_t program_timeout_thread;

    clock_gettime(CLOCK_REALTIME, &program_start_time);

    pthread_create(&BTnearMe_thread, NULL, BTnearMe_thread_func, queues);
    pthread_create(&joiner_new_macaddresses_thread, NULL, thread_joiner_thread_func, pt_new_mac);
    pthread_create(&joiner_close_contacts_thread, NULL, thread_joiner_thread_func, pt_close_contacts);
    pthread_create(&testCOVID_thread, NULL, testCovid_thread_func, pt_close_contacts);
    pthread_create(&program_timeout_thread, NULL, program_timeout_thread_func, queues);

    pthread_join(BTnearMe_thread, NULL);
    pthread_join(joiner_new_macaddresses_thread, NULL);
    pthread_join(joiner_close_contacts_thread, NULL);
    pthread_join(testCOVID_thread, NULL);
    pthread_join(program_timeout_thread, NULL);

    write_history();

    pthread_queue_delete(pt_new_mac);
    pthread_queue_delete(pt_close_contacts);
    free(pt_new_mac);
    free(pt_close_contacts);
    free(queues);
    free(btnearme_history);
    
    return 0;
}