while true; do
    timeout 0.2 arecord -f cd -t wav -r 16000 -c 1 2>/dev/null > silent_check.wav
    sox silent_check.wav -n stat 2>&1  | grep "Maximum amplitude" | awk '{print $3}'
    sleep 0.2
done
