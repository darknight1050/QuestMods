adb shell ip addr show wlan0 | findstr 24
set /P ip=Enter ip: 
adb tcpip 5555
adb connect %ip%:5555
 