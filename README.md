# Yet Another Audio Streamer... Why?
There is plenty of audio streaming solutions out there; however, Squeezebox ecosystem (with its community, streaming protocol, devices, software receivers, etc.) remains among most popular solutions used. Despite the fact that it was originated back from 2000, it is still attractive for DIY community due to its openness, multi-room synchronization and streaming quality. Squeezebox streaming protocol does not require resampling, which is a key feature for bit-perfect streaming solution (Airplay, SnapCast, etc. are not bit-perfect as they resample to a predefined sample rate).  
There is one particular area within Squeezebox ecosystem, which could be MUCH better – Logitech Media Server (LMS).

# So what is the problem that SlimStreamer solves?
After years of development, LMS, a central part of Squeezebox based solution, has become a big monolithic Web application.
To be fair, it works nicely (thanks to community); however, it has an essential drawback (one may consider it as a design flaw) – everything has to be managed by LMS. Music collection (local / remote), streaming services, alarms, etc. have to be integrated within LMS through plugins or similar.  
This is where SlimStreamer comes in: it decouples Squeezebox streaming capability from the rest of the functionality around managing music ([Single responsibility principle](https://en.wikipedia.org/wiki/Single_responsibility_principle)).
Any application that outputs audio to a default ALSA device can be used as a music source for streaming.
In fact, audio streaming is done transparently by SlimStreamer and [SlimPlexor](https://github.com/gimesketvirtadieni/slimplexor) (an ALSA plugin) behind the scene.
Moreover, SlimStreamer captures PCM stream in a bit-perfect way (without resampling to a predefined sample rate), which allows streaming audio in best possible quality!

# That might be interesting… So how does it work?
![Diagram](flow.jpg)
   **#1** An arbitrary music application (MPD, Mopidy, Spotify client, … whatever) plays music by sending PCM stream to the default ALSA device through ALSA API  
   **#2** ALSA directs this PCM stream to SlimPlexor ALSA plugin, which is defined as the default ALSA device  
   **#3** SlimPlexor ALSA plugin redirects PCM data to a predefined ALSA loopback device (one per sample rate)  
   **#4** SlimStreamer keeps reading from all predefined ALSA loopback devices (sample rate is fixed for each loopback device)  
   **#5** SlimStreamer exchanges messages (commands) with Squeezebox devices through SlimProto protocol  
   **#6** SlimStreamer streams PCM data to connected Squeezebox devices through HTTP protocol

# Hmm, I want to give it a try. Where do I start?

The first step is to setup [SlimPlexor](https://github.com/gimesketvirtadieni/slimplexor) plugin properly.  
Once SlimPlexor works as required, SlimStreamer should be obtained and setup.

## Obtaining SlimStreamer binaries

The easest way to obtain SlimStreamer is to download the binary file.  
Please note that binary files are platform specific, so use below instructions only as example (you can check out all releases with binaries [here](https://github.com/gimesketvirtadieni/slimstreamer/releases))

```
wget https://github.com/gimesketvirtadieni/slimstreamer/releases/download/v0.7.0-beta/SlimStreamer_v0.7.0-beta_x86_64.zip
unzip SlimStreamer_v0.7.0-beta_x86_64.zip
./SlimStreamer
```


If SlimPlexor and ALSA were setup properly, then the output should look something like that:

```
andrej@sandbox:~/$ ./SlimStreamer 
2018/02/03 14:01:08.251632 DEBUG [140570593976832] (Streamer.hpp:75) {proto} - Streamer object was created (id=0x56076b857ba0)
2018/02/03 14:01:08.251638 DEBUG [140570593976832] (Server.hpp:86) {conn} - TCP server object was created (id=0x56076b857e50)
2018/02/03 14:01:08.251640 DEBUG [140570593976832] (Server.hpp:86) {conn} - TCP server object was created (id=0x56076b858260)
2018/02/03 14:01:08.252210 DEBUG [140570552411904] (Streamer.hpp:49) {proto} - Timer thread started
2018/02/03 14:01:08.318715 DEBUG [140570593976832] (Scheduler.hpp:33) {slim} - Scheduler object was created (id=0x56076dfc3980)
2018/02/03 14:01:08.319132 INFO [140570255939328] (Server.hpp:106) {conn} - Starting TCP new server (id=0x56076b857e50, port=3484, max connections=2)...
2018/02/03 14:01:08.319419 INFO [140570255939328] (Server.hpp:177) {conn} - Acceptor was started (id=0x7fd900000bd0, port=3484)
2018/02/03 14:01:08.319519 DEBUG [140570255939328] (Connection.hpp:47) {conn} - Connection object was created (id=0x7fd900000c40)
2018/02/03 14:01:08.319584 DEBUG [140570255939328] (Connection.hpp:197) {conn} - Connection was started (id=0x7fd900000c40)
2018/02/03 14:01:08.319655 INFO [140570255939328] (Server.hpp:148) {conn} - New connection was added (id=0x7fd900000c40, connections=1)
2018/02/03 14:01:08.319851 INFO [140570255939328] (Server.hpp:113) {conn} - TCP server was started (id=0x56076b857e50)
2018/02/03 14:01:08.319902 INFO [140570255939328] (Server.hpp:106) {conn} - Starting TCP new server (id=0x56076b858260, port=9001, max connections=2)...
2018/02/03 14:01:08.319947 INFO [140570255939328] (Server.hpp:177) {conn} - Acceptor was started (id=0x7fd9000008c0, port=9001)
2018/02/03 14:01:08.319975 DEBUG [140570255939328] (Connection.hpp:47) {conn} - Connection object was created (id=0x7fd900001540)
2018/02/03 14:01:08.320001 DEBUG [140570255939328] (Connection.hpp:197) {conn} - Connection was started (id=0x7fd900001540)
2018/02/03 14:01:08.320043 INFO [140570255939328] (Server.hpp:148) {conn} - New connection was added (id=0x7fd900001540, connections=1)
2018/02/03 14:01:08.320075 INFO [140570255939328] (Server.hpp:113) {conn} - TCP server was started (id=0x56076b858260)
2018/02/03 14:01:08.320394 DEBUG [140570239153920] (Scheduler.hpp:61) {slim} - Starting PCM data capture thread (id=0x56076dfc3980)
2018/02/03 14:01:08.338551 DEBUG [140570230761216] (Scheduler.hpp:61) {slim} - Starting PCM data capture thread (id=0x56076dfc3980)
2018/02/03 14:01:08.342845 DEBUG [140570222368512] (Scheduler.hpp:61) {slim} - Starting PCM data capture thread (id=0x56076dfc3980)
2018/02/03 14:01:08.356469 DEBUG [140570213975808] (Scheduler.hpp:61) {slim} - Starting PCM data capture thread (id=0x56076dfc3980)
2018/02/03 14:01:08.364639 DEBUG [140570205583104] (Scheduler.hpp:61) {slim} - Starting PCM data capture thread (id=0x56076dfc3980)
2018/02/03 14:01:08.375349 DEBUG [140570197190400] (Scheduler.hpp:61) {slim} - Starting PCM data capture thread (id=0x56076dfc3980)
2018/02/03 14:01:08.389961 DEBUG [140569850410752] (Scheduler.hpp:61) {slim} - Starting PCM data capture thread (id=0x56076dfc3980)
2018/02/03 14:01:08.398418 DEBUG [140569842018048] (Scheduler.hpp:61) {slim} - Starting PCM data capture thread (id=0x56076dfc3980)
2018/02/03 14:01:08.408340 DEBUG [140569833625344] (Scheduler.hpp:61) {slim} - Starting PCM data capture thread (id=0x56076dfc3980)
2018/02/03 14:01:08.422830 DEBUG [140569825232640] (Scheduler.hpp:61) {slim} - Starting PCM data capture thread (id=0x56076dfc3980)
2018/02/03 14:01:08.431553 DEBUG [140569816839936] (Scheduler.hpp:61) {slim} - Starting PCM data capture thread (id=0x56076dfc3980)
2018/02/03 14:01:08.441383 DEBUG [140569808447232] (Scheduler.hpp:61) {slim} - Starting PCM data capture thread (id=0x56076dfc3980)
2018/02/03 14:01:08.455706 DEBUG [140569800054528] (Scheduler.hpp:61) {slim} - Starting PCM data capture thread (id=0x56076dfc3980)
2018/02/03 14:01:08.464388 DEBUG [140569791661824] (Scheduler.hpp:91) {slim} - Starting streamer thread (id=0x56076dfc3980)
...
```


# Alright, I think I can handle building SlimStreamer myself

Although building SlimStreamer is a straightforward procedure, it requires that all the prerequisites to be installed.
The description below explains compilation procedure in details.


## Required prerequisites

SlimStreamer is written in C++17 so an adequate GCC compiler must be available (the commands below also installs other prerequisites like ALSA headers, etc.):

```
sudo apt-get update
sudo apt-get install git build-essential g++ cmake libasound2-dev libflac++-dev zip unzip
```

Unzip and zip programs are used only by compilation script when ```-z``` paramter is provided (it is used for making a release).
To validate if installed compiler supports C++17, use this command:

```
andrej@sandbox:~$ g++ --version                                                                                                                                                
g++ (Ubuntu 7-20170407-0ubuntu2) 7.0.1 20170407 (experimental) [trunk revision 246759]                                                                                                                            
Copyright (C) 2017 Free Software Foundation, Inc.                                                                                                                                                                 
This is free software; see the source for copying conditions.  There is NO                                                                                                                                        
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```

If you got version 7.x (or above) installed, it will do for compiling SlimStreamer.
However GCC version 7.x is not yet shipped by default for most Linux distributions.
In other words, chances are high that a lower version of the compiler will be present, which leads to a question how you should upgrade it to version 7.x (which supports C++17).
Here you go:

```
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install gcc-snapshot
sudo apt-get update
sudo apt-get install gcc-7 g++-7
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 60 --slave /usr/bin/g++ g++ /usr/bin/g++-7
```

After upgrading GCC, you should get a similar output as above when issuing a command ```g++ --version```.


## Compiling SlimStreamer

To compile SlimStreamer one should do like following:

1. Obtain the source code

```
git clone --recurse-submodules https://github.com/gimesketvirtadieni/slimstreamer.git
cd slimstreamer
./scripts/createLocalBranches.sh
```

It will download all the source code needed for compiling SlimStreamer.
The last two lines are required only if you want to get all the branches into your local repo.


2. Compiling SlimStreamer

Assuming all required packages are installed, compilation step is as simple as that:

```
cd make
../scripts/compile -d
```

```-d``` parameter is used to tell the compilation script to build dependences as well (like g3logger).
Building dependencies is required only once; the compilation script may be used without ```-d``` parameter later.
If compilation succeeded then the current directory should look like following:

```
andrej@sandbox:~/slimstreamer/make$ ls
ConsoleSink.o  Makefile  SinkFilter.o  SlimStreamer  SlimStreamer.o  Source.o
```

Compilation process produces one executable file - SlimStreamer (the rest of the files are not needed to run it).


# Running SlimStreamer

To stream music to Squeezebox players, they need to be "directed" to use SlimStreamer (instead of LMS).
This should be done by providing hostname and port (3483) to all players so they can connect to SlimStreamer.
If you are using squeezelite, a command (```-s``` parameter defines a server to connect to) may look like following (should be run on a host with an attached DAC; a server running SlimStreamer does not required a 'physical' DAC attached):

```
andrej@sandbox:~$ /usr/bin/squeezelite -n playername -o outputdevice -d all=debug -s slimstreamerhost:3483
[01:50:16.200439] stream_init:290 init stream
[01:50:16.200635] stream_init:291 streambuf size: 2097152
[01:50:16.202250] output_init_alsa:817 init output
[01:50:16.202339] output_init_alsa:846 requested alsa_buffer: 40 alsa_period: 4 format: any mmap: 1
[01:50:16.202396] output_init_common:346 outputbuf size: 3528000
[01:50:16.202466] output_init_common:370 idle timeout: 0
[01:50:16.206681] output_init_common:410 supported rates: 384000 352800 192000 176400 96000 88200 48000 24000 22500 16000 12000 8000 
...
```

That's it, enjoy streaming ;)


# Development Status

SlimStreamer works end-to-end and it is fairly stable hence can be used for beta testing, however the development is still ongoing:  
  * Capture-and-deliver PCM data to SlimStreamer in a bit-perfect way is done
  * TCP server (running on 3483 port) required for serving SlimProto commands is implemented
  * SlimProto players are able to connect to SlimStreamer (although tested only with squeezelite)
  * TCP server (running on 9000 port), required for streaming PCM data is done
  * Streaming functionality works
  * Support for multiple sampling rates is done
  * Transcoding 'raw' PCM data to lossless format (FLAC) is done (it decreases amount of used network bandwidth by 2,5 times)
  * SlimProto auto-discovery is still missing
  * Streams synchronization is still missing
  * There are many 'shortcuts' left in the codebase so in many ways it is still requires maturing

Any feedback and comments are much appreciated!
