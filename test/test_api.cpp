#include <assert.h>
#include <chrono>
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

# define THREAD2 = 1
//# define THREAD3 = 1
# define BEFORE_START_THREAD = 1

size_t length;
int shm_id;
int counter;

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

void consumer_wrc(uint64_t queue_offset, std::promise<uint64_t> &offset,  std::promise<std::chrono::system_clock::time_point> &t_receiver)
{
    //sleep(1);
    cxl_shm shm = cxl_shm(length, shm_id);
    shm.thread_init();
    void* start = shm.get_start();
    
    auto t_start = std::chrono::high_resolution_clock::now();
    
    
    std::vector<cxl_message_queue_t*> vec_q;
    std::vector<RootRef*> vec_tbr;
    
    for (int i = 0; i < counter; i ++) {
        cxl_message_queue_t* q0 = (cxl_message_queue_t*) get_data_at_addr(start, queue_offset);
        vec_q.push_back(q0);
        POTENTIAL_FAULT
        cxl_thread_local_state_t* tls = (cxl_thread_local_state_t*) get_data_at_addr(start, shm.get_tls_offset());
        if(q0->receiver_id == 0)
        {
            POTENTIAL_FAULT
            q0->receiver_next = tls->receiver_queue;
            POTENTIAL_FAULT
            tls->receiver_queue = queue_offset;
            POTENTIAL_FAULT
            q0->receiver_id = shm.get_thread_id();
        }
        RootRef* tbr0 = shm.thread_base_ref_alloc(tls);
        vec_tbr.push_back(tbr0);
    }

    cxl_message_queue_t* q ;
    RootRef* tbr;
    for (int i = 0; i < counter; i++) {
        q = vec_q[i];
        tbr = vec_tbr[i];
        //std::cout << "consumer_wrc before cxl_unwrap_mend i :" << i << " ,queue_offset:" << queue_offset << std::endl;
        
        CXLRef r1 = shm.cxl_unwrap_mend(queue_offset, q, tbr);
        
        //CXLRef r1 = shm.cxl_unwrap_mend(queue_offset);
        //std::cout << "\n \n consumer_wrc cxl_unwrap_mend after :" << std::endl;
        //std::cout << "consumer_wrc cxl_unwrap_mend after r1.get_tbr():" << r1.get_tbr() << std::endl;
        //std::cout << "consumer_wrc cxl_unwrap_mend after r1.get_tbr()->pptr:" << r1.get_tbr()->pptr << std::endl;
       
        uint64_t obj_offset = r1.data;
        CXLObj* cxl_obj1 = (CXLObj*)get_data_at_addr(start, obj_offset);
        
        
        while (cxl_obj1->writer_count != 0) {
        }
        
  
    }
    t_receiver.set_value(std::chrono::high_resolution_clock::now());
    //std::cout << "consumer_wrc t_receiver_temp sleep  " << " ,&t_receiver:" << &t_receiver  << std::endl;
    
    //CXLRef r1 = shm.cxl_unwrap(queue_offset);
    // std::cout<<"consumer4 r1.get_tbr():" << r1.get_tbr() <<std::endl;
    // std::cout<<"consumer4 r1.get_tbr()->pptr:" << r1.get_tbr()->pptr <<std::endl;
    // offset.set_value(r1.get_tbr()->pptr);

}


long t_duration;

auto calculateTimeEnd(std::chrono::time_point<std::chrono::system_clock> t_start, std::chrono::time_point<std::chrono::system_clock> t_end) -> long {
    t_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(t_end - t_start).count();
    #ifdef THREAD2
    t_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(t_end - t_start - std::chrono::seconds(1)).count();
    #endif
    #ifdef THREAD3
    t_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(t_end - t_start - std::chrono::seconds(2)).count();
    #endif
    return t_duration;
}


auto test_warpper() {

    using namespace std;
    length = (ZU(1) << 28);
    shm_id = shmget(100, length, IPC_CREAT|0664);
    
    shmctl(shm_id, IPC_RMID, NULL);


    length = (ZU(1) << 28);
    shm_id = shmget(100, length, IPC_CREAT|0664);
    cxl_shm shm = cxl_shm(length, shm_id);
    
    //std::cout  << "1" << std::endl;
    shm.thread_init();
    
    //std::cout  << "1" << std::endl;

    // CHECK_BODY("thread init") {
    //     shm.thread_init();
    //     result = (shm.get_thread_id() != 0);
    // }

    // CHECK_BODY("malloc and free") {
    //     CXLRef ref = shm.cxl_malloc(32, 0);
    //     result = (ref.get_tbr() != NULL && ref.get_addr() != NULL);
    // };

    // CHECK_BODY("wrap ref") {
    //     CXLRef ref = shm.cxl_malloc(32, 0);
    //     uint64_t addr = shm.cxl_wrap(ref);
    //     result = (addr != 0);
    // };
    //CHECK_BODY("data transfer") {
        CXLRef r1 = shm.cxl_malloc(100, 0);
        void* start = shm.get_start();
        uint64_t queue_offset1 = shm.create_msg_queue(2);
        std::promise<uint64_t> offset_1;
        std::promise<std::chrono::time_point<std::chrono::system_clock>> t_receiver1;


        #ifdef THREAD2
        uint64_t queue_offset2 = shm.create_msg_queue(4);
        std::promise<uint64_t> offset_2;
        std::promise<std::chrono::time_point<std::chrono::system_clock>> t_receiver2;
        #endif
        #ifdef THREAD3
        uint64_t queue_offset3 = shm.create_msg_queue(6);
        std::promise<uint64_t> offset_3;
        std::promise<std::chrono::time_point<std::chrono::system_clock>> t_receiver3;
        #endif
        std::vector<CXLRef> block_vec;

        #ifdef BEFORE_START_THREAD
            std::cout << "\n \n \n start thread1:" << queue_offset1 << std::endl;
            std::thread t1(consumer_wrc, queue_offset1, std::ref(offset_1), std::ref(t_receiver1));

            auto t_b_s = std::chrono::high_resolution_clock::now();
            //std::cout << "before sleep:" << std::chrono::duration_cast<std::chrono::nanoseconds>(t_b_s - t_start).count() << std::endl;
            
            sleep(1);
            auto t_a_s = std::chrono::high_resolution_clock::now();
            //std::cout << "after sleep:" << std::chrono::duration_cast<std::chrono::nanoseconds>(t_a_s - t_start).count() << std::endl;
            
            
            
            #ifdef THREAD2
            std::cout << "\n \n \n start thread2:" << queue_offset2 << std::endl;
            std::thread t2(consumer_wrc, queue_offset2, std::ref(offset_2), std::ref(t_receiver2));
            #endif
            sleep(1);
            #ifdef THREAD3
            //std::cout << "\n \n \n start thread3:" << queue_offset2 << std::endl;
            std::thread t3(consumer_wrc, queue_offset3, std::ref(offset_3), std::ref(t_receiver3));
            #endif
            //sleep(1);
        #endif

        for (int i = 0; i < counter; i++) { //push
            CXLRef r = shm.cxl_malloc_wrc(DATA_SIZE_BLOCK, 0);
            block_vec.push_back(r);
        }


        auto t_start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < counter; i++) { //send
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

            #ifdef THREAD3
            shm.sent_to(queue_offset3, r1);
            #endif
            // bool send_res1    = shm.sent_to(queue_offset1, r1);
            // #ifdef THREAD2
            // bool send_res2    = shm.sent_to(queue_offset2, r1);
            // #endif
            // #ifdef THREAD3
            // bool send_res3    = shm.sent_to(queue_offset3, r1);
            // #endif
        } //after send
        
        #ifdef BEFORE_START_THREAD
            #else
            //std::cout << "\n \n \n start thread1:" << queue_offset1 << std::endl;
            std::thread t1(consumer_wrc, queue_offset1, std::ref(offset_1), std::ref(t_receiver1));

            auto t_b_s = std::chrono::high_resolution_clock::now();
            //std::cout << "before sleep:" << std::chrono::duration_cast<std::chrono::nanoseconds>(t_b_s - t_start).count() << std::endl;
            
            sleep(1);
            auto t_a_s = std::chrono::high_resolution_clock::now();
            //std::cout << "after sleep:" << std::chrono::duration_cast<std::chrono::nanoseconds>(t_a_s - t_start).count() << std::endl;
            
            
            
            #ifdef THREAD2
            //std::cout << "\n \n \n start thread2:" << queue_offset2 << std::endl;
            std::thread t2(consumer_wrc, queue_offset2, std::ref(offset_2), std::ref(t_receiver2));
            #endif
            sleep(1);
            #ifdef THREAD3
            //std::cout << "\n \n \n start thread3:" << queue_offset2 << std::endl;
            std::thread t3(consumer_wrc, queue_offset3, std::ref(offset_3), std::ref(t_receiver3));
            #endif
            //sleep(1);
        #endif

        //std::cout << "test_warpper thread1 queue_offset1:" << queue_offset1 << std::endl;
        
        //std::thread t1(consumer, queue_offset1, std::ref(offset_1));
        
        //std::thread t2(consumer_wrc, queue_offset2, std::ref(offset_2));

        t1.join();
        auto t_real_receive1 = t_receiver1.get_future().get();
        auto t_end = t_real_receive1;
        
        
        //sleep(1);

        #ifdef THREAD2
        t2.join();
        auto t_real_receive2 = t_receiver2.get_future().get();
        t_end = t_end > t_real_receive2 ? t_end : t_real_receive2;
        #endif

        #ifdef THREAD3
        t3.join();
        auto t_real_receive3 = t_receiver3.get_future().get();
        t_end = t_end > t_real_receive3 ? t_end : t_real_receive3;
        #endif
        //sleep(1);
        
        // std::cout  << "1111: status"  << std::endl;
        //auto status = offset_2.get_future().get();
        //std::cout  << "1111: status" << status <<"r1.get_tbr()->pptr" <<r1.get_tbr()->pptr << std::endl;
        //result = (status == r1.get_tbr()->pptr);
        
    //};

    auto time_total =  calculateTimeEnd(t_start, t_end);
    shmctl(shm_id, IPC_RMID, NULL);
    return time_total;
}


int main(int argc, char *argv[])
{
    
    long result = 0;
    int iter = atoi(argv[1]);
    DATA_SIZE_BLOCK = atoi(argv[2]);
    DATA_SIZE_MESSAGE = atoi(argv[3]);
    counter = DATA_SIZE_MESSAGE / DATA_SIZE_BLOCK;
    //counter = 1;
    for (int i = 0; i < iter; i++) {
        // 这里可能有数据溢出的风险。
        result += test_warpper();
    }

    /* std::cout << "+++++++++++++++++++++++++++++" << std::endl;
    std::cout << "DATA_SIZE_BLOCK: " << DATA_SIZE_BLOCK << std::endl;
    std::cout << "DATA_SIZE_MESSAGE: " << DATA_SIZE_MESSAGE << std::endl;
    std::cout << "average t_duration: " << result / (1.0 * iter) << std::endl;
    std::cout << "+++++++++++++++++++++++++++++" << std::endl;
    std::cout << std::endl; */
    
    std::cout << "Total: " << result / (1.0 * iter) << std::endl;

    return print_test_summary();
}