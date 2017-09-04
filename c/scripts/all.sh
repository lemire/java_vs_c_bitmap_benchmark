#!/bin/bash
######################
# To add a technique, simply append the file name of your executable to the commands array below
#######################
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
declare -a commands=( 'roaring_benchmarks'   'ewah32_benchmarks'   );
for f in  census-income census-income_srt census1881  census1881_srt  weather_sept_85  weather_sept_85_srt wikileaks-noquotes  wikileaks-noquotes_srt ; do
  echo "# processing file " $f
  for t in "${commands[@]}"; do
     echo "#" $t
    ./$t CRoaring/benchmarks/realdata/$f;
  done
  echo
  echo
done
