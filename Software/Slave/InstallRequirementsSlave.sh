echo "Checking requirements for Smart-Audio Slave"

echo
echo "Updating..."
sudo apt-get update
echo "done"

echo
echo "Snapclient:"
if [ $(dpkg-query -W -f='${Status}' snapclient 2>/dev/null | grep -c "ok installed") -eq 0 ];
then
echo "Installing snapclient"
sudo -s apt-get install -y snapclient
else
echo "Snapclient already installed"
fi

echo
echo "Libmosquitto:"
if [ $(dpkg-query -W -f='${Status}' libmosquitto-dev 2>/dev/null | grep -c "ok installed") -eq 0 ];
then
echo "Installing libmosquitto"
sudo -s apt-get install -y libmosquitto-dev
else
echo "Libmosquitto already installed"
fi
