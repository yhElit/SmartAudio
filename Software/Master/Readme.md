# Sensoren über topic unterscheiden

In dieser Variante bekommt jeder Sensor seinen eigenen channel und ist identifizierbar über den Topic. Nun müssen Master oder Client einfach nur die Commands im jeweiligen Topic (bspw. smartaudio/Raum/Sensor1) publishen (Clients: In/Out, Master: VOn/VOff). In on_message wird dann unterschieden welcher command gerade geschrieben wurde.
Da hier nur ein kleiner Befehlssatz existiert hält sich der Aufwand in Grenzen und keine clientid muss mit in den Payload geschrieben werden, weswegen im Gegensatz zur anderen Version JSON entfallen kann.
