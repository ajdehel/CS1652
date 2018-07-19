#Stop and Wait
echo "-----STOP AND WAIT-----"
gcc project2_stop_wait.c -o stop_wait
if [[ $? -ne 0 ]]; then
    echo "project2_stop_wait.c did not compile"
else
    ./stop_wait < stop_wait_input.txt > stop_wait_output.txt
fi
echo -e "\n\n\n"
echo "-----GO BACK N-----"
gcc project2_gbn.c -o gbn
if [[ $? -ne 0 ]]; then
    echo "project2_gbn.c did not compile"
else
    ./gbn < gbn_input.txt > gbn_output.txt
fi
echo -e "\n\n\n"
#Go Back N
