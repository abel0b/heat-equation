[[ debug = "true" ]] && set -x

size_x=${size_x:-256}
size_y=${size_y:-256}
max_steps=${max_steps:-10000}
np=${np:-4}

reset="\e[0m"
bold="\e[1mB"
green="\e[32m"
red="\e[31m"

echo "testing all stencil versions with parameters"
echo "size_x=$size_x"
echo "size_y=$size_y"
echo "max_steps=$max_steps"

versions="baseline openmp mpi"

mkdir -p build/logs

source env.sh

function test() {
    pass=true

    for version in $@
    do
        if [[ $version =~ mpi ]]
        then
            mpirun -np 4 ./build/stencil-$version $size_x $size_y $max_steps > ./build/logs/stencil-$version.log
        else
            ./build/stencil-$version $size_x $size_y $max_steps > ./build/logs/stencil-$version.log
        fi

        if [[ ! "$?" = "0" ]]
        then
            echo "./build/stencil-$version did not exit successfully" 
            exit 1
        fi
    done
    shift

    for version in $@
    do
        ./build/stencil-compare build/stencil-baseline.bin build/stencil-$version.bin
        if [[ "$?" = "0" ]]
        then
            echo -e "$version .. ${green}OK$reset"
        else
            echo -e "$version .. ${red}NOK$reset"
            pass=false
        fi
    done

    if [[ $pass = "true" ]]
    then
        echo -e "-> ${green}all tests passed successfully$reset"
    else
        echo -e "-> ${red}tests failed$reset"
        exit 1
    fi
}

test $versions

