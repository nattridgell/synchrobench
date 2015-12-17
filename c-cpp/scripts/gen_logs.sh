DIR=".."
OUTPUT=${DIR}/output
BIN=${DIR}/bin

SYNC="LOCKFREE"
THREADS="1 2 4 8 16 24 32"
SIZE="4096 65536"
UPDATES="0 20 100"
ITER=10


if [ ! -d "${OUTPUT}" ]; then
  mkdir ${OUTPUT}
else
  rm -rf ${OUTPUT}/*
fi

mkdir ${OUTPUT}/log ${OUTPUT}/data ${OUTPUT}/analysis

###############################
# records all benchmark outputs
###############################

# skiplist benchmarks
BENCHES="versioned-skiplist lockfree-fraser-skiplist MUTEX-skiplist"
for i in ${SIZE}; do
  r=`expr $i \* 2`
  for bench in ${BENCHES}; do
    for t in ${THREADS}; do
      for u in ${UPDATES}; do
        OUT=${OUTPUT}/log/${bench}-u${u}-t${t}-i${i}.log
        echo "Writing to ${OUT}"
        for (( j=1; j<=${ITER}; j++ )); do
          ${BIN}/${bench} -u ${u} -t ${t} -i ${i} -r ${r} >> ${OUT}
        done
      done
    done
  done
done
