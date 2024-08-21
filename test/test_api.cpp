#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <thread>
#include <unistd.h>
#include <future>

#include "cxlmalloc.h"
#include "cxlmalloc-internal.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include "test_helper.h"

size_t length;
int shm_id;

int DATA_SIZE_BLOCK = 128;
int DATA_SIZE_MESSAGE = 128;
int counter = DATA_SIZE_MESSAGE / DATA_SIZE_BLOCK;

std::atomic<bool> firstSendDone(false);
std::atomic<bool> firstUnwrapDone(false);

void consumer(uint64_t queue_offset, std::promise<uint64_t> &offset)
{
    //sleep(1);
    std::cout<<"consumer0"<<std::endl;
    cxl_shm shm = cxl_shm(length, shm_id);
    std::cout<<"consumer1"<<std::endl;
    shm.thread_init();
    std::cout<<"consumer2"<<std::endl;
    void* start = shm.get_start();
    std::cout<<"consumer3 start: " << start <<std::endl;
    auto result = 1;
    for (int i = 0; i < counter; i ++) {
        CXLRef r1 = shm.cxl_unwrap_mend(queue_offset);
        std::atomic_thread_fence(std::memory_order_release);
        firstUnwrapDone.store(true, std::memory_order_release);
        std::cout<<"consumer4 r1.get_tbr():" << r1.get_tbr() <<std::endl;
        std::cout<<"consumer4 r1.get_tbr()->pptr:" << r1.get_tbr()->pptr <<std::endl;
        std::cout<<"consumer5"<<std::endl;
        
    }
    offset.set_value(result);
}

void consumer_t2(uint64_t queue_offset, std::promise<uint64_t> &offset)
{
    //sleep(1);
    std::cout<<"consumer_t2 consumer0"<<std::endl;
    cxl_shm shm = cxl_shm(length, shm_id);
    std::cout<<"consumer_t2 consumer1"<<std::endl;
    shm.thread_init();
    std::cout<<"consumer_t2 consumer2"<<std::endl;
    void* start = shm.get_start();
    std::cout<<"consumer_t2 consumer3  start: " << start <<std::endl;
    
    auto result = 1;
    for (int i = 0; i < counter; i ++) {
        while (!firstUnwrapDone.load(std::memory_order_release));
        std::atomic_thread_fence(std::memory_order_acquire);
        CXLRef r1 = shm.cxl_unwrap_mend(queue_offset);
        std::cout<<"consumer_t2 consumer4 r1.get_tbr():" << r1.get_tbr() <<std::endl;
        std::cout<<"consumer_t2 consumer4 r1.get_tbr()->pptr:" << r1.get_tbr()->pptr <<std::endl;
        std::cout<<"consumer_t2 consumer5"<<std::endl;
    }
    offset.set_value(result);
}


int main()
{
    using namespace std;
    length = (ZU(1) << 28);
    shm_id = shmget(100, length, IPC_CREAT|0664);
    
    shmctl(shm_id, IPC_RMID, NULL);


    length = (ZU(1) << 28);
    shm_id = shmget(100, length, IPC_CREAT|0664);
    cxl_shm shm = cxl_shm(length, shm_id);
    
    std::cout  << "1" << std::endl;
    shm.thread_init();
    
    std::cout  << "1" << std::endl;

    uint64_t queue_offset1 = shm.create_msg_queue(2);
    uint64_t queue_offset2 = shm.create_msg_queue(4);
    std::promise<uint64_t> offset_1;
    std::promise<uint64_t> offset_2;
    std::thread t1(consumer, queue_offset1, std::ref(offset_1));
    //sleep(1);
    std::thread t2(consumer_t2, queue_offset2, std::ref(offset_2));
    CXLRef r1 = shm.cxl_malloc(DATA_SIZE_BLOCK, 0);
    

    for (int i = 0; i < counter; i ++) {
        
        std::cout<<"send start t1: " << queue_offset1 <<std::endl;
        shm.sent_to(queue_offset1, r1);
        //sleep(1);

        std::atomic_thread_fence(std::memory_order_release);
        firstSendDone.store(true, std::memory_order_release);
        
        while (!firstSendDone.load(std::memory_order_release));
        std::atomic_thread_fence(std::memory_order_acquire);

        std::cout<<"send start t2: " << queue_offset1 <<std::endl;
        shm.sent_to(queue_offset2, r1);
        
        //sleep(1);
    }
    
    // std::thread t1(consumer, queue_offset1, std::ref(offset_1));
    // sleep(1);
    // std::thread t2(consumer_t2, queue_offset2, std::ref(offset_2));
    t1.join();
    
    t2.join();

    
    std::cout  << "1111: status"  << std::endl;
    //auto status = offset_2.get_future().get();
    //std::cout  << "1111: status" << status <<"r1.get_tbr()->pptr" <<r1.get_tbr()->pptr << std::endl;
    //result = (status == r1.get_tbr()->pptr);

    shmctl(shm_id, IPC_RMID, NULL);

    return print_test_summary();
}