[[ $debug = true ]] && set -x
[[ $debug = true ]] && repeat=1 || repeat=10
plot_dir=plot
plot_width=1080
plot_height=720
output_dir=./results/plot
max_steps=${max_steps:-10000}
version=mpi
size=4000
nps=(4 16 64)

source env.sh

set -e

mkdir -p $output_dir
mkdir -p build/logs

echo "computing strong scalability .."

echo > $output_dir/strong_scalability.dat
time_ms_ref=none
for i in ${!nps[@]}
do
    echo "stencil $size ${nps[$i]} .."
    
    perf stat -o build/logs/$version-perf-report.txt -ddd -r $repeat mpirun -np ${nps[$i]} ./build/stencil-$version $size $size $max_steps
    time_ms=$(cat build/logs/$version-perf-report.txt | sed "s/^[ \t]*//" | grep "time elapsed" | cut -d" " -f1 | sed "s/,/\./")

    if [[ $time_ms_ref = none ]]
    then
        time_ms_ref=$time_ms
    fi
    speedup=$(echo "print($time_ms_ref/$time_ms)" | python3)
   echo "\"$size/${nps[$i]}\" $speedup" >> $output_dir/strong_scalability.dat
done

echo > $output_dir/strong_scalability.conf
echo "set terminal png size $plot_width,$plot_height" >> $output_dir/strong_scalability.conf
echo "set output \"$output_dir/strong_scalability.png\"" >> $output_dir/strong_scalability.conf 
echo "set xlabel \"size/np\"" >> $output_dir/strong_scalability.conf
echo "set ylabel \"speedup\"" >> $output_dir/strong_scalability.conf
echo "set boxwidth 1.0" >> $output_dir/strong_scalability.conf
echo "set style fill solid" >> $output_dir/strong_scalability.conf
echo "set yrange [0:]" >> $output_dir/strong_scalability.conf
echo "plot \"$output_dir/strong_scalability.dat\" using 2: xtic(1) with histogram notitle linecolor rgb '#006EB8'" >> $output_dir/strong_scalability.conf

cat $output_dir/strong_scalability.conf | gnuplot

echo "Speedups"
cat $output_dir/strong_scalability.dat
