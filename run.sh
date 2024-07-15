#!/bin/bash
rm result.log
# echo "test single thread consumer without thread" >> result.log

DATA_SIZE_MESSAGE=("128" "160" "192" "224" "256" "288" "320" "512" "1024" "2048" "4096")

for size in "${DATA_SIZE_MESSAGE[@]}"
do
    echo -n $size >> result.log
    # 参数说明：第一个参数是运行次数，最大值为1024， 第二个参数是：DATA_SIZE_BLOCK, 第三个参数是：DATA_SIZE_MESSAGE
    ./build/cxlmalloc-test-api 5 128 $size >> result.log
done

# message size 可以小于 block size吗？

DATA_SIZE_BLOCK=("32" "64" "128" "256" "512" "1024")
for size_block in "${DATA_SIZE_BLOCK[@]}"
do
    for 
    echo -n $size >> result.log
    ./build/cxlmalloc-test-api 5 $size 1024 >> result.log
done