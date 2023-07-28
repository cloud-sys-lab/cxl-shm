set -x
sh ./script/run_shbench.sh
sh ./script/run_threadtest.sh
sh script/run_rpc.sh
sh script/run-pr.sh
sh script/run-wc.sh
sh script/run_kv.sh
sh script/run_kv_ratio.sh
sh script/run_kv_zipf.sh
sh script/run-smallbank.sh
sh script/run-tatp.sh
sh script/run-smallbank-tbb.sh
sh script/run-tatp-tbb.sh
sh script/run-graph.sh