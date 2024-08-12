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
int counter = 1;

int DATA_SIZE_BLOCK = 128;
int DATA_SIZE_MESSAGE = 128;

void consumer(uint64_t queue_offset, std::promise<uint64_t> &offset)
{
    //sleep(1);
    std::cout<<"consumer0"<<std::endl;
    cxl_shm shm = cxl_shm(length, shm_id);
    std::cout<<"consumer1"<<std::endl;
    shm.thread_init();
    std::cout<<"consumer2"<<std::endl;
    void* start = shm.get_start();
    std::cout<<"consumer3 queue_offset: " << queue_offset <<std::endl;
    CXLRef r1 = shm.cxl_unwrap(queue_offset);
    std::cout<<"consumer4 r1.get_tbr():" << r1.get_tbr() <<std::endl;
    std::cout<<"consumer4 r1.get_tbr()->pptr:" << r1.get_tbr()->pptr <<std::endl;
    offset.set_value(r1.get_tbr()->pptr);
    std::cout<<"consumer5"<<std::endl;
}

void consumer_wrc(uint64_t queue_offset, std::promise<uint64_t> &offset, std::promise<std::chrono::time_point<std::chrono::system_clock> > &t_receiver)
{
    //sleep(1);
    cxl_shm shm = cxl_shm(length, shm_id);
    shm.thread_init();
    void* start = shm.get_start();
    
    auto t_start = std::chrono::high_resolution_clock::now();
    
    RootRef* tbr;
    for (int i = 0; i < counter; i++) {
        std::cout << "cxl_unwrap_wrc before i :" << 1 << " ,queue_offset:" << queue_offset << std::endl;
        
        //CXLRef r1 = shm.cxl_unwrap_wrc(queue_offset, q, tls, tbr);
        
        CXLRef r1 = shm.cxl_unwrap(queue_offset);
        std::cout << "cxl_unwrap_wrc after :" << std::endl;
        std::cout << "cxl_unwrap_wrc after r1.get_tbr():" << r1.get_tbr() << std::endl;
        std::cout << "cxl_unwrap_wrc after r1.get_tbr()->pptr:" << r1.get_tbr()->pptr << std::endl;
       
        uint64_t obj_offset = r1.data;
        CXLObj* cxl_obj1 = (CXLObj*)get_data_at_addr(start, obj_offset);
        
        
        while (cxl_obj1->writer_count != 0) {
        }
        
        auto t_receiver_temp = std::chrono::high_resolution_clock::now();
        t_receiver.set_value(t_receiver_temp);
    }
    //CXLRef r1 = shm.cxl_unwrap(queue_offset);
    // std::cout<<"consumer4 r1.get_tbr():" << r1.get_tbr() <<std::endl;
    // std::cout<<"consumer4 r1.get_tbr()->pptr:" << r1.get_tbr()->pptr <<std::endl;
    // offset.set_value(r1.get_tbr()->pptr);

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
        void* start = shm.get_start();
        uint64_t queue_offset1 = shm.create_msg_queue(2);
        
        #ifdef THREAD2
        uint64_t queue_offset2 = shm.create_msg_queue(4);
        #endif
        std::vector<CXLRef> block_vec;
        for (int i = 0; i < counter; i++) {
            CXLRef r = shm.cxl_malloc_wrc(DATA_SIZE_BLOCK, 0);
            block_vec.push_back(r);
        }

        for (int i = 0; i < counter; i++) {
            CXLRef r1 = block_vec[i];
            uint64_t obj_offset = r1.data;
            CXLObj* cxl_obj = (CXLObj*)get_data_at_addr(start, obj_offset);
        
            while (cxl_obj->reader_count != 0 && cxl_obj->writer_count != 0) {
                
            }
            if (cxl_obj->reader_count == 0 && cxl_obj->writer_count == 0) {
                cxl_obj->writer_count++;
                RootRef* tbr1 = (RootRef*) get_data_at_addr(start, r1.tbr);
                tbr1->ref_cnt++;
                cxl_obj->str_content = "bbb";
                cxl_obj->writer_count--;
            }


            shm.sent_to(queue_offset1, r1);
            // shm.sent_to(queue_offset1, r1);
            #ifdef THREAD2
            // shm.sent_to(queue_offset2, r1);
            shm.sent_to(queue_offset2, r1);
            #endif
            // bool send_res1    = shm.sent_to(queue_offset1, r1);
            // #ifdef THREAD2
            // bool send_res2    = shm.sent_to(queue_offset2, r1);
            // #endif
            // #ifdef THREAD3
            // bool send_res3    = shm.sent_to(queue_offset3, r1);
            // #endif
        }



        std::promise<uint64_t> offset_1;
        std::promise<std::chrono::time_point<std::chrono::system_clock>> t_receiver1;
        std::thread t1(consumer_wrc, queue_offset1, std::ref(offset_1), std::ref(t_receiver1));
        sleep(1);
        #ifdef THREAD2

        std::promise<uint64_t> offset_2;
        std::promise<std::chrono::time_point<std::chrono::system_clock>> t_receiver2;
        std::thread t2(consumer_wrc, queue_offset2, std::ref(offset_2), std::ref(t_receiver2));
        #endif
        //std::cout << "test_warpper thread1 queue_offset1:" << queue_offset1 << std::endl;
        
        //std::thread t1(consumer, queue_offset1, std::ref(offset_1));
        
        //std::thread t2(consumer_wrc, queue_offset2, std::ref(offset_2));

        t1.join();
        auto t_real_receive1 = t_receiver1.get_future().get();
        auto t_end = t_real_receive1;
        sleep(1);
        #ifdef THREAD2
        t2.join();
        auto t_real_receive2 = t_receiver2.get_future().get();
        t_end = t_end > t_real_receive2 ? t_end : t_real_receive2;
        #endif
        // sleep(1);
        
        std::cout  << "1111: status"  << std::endl;
        //auto status = offset_2.get_future().get();
        //std::cout  << "1111: status" << status <<"r1.get_tbr()->pptr" <<r1.get_tbr()->pptr << std::endl;
        //result = (status == r1.get_tbr()->pptr);
    };

    shmctl(shm_id, IPC_RMID, NULL);

    return print_test_summary();
}