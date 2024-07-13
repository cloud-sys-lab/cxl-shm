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
    sleep(3);
    cxl_shm shm = cxl_shm(length, shm_id);
    shm.thread_init();
    void* start = shm.get_start();
    CXLRef r1 = shm.cxl_unwrap(queue_offset);
    offset.set_value(r1.get_tbr()->pptr);
}

void consumer_wrc(uint64_t queue_offset, std::promise<uint64_t> &offset)
{
    sleep(3);
    cxl_shm shm = cxl_shm(length, shm_id);
    shm.thread_init();
    void* start = shm.get_start();
    CXLRef r1 = shm.cxl_unwrap_wrc(queue_offset);
    uint64_t obj_offset = r1.data;
    CXLObj* cxl_obj1 = (CXLObj*)((uintptr_t)start + obj_offset);
    while (cxl_obj1->writer_count != 0) {
        cxl_obj1->reader_count++;
    }
    offset.set_value(r1.get_tbr()->pptr);
}

int main()
{
    using namespace std;
    length = (ZU(1) << 28);
    shm_id = shmget(100, length, IPC_CREAT|0664);
    cxl_shm shm = cxl_shm(length, shm_id);
    shm.thread_init();

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

    // CHECK_BODY("data transfer") {
    //     CXLRef r1 = shm.cxl_malloc(1008, 0);
    //     uint64_t queue_offset = shm.create_msg_queue(2);
    //     shm.sent_to(queue_offset, r1);
    //     std::promise<uint64_t> offset_2;
    //     std::thread t1(consumer, queue_offset, std::ref(offset_2));
    //     t1.join();
    //     auto status = offset_2.get_future().get();
    //     result = (status == r1.get_tbr()->pptr);
    // };

    CHECK_BODY("t1 to t2") {
        //todo ： 发送的时间-最后一个receiver收到的时间
        CXLRef r1 = shm.cxl_malloc_wrc(1024*1024*2, 0);
        void* start = shm.get_start();
        r1.str_content = "aaa";
        uint64_t queue_offset = shm.create_msg_queue(2);
        
        uint64_t obj_offset = r1.data;
        CXLObj* cxl_obj = (CXLObj*)((uintptr_t)start + obj_offset);
        // 起t1，循环等待queue的对象
        std::promise<uint64_t> offset_2;
        std::thread t1(consumer_wrc, queue_offset, std::ref(offset_2));

        while (cxl_obj->reader_count != 0 && cxl_obj->writer_count != 0) {
            
        }
        if (cxl_obj->reader_count == 0 && cxl_obj->writer_count == 0) {
            cxl_obj->writer_count++;
            r1 = shm.cxl_malloc_wrc(1024*1024*2, 0);
            RootRef* tbr1 = r1.tbr;
            tbr1->ref_cnt++;
            r1.str_content = "bbb";
            cxl_obj->writer_count--;
        }
        
        shm.sent_to(queue_offset, r1);
        t1.join();
        auto status = offset_2.get_future().get();

        CXLRef r1_t2 = shm.get_ref(status);
        uint64_t obj_offset_t2 = r1_t2.data;
        CXLObj* cxl_obj_t2 = (CXLObj*)((uintptr_t)start + obj_offset);
        cxl_obj_t2->reader_count--;
        result = (status == r1.get_tbr()->pptr);
    }
    shmctl(shm_id, IPC_RMID, NULL);

    return print_test_summary();
}