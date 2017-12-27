# Yet Another Audio Streamer... Why?
There is plenty of audio streaming solutions out there; however, Squeezebox ecosystem (with its community, streaming protocol, devices, software receivers, etc.) remains among most popular solutions used. Despite the fact that it was originated back from 2000, it is still attractive for DIY community due to its openness, multi-room synchronization and streaming quality. Squeezebox streaming protocol does not require resampling, which is a key feature for bit-perfect streaming solution (Airplay, SnapCast, etc. are not bit-perfect as they resample to a predefined sample rate).  
There is one particular area within Squeezebox ecosystem, which could be MUCH better – Logitech Media Server (LMS).

# So what is the problem that SlimStreamer solves?
After years of development, LMS, a central part of Squeezebox based solution, has become a big monolithic Web application.
To be fair, it works nicely (thanks to community); however, it has an essential drawback (one may consider it as a design flaw) – everything has to be managed by LMS. Music collection (local / remote), streaming services, alarms, etc. have to be integrated within LMS through plugins or similar.  
This is where SlimStreamer comes in: it decouples Squeezebox streaming capability from the rest of the functionality around managing music ([Single responsibility principle](https://en.wikipedia.org/wiki/Single_responsibility_principle)).
Any application that outputs audio to a default ALSA device can be used as music source for streaming.
In fact, audio streaming is done transparently by SlimStreamer and [SlimPlexor](https://github.com/gimesketvirtadieni/slimplexor) (an ALSA plugin) behind the scene.
Moreover, SlimStreamer captures PCM stream in a bit-perfect way (without resampling to a predefined sample rate), which allows streaming audio in best possible quality!

# That might be interesting… So how does it work?
![Diagram](flow.jpg)
   **#1** An arbitrary music application (MPD, Mopidy, Spotify client, … whatever) plays music by sending PCM stream to the default ALSA device through ALSA API  
   **#2** ALSA directs this PCM stream to SlimPlexor ALSA plugin, which is defined as the default ALSA device  
   **#3** SlimPlexor ALSA plugin redirects PCM data to a predefined ALSA loopback device (one per sample rate)  
   **#4** SlimStreamer keeps reading from all predefined ALSA loopback devices (sample rate is fixed for each loopback device)  
   **#5** SlimStreamer sends PCM data to connected Squeezebox devices through SlimProto over TCP  

# Hmm, I want to give it a try. Where do I start?
This project is still ‘work in progress’:  
  * Capture-and-deliver PCM data to SlimStreamer in a bit-perfect way is implemented and works
  * TCP server (running on 3483 port) required for serving SlimProto commands is implemented (although only few commands are supported for now)
  * SlimProto handshake works (Squeezebox players, including squeezelite, are able to connect to SlimStreamer)
  * TCP server (running on 9000 port) required for streaming PCM data is being developed
  * HTTP streaming functionality is still missing

The project is getting closer and closer for end-2-end usage, stay tuned ;)

Any feedback and comments are much appreciated!
