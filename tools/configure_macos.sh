echo "Select config item to update"
echo "Navigate with up and down arrow keys, select with enter"
echo "[ ] SSID"
echo "[ ] SSID passphrase"
echo "[ ] Host"
echo "[ ] Port"
echo "[ ] Path"
echo "[ ] Proj"
echo "[ ] Project key"
echo "[ ] Speed min"
echo "[ ] Speed max"
echo "[ ] Deadzone"
echo "[ ] Calibrate Low"
echo "[ ] Calibrate High"
echo "[ ] Sensitivity"



index=0
while true
do
	read -r -sn1 arrowKey
	printf "\e[${index}F\e[1C \e[${index}E"
	case $arrowKey in
		A)
			if [ $index -ne 13 ]
			then
				((index=index+1))
			fi
			;;
		B)
			if [ $index -ne 1 ]
			then
				((index=index-1))
			fi
			;;
		"") break ;;
		*) continue ;;
	esac
	printf "\e[${index}F\e[1C*\e[${index}E"
done

printf "\e[${index}F\e[1C*\e[${index}E"
printf "Enter the value: "
read value

case $index in
	13) key="ssid" ;;
	12) key="ssidkey" ;;
	11) key="host" ;;
	10) key="port" ;;
	9) key="path" ;;
	8) key="proj" ;;
	7) key="projkey" ;;
	6) key="speedmin" ;;
	5) key="speedmax" ;;
	4) key="deadzone" ;;
	3) key="callow" ;;
	2) key="calhigh" ;;
	1) key="sensitivity" ;;
esac

echo -e "$key=$value\n" > `ls /dev/cu.usbserial-* | head -1`
