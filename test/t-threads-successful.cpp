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

void consumer(uint64_t queue_offset, std::promise<uint64_t> &offset)
{
    //sleep(1);
    std::cout<<"consumer0"<<std::endl;
    cxl_shm shm = cxl_shm(length, shm_id);
    std::cout<<"consumer1"<<std::endl;
    shm.thread_init();
    std::cout<<"consumer2"<<std::endl;
    void* start = shm.get_start();
    std::cout<<"consumer3"<<std::endl;
    CXLRef r1 = shm.cxl_unwrap(queue_offset);
    std::cout<<"consumer4 r1.get_tbr():" << r1.get_tbr() <<std::endl;
    std::cout<<"consumer4 r1.get_tbr()->pptr:" << r1.get_tbr()->pptr <<std::endl;
    offset.set_value(r1.get_tbr()->pptr);
    std::cout<<"consumer5"<<std::endl;
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

    CHECK_BODY("thread init") {
        shm.thread_init();
        result = (shm.get_thread_id() != 0);
    }

    CHECK_BODY("malloc and free") {
        CXLRef ref = shm.cxl_malloc(32, 0);
        result = (ref.get_tbr() != NULL && ref.get_addr() != NULL);
    };

    // CHECK_BODY("wrap ref") {
    //     CXLRef ref = shm.cxl_malloc(32, 0);
    //     uint64_t addr = shm.cxl_wrap(ref);
    //     result = (addr != 0);
    // };

    CHECK_BODY("data transfer") {
        CXLRef r1 = shm.cxl_malloc(100, 0);
        uint64_t queue_offset1 = shm.create_msg_queue(2);
        uint64_t queue_offset2 = shm.create_msg_queue(4);
        shm.sent_to(queue_offset1, r1);
        shm.sent_to(queue_offset2, r1);
        std::promise<uint64_t> offset_1;
        std::promise<uint64_t> offset_2;
        std::thread t1(consumer, queue_offset1, std::ref(offset_1));
        
        sleep(1);
        std::thread t2(consumer, queue_offset2, std::ref(offset_2));
        t1.join();
        
        sleep(1);
        t2.join();

        // sleep(1);
        
        std::cout  << "1111: status"  << std::endl;
        //auto status = offset_2.get_future().get();
        //std::cout  << "1111: status" << status <<"r1.get_tbr()->pptr" <<r1.get_tbr()->pptr << std::endl;
        //result = (status == r1.get_tbr()->pptr);
    };

    shmctl(shm_id, IPC_RMID, NULL);

    return print_test_summary();
}