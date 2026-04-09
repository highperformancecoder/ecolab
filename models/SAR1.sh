export OMP_NUM_THREADS=4
for mig in {0,1e-5,1e-4,1e-3}; do
    python3 SAR1.py $mig > SAR1Results_$mig.csv &
    sleep 1
done
wait

head -1 SAR1Results_0.csv >SAR1Results.csv
for i in SAR1Results_*.csv; do
    tail -n +2  $i >>SAR1Results.csv
done
