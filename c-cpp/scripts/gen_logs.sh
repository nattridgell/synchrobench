#! /bin/bash

DIR=".."
OUTPUT=${DIR}/output
BIN=${DIR}/bin

SYNC="LOCKFREE"
THREADS="1 2 4"
SIZE=256
UPDATES="10 25"
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

# lockfree benchmarks
BENCHES="versioned-skiplist"
for bench in ${BENCHES}; do
  for t in ${THREADS}; do
    for u in ${UPDATES}; do
      OUT=${OUTPUT}/log/${bench}-u${u}-t${t}.log
      echo "Starting to write to ${OUT}"
      for (( j=1; j<=${ITER}; j++ )); do
        ${BIN}/${bench} -u ${u} -t ${t} -i ${SIZE} >> ${OUT}
      done
    done
  done
done
