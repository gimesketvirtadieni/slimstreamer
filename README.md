# Motivation
There is plenty of audio streaming solutions out there. However, Squeezebox ecosystem (with its community, streaming protocol, devices, software receivers, etc.) remains among most used solutions. Despite the fact that it was originated back from 2000, it is still attractive for DIY community due to its openness, multi-room synchronization and streaming quality. Squeezebox streaming protocol does not require resampling, which is a key feature for bit-perfect streaming solution (Airplay, SnapCast, etc. are not bit-perfect as they resample to a predefined sample rate). There is just one particular area, which might be MUCH better – Logitech Media Server.

# So what is the problem that SlimStreamer solves?
After years of development, Logitech Media Server (LMS), a central part of Squeezebox based solution, has become a giant monolithic Web application. To be fair, it works nicely. However, there is an essential drawback (which I consider a design flaw) with LMS: you have to manage everything from it. Music collection (local / remote), streaming services, alarms, etc. have to be integrated within LMS through plugins or similar.
This is where SlimStreamer comes into the picture: it decouples Squeezebox streaming from the rest of functionality any music apps provides. It works like this:
Music app (MPD, Mopidy, Spotify client, whatever) plays music by sending PCM stream via ALSA interface
ALSA directs this PCM stream to SlimPlexor ALSA plugin
SlimPlexor ALSA plugin redirects to ALSA Loopback device preconfigured 
