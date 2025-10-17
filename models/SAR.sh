i=0
while [ $[i++] -lt 8 ]; do
    python3 SAR.py > SARResults_$i.csv &
    sleep 1
done
wait
