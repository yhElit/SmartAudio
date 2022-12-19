# lib 'python-alsaaudio' installieren
import alsaaudio

mixer = alsaaudio.Mixer()
volume = mixer.getvolume(alsaaudio.PCM_PLAYBACK)
mixer.setvolume(50)