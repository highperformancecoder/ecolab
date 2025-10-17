i=0
while [ $[i++] -lt 8 ]; do
    python3 SAR.py $i > SARResults_$i.csv &
    sleep 1
done
wait

head -1 SARResults_1.csv >SARResults.csv
i=0
while [ $[i++] -lt 8 ]; do
    tail -n +2  SARResults_$i.csv >>SARResults.csv
done
