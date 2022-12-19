if [ -f /boot/newhost ]
then
  #check if file is empty
  if [ -s /boot/newhost ]
  then
    OLDHOST=`cat /etc/hostname`
    NEWHOST=`cat /boot/newhost`
    echo "Changing $OLDHOST to $NEWHOST..."
    echo $NEWHOST > /etc/hostname
    hostnamectl set-hostname $NEWHOST
    rm /boot/newhost
    shutdown -r now
  else
    rm /boot/newhost
  fi
fi
