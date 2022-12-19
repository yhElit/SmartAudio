echo "Checking requirements for Smart-Audio Master"

echo
echo "Mosquitto:"
if [ $(dpkg-query -W -f='${Status}' mosquitto 2>/dev/null | grep -c "ok installed") -eq 0 ];
then
echo "Installing Mosquitto"
sudo -s apt-get install -y Mosquitto
else
echo "Mosquitto already installed"
fi

echo
echo "Python-Pip:"
if [ $(dpkg-query -W -f='${Status}' python-pip 2>/dev/null | grep -c "ok installed") -eq 0 ];
then
echo "Installing python-pip"
sudo -s apt-get install -y python-pip
else
echo "Python-Pip already installed"
fi

echo
echo "Paho-MQTT:"
if [ $(pip list | grep -c "paho-mqtt") -eq 0 ];
then
echo "Installing Paho-MQTT"
sudo -s pip install paho-mqtt
else
echo "Paho-MQTT already installed"
fi

echo
echo "Configparser 4.0.2:"
if [ $(pip list | grep -c "configparser") -eq 0 ];
then
echo "Installing Configparser"
sudo -s pip install 'configparser==4.0.2'
else
echo "Configparser already installed"
fi
