#!/bin/bash

for i in {0..99}
do
  curl -s "http://localhost:7000/[0-99]" &
done

wait
