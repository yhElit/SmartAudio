# Kompilieren der Software mitsamt aller libraries

Siehe build.sh:

Einleiten eines bash terminals
~~~
#!/bin/bash
~~~
Entfernen aller bisherigen objectfiles:
~~~
rm -f ./*.o
~~~

Kompilieren des Ini-Parsers
~~~
gcc -c ini.c -o ini.o
~~~

Kompilieren der Communicationlibrary
~~~
gcc -c communication.c -o comm.o
~~~

Kompilieren der Detektionslibrary
~~~
gcc -D_GNU_SOURCE -c DetectionOfHumanFlowDirection.c -o dohfd.o
~~~

Linken aller Komponenten
~~~
gcc -o smartaudioclient ini.o dohfd.o comm.o -lwiringPi -lmosquitto -lpthread
~~~

# Einrichten des Autostarts für die smart-audio software
Auf Raspbian Buster die Datei "Autostart" unter /etc/xdg/lxsessions/LXDE-Pi editieren
und folgenden Eintrag einfügen:
- sh /home/pi/startsmartaudio.sh

# Ändern des hostnamens on demand

Der Hostname kann wahlweise mit dem tool raspi-config, alternativ direkt über die SD-Karte geändert werden.

Hierzu kopiert man die Datei hostname.sh ins Verzeichnis /home/pi/ und modifiziert die Datei "autostart" mit einer zusätzlichen Zeile:

~~~
@sudo -s sh /home/pi/hostname.sh
~~~

Um den Hostnamen zu ändern, fügt man eine Datei "newhost" ins Bootverzeichnis der SD-Karte mit dem neuen Hostnamen.
