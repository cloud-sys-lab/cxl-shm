for i in 1 2 4 8 16 24 32 48 64
do
	sleep 1
	./cxlmalloc-benchmark-tatp_tbb $i  >> tatp-tbb.txt
done

grep "MOPS" tatp-tbb.txt