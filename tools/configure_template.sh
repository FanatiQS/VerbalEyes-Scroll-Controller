# Just remove the configuration items you do not want to update
while read value; do printf "$value\n" >> $(ls /dev/cu.usbserial-* | head -1); done <<< '\
ssid=
ssidkey=
host=
port=80
path=/
proj=
projkey=
speedmin=-10
speedmax=10
deadzone=0
callow=5
calhigh=1024
sensitivity=4
'
