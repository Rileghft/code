TIMEFORMAT="(%U + %S) * 1000"
(time ./$1 $2) 2> cpu_time.txt
cputime=$(bc < cpu_time.txt)
rm cpu_time.txt
echo "Total: $cputime ms"
