javac *.java
if [[ $? -ne 0 ]]; then
    echo Error
else
    echo $1 $2
    java $1 $2
fi
