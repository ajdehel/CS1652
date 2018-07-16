#Stop and Wait
echo "-----STOP AND WAIT-----"
gcc -w project2_stop_wait.c -o stop_wait
if [[ $? -ne 0 ]]; then
    exit
else
    ./stop_wait < input.txt
fi
echo -e "\n\n\n"
echo "-----GO BACK N-----"
#Go Back N
gcc project2_gbn.c -o gbn
if [[ $? -ne 0 ]]; then
    exit
else
    ./gbn < input.txt
fi

