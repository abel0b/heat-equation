[[ $debug = true ]] && set -x

versions="baseline openmp mpi"
[[ $debug = true ]] && repeat=1 || repeat=10
plot_dir=plot
plot_width=1080
plot_height=720
output_dir=./build/plot
size_x=${size_x:-1024}
size_y=${size_y:-1024}
max_steps=${max_steps:-10000}
np=${np:-16}

set -e

mkdir -p $output_dir
mkdir -p build/logs

echo "performing benchmarks .."

function plot() {
    echo > $output_dir/speedups.dat
    time_ms_ref=none
    for version in $versions
    do
        echo "$version .."
        
        if [[ $version =~ mpi ]]
        then
             OMP_NUM_THREADS=3 perf stat -o build/logs/$version-perf-report.txt -ddd -r $repeat mpirun -np $np ./build/stencil-$version $size_x $size_y $max_steps
        else
             OMP_NUM_THREADS=$np perf stat -o build/logs/$version-perf-report.txt -ddd -r $repeat ./build/stencil-$version $size_x $size_y $max_steps
        fi
        time_ms=$(cat build/logs/$version-perf-report.txt | sed "s/^[ \t]*//" | grep "time elapsed" | cut -d" " -f1 | sed "s/,/\./")

        if [[ $time_ms_ref = none ]]
        then
            time_ms_ref=$time_ms
        fi
        speedup=$(echo "print($time_ms_ref/$time_ms)" | python3)
       echo "\"$version\" $speedup" >> $output_dir/speedups.dat
    done

    echo > $output_dir/speedups.conf
    echo "set terminal png size $plot_width,$plot_height" >> $output_dir/speedups.conf
    echo "set output \"$output_dir/speedups.png\"" >> $output_dir/speedups.conf 
    echo "set xlabel \"version\"" >> $output_dir/speedups.conf
    echo "set ylabel \"speedup\"" >> $output_dir/speedups.conf
    echo "set boxwidth 0.5" >> $output_dir/speedups.conf
    echo "set style fill solid" >> $output_dir/speedups.conf
    echo "plot \"$output_dir/speedups.dat\" using 2: xtic(1) with histogram notitle" >> $output_dir/speedups.conf
    
    cat $output_dir/speedups.conf | gnuplot

    echo "Speedups"
    cat $output_dir/speedups.dat
}

plot
