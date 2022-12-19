#Volume einstellen -> pulseaudio
from pulsectl import Pulse, PulseVolumeInfo

with Pulse('volume-example') as pulse:
    #hier müsste man den BT-speaker identifizieren
    sink_input = pulse.sink_input_list()[0]
  

    volume = sink_input.volume
    print(volume.values)
    print(volume.value_flat)
    
    #Volume einstellbar von 0..1.0
    volume.value_flat = 1.0

    #neues volume setzen
    pulse.volume_set(sink_input, volume)
