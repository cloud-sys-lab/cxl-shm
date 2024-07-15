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

# define DATA_SIZE_BLOCK 128
# define DATA_SIZE_MESSAGE 600

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

void consumer_wrc(uint64_t queue_offset, std::promise<uint64_t> &offset, std::promise<uint64_t> &t_receiver)
{
    // sleep(3);
    cxl_shm shm = cxl_shm(length, shm_id);
    shm.thread_init();
    void* start = shm.get_start();
    for (int i = DATA_SIZE_BLOCK; i <= DATA_SIZE_BLOCK; i+=DATA_SIZE_BLOCK) {
        
        CXLRef r1 = shm.cxl_unwrap_wrc(queue_offset);
        // auto t_receiver_temp = static_cast<uint64_t>(time(NULL));
        uint64_t obj_offset = r1.data;
        CXLObj* cxl_obj1 = (CXLObj*)get_data_at_addr(start, obj_offset);
        while (cxl_obj1->writer_count != 0) {
        }
        // offset.set_value(r1.get_tbr()->pptr);
//        t_receiver.set_value(t_receiver_temp);
    }
    auto t_receiver_temp = static_cast<uint64_t>(time(NULL));
    t_receiver.set_value(t_receiver_temp);
}
std::vector<size_t> test_sizes = {
        8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536,
        131072, 262144, 524288, 1048576, 2097152, 4194304, 8388608, 16777216,
        33554432, 67108864, 134217728};

int main()
{
    length = (ZU(1) << 28);
    shm_id = shmget(100, length, IPC_CREAT|0664);
    shmctl(shm_id, IPC_RMID, NULL);

    using namespace std;
    length = (ZU(1) << 28);
    shm_id = shmget(100, length, IPC_CREAT|0664);
    cxl_shm shm = cxl_shm(length, shm_id);
    shm.thread_init();

    CHECK_BODY("thread init") {
        shm.thread_init();
        result = (shm.get_thread_id() != 0);
    }
    
    // CHECK_BODY("malloc and free") {
    //     CXLRef ref = shm.cxl_malloc(32, 0);
    //     result = (ref.get_tbr() != NULL && ref.get_addr() != NULL);
    //     shm.cxl_free(true, (cxl_block*)(&ref));
    // };
    // CHECK_BODY("wrap ref") {
    //     CXLRef ref = shm.cxl_malloc(32, 0);
    //     uint64_t addr = shm.cxl_wrap(ref);
    //     result = (addr != 0);
    // };

    // CHECK_BODY("data transfer") {
    //     std::cout << "data transfer 1" << std::endl;
    //     CXLRef r1 = shm.cxl_malloc(1008, 0);
    //     std::cout << "data transfer 2" << std::endl;
    //     uint64_t queue_offset = shm.create_msg_queue(2);
    //     std::cout << "data transfer 3" << std::endl;
    //     shm.sent_to(queue_offset, r1);
        
    //     std::cout << "data transfer 4" << std::endl;
    //     std::promise<uint64_t> offset_2;
    //     std::promise<uint64_t> t_receiver;
        
    //     std::cout << "data transfer 5" << std::endl;
    //     std::thread t1(consumer_wrc, queue_offset, std::ref(offset_2), std::ref(t_receiver));
        
    //     std::cout << "data transfer 6" << std::endl;
    //     t1.join();
        
    //     std::cout << "data transfer 7" << std::endl;
    //     auto status = offset_2.get_future().get();
        
    //     std::cout << "data transfer 8" << "status"<<status<<"r1.get_tbr()" <<r1.get_tbr() << std::endl;
    //     result = (status == r1.get_tbr()->pptr);
    // };

    CHECK_BODY("t1 to t2") {
        void* start = shm.get_start();
        //std::cout << "t1 to t2 1111" << std::endl;
        uint64_t queue_offset1 = shm.create_msg_queue(2);
        uint64_t queue_offset2 = shm.create_msg_queue(3);
        //uint64_t queue_offset3 = shm.create_msg_queue(4);
        

        //std::cout << "t1 to t2 11111" << std::endl;
        
        //std::cout << "t1 to t2 2" << std::endl;
        // 起t1，循环等待queue的对象
        std::promise<uint64_t> offset_1;
        std::promise<uint64_t> offset_2;
        //std::promise<uint64_t> offset_3;
        
        std::promise<uint64_t> t_receiver1;
        std::promise<uint64_t> t_receiver2;
        //std::promise<uint64_t> t_receiver3;
        std::thread t1(consumer_wrc, queue_offset1, std::ref(offset_1), std::ref(t_receiver1));
        std::thread t2(consumer_wrc, queue_offset2, std::ref(offset_2), std::ref(t_receiver2));
        //std::thread t3(consumer_wrc, queue_offset3, std::ref(offset_3), std::ref(t_receiver3));
         //std::cout << "t1 to t2 4" << std::endl;
            auto t_send = static_cast<uint64_t>(time(NULL));
            
         for (int i = DATA_SIZE_BLOCK; i <= DATA_SIZE_MESSAGE; i+=DATA_SIZE_BLOCK) {


            std::cout << "t1 to t2 i:" << i << std::endl;
            //todo ： 发送的时间-最后一个receiver收到的时间
            
            CXLRef r1 = shm.cxl_malloc_wrc(DATA_SIZE_BLOCK, 0);
            
            //std::cout << "t1 to t2 111" << std::endl;
            r1.str_content = "aaa";
            std::cout << "t1 to t2 11 , r1.get_tbr()->pptr" << r1.get_tbr()->pptr << std::endl;
            
            //std::cout << "t1 to t2 2" << std::endl;
            uint64_t obj_offset = r1.data;
            CXLObj* cxl_obj = (CXLObj*)get_data_at_addr(start, obj_offset);
            
            // uint64_t obj_offset = r1.data;
            // CXLObj* cxl_obj = (CXLObj*)get_data_at_addr(start, obj_offset);
            //std::cout << "t1 to t2 3" << std::endl;
            while (cxl_obj->reader_count != 0 && cxl_obj->writer_count != 0) {
                
            }
            std::cout << "before change , r1.get_tbr()->pptr" << r1.get_tbr()->pptr << std::endl;
            if (cxl_obj->reader_count == 0 && cxl_obj->writer_count == 0) {
                cxl_obj->writer_count++;
                std::cout << "change 1: , r1.get_tbr()->pptr:" << r1.get_tbr()->pptr << std::endl;
                //r1 = shm.cxl_malloc_wrc(100, 0);
                //std::cout << "change 2: , r1.get_tbr()->pptr:" << r1.get_tbr()->pptr << std::endl;
                RootRef* tbr1 = (RootRef*) get_data_at_addr(start, r1.tbr);
                
                std::cout << "change 3: , r1.get_tbr()->pptr:" << r1.get_tbr()->pptr << std::endl;
                tbr1->ref_cnt++;
    
                r1.str_content = "bbb";
                cxl_obj->str_content = "bbb";
                    
                cxl_obj->writer_count--;
            }
            std::cout << "after change , r1.get_tbr()->pptr" << r1.get_tbr()->pptr << std::endl;

            
            std::cout << "t1 to t2 4.0,"<< t_send << "r1.get_tbr()->pptr" << r1.get_tbr()->pptr << std::endl;
            bool send_res1    = shm.sent_to(queue_offset1, r1);
            bool send_res2    = shm.sent_to(queue_offset2, r1);
            //bool send_res3    = shm.sent_to(queue_offset3, r1);

            std::cout << "t1 to t2 4.01 sendRes:" << (send_res1 ? "true" : "false") << std::endl;
            //std::cout << "t1 to t2 4.01 sendRes:" << (send_res2 ? "true" : "false") << std::endl;
        }
        //std::cout << "t1 to t2 4.01 sendRes:" << (send_res3 ? "true" : "false") << std::endl;
        
        //std::cout << "t1 to t2 4.02" << std::endl;
        t1.join();
        t2.join();
        //t3.join();
        
        auto t_receiver_final = static_cast<uint64_t>(time(NULL));
        //std::cout << "t1 to t2 4.1" << std::endl;
        // auto t_receive_2 = time(NULL);
        // auto status1 = offset_1.get_future().get();
        // auto status2 = offset_2.get_future().get();
        // auto status3 = offset_3.get_future().get();
        
        //std::cout << "t1 to t2 4.2" << std::endl;
        auto t_real_receive1 = t_receiver1.get_future().get();
        //auto t_real_receive2 = t_receiver2.get_future().get();
        //auto t_real_receive3 = t_receiver3.get_future().get();

        
        //std::cout << "t1 to t2 4.3" << std::endl;
        //t_receiver_final = t_real_receive1 > t_real_receive2 ? t_real_receive1 : t_real_receive2;
        //t_receiver_final = t_receiver_final > t_real_receive3 ? t_receiver_final : t_real_receive3;
        
        auto t_all = t_receiver_final - t_send;

        //取最大的时间
        //if () 
        //auto t_all_2 = t_receive_2 - t_send;
        
        std::cout << "t_all :" << t_all  <<  std::endl; //",t_real_receive" << t_real_receive1 << ",t_send" << t_send  << ",t_receive_2" << t_receive_2 << "，status" << status1 <<"，r1.get_tbr()" << r1.get_tbr() << std::endl;
        // std::cout << "t_all_2 :" << t_all_2 << std::endl;

        //std::cout << "t1 to t2 5" << std::endl;
        // CXLRef* r1_t2_1 = (CXLRef*)get_data_at_addr(start, status1);
        // CXLRef* r1_t2_2 = (CXLRef*)get_data_at_addr(start, status2);
        // CXLRef* r1_t2_3 = (CXLRef*)get_data_at_addr(start, status3);

        // uint64_t obj_offset_t2_1 = r1_t2_1->data;
        // uint64_t obj_offset_t2_2 = r1_t2_2->data;
        // uint64_t obj_offset_t2_3 = r1_t2_3->data;
        // CXLObj* cxl_obj_t2 = (CXLObj*)get_data_at_addr(start, obj_offset);
        // std::cout << "cxl_obj_t2_content :" << cxl_obj_t2->str_content << std::endl; 
        // cxl_obj_t2->reader_count--;
        // result = (status2 == r1.get_tbr()->pptr);
        //std::cout << "t1 to t2 6" << std::endl;
        shmctl(shm_id, IPC_RMID, NULL);
    };

    std::cout << "print_test_summary" << std::endl;
    return print_test_summary();
}