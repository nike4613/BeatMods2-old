#!/bin/bash

for i in {0..99}
do
  curl -s "http://localhost:7000/[0-99]?json=%7B%22hi%22%3A%22yes%3F%22%2C%22you%22%3A%5B%22know%22%2C%22i%22%2C%22like%22%2C%22the%22%5D%2C%22number%22%3A1.7%7D" &
done

wait
