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
Once SlimPlexor works as required, proceed with the steps below.


## Required prerequisites

SlimStreamer is written in C++17 so it requires an adequate C++ compiler (the commands below also installs ALSA headers required for using ALSA API):

```
sudo apt-get update
sudo apt-get install build-essential g++ libasound2-dev
```

To validate if C++ compiler supports C++17, use this command:

```
andrej@sandbox:~$ g++ --version                                                                                                                                                
g++ (Ubuntu 7-20170407-0ubuntu2) 7.0.1 20170407 (experimental) [trunk revision 246759]                                                                                                                            
Copyright (C) 2017 Free Software Foundation, Inc.                                                                                                                                                                 
This is free software; see the source for copying conditions.  There is NO                                                                                                                                        
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```

If you got version 7.x (or above) installed, it will do for compiling SlimStreamer


## Compiling SlimStreamer

Building SlimStreamer is done in three steps:

1. Obtaining source code

```
git clone https://github.com/gimesketvirtadieni/slimstreamer.git
cd slimstreamer
git submodule update --init --recursive
```

It will download all the source code needed for compiling SlimStreamer.


2. Compiling dependencies

SlimStreamer uses number of dependencies however most of them are provided in header files, so they do not need to be compiled separately.
The only dependency that needs to be compiled separately is a logger library.
To compile the logger library, one should do like following (from a 'slimstreamer' directory):

```
cd dependencies
cd g3log
ls
```

The output should look similar to:

```
andrej@sandbox:~/slimstreamer/dependencies/g3log$ ls
3rdParty      build        CleanAll.cmake  CPackLists.txt  GenerateMacroDefinitionsFile.cmake  Options.cmake    scripts  sublime.formatting  test_performance
API.markdown  Build.cmake  CMakeLists.txt  example         LICENSE                             README.markdown  src      test_main           test_unit
```

Now the actual compilation steps for the logger library:

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DUSE_DYNAMIC_LOGGING_LEVELS=ON ..
make
ls
```

The output should look similar to:

```
andrej@sandbox:~/slimstreamer/dependencies/g3log/build$ ls
CMakeCache.txt  cmake_install.cmake  CPackSourceConfig.cmake  g3log-FATAL-contract  libg3logger.a   Makefile
CMakeFiles      CPackConfig.cmake    g3log-FATAL-choice       g3log-FATAL-sigsegv   libg3logger.so
```

One more important step:

```
rm libg3logger.so
```

It is required to remove dynamic library file (libg3logger.so) to make sure linker bundles everything into one executable file, avoiding any dependencies to dynamic libraries.


3. Compiling SlimStreamer itself

```
cd ../../..
cd make
make
ls
```

If compilation succeeded then the output should look like following:

```
andrej@sandbox:~/slimstreamer/make$ ls
Makefile  SlimStreamer
```


## Running SlimStreamer

Compilation process produces one binary executable file - SlimStreamer.
Just run it and point your Squeezebox players to use it instead of LMS.
If you are using squeezelite, a command may look like following (should be run on a host with an attached DAC; a server running SlimStreamer does not required a 'physical' DAC attached):

```
andrej@sandbox:~$ /usr/bin/squeezelite -n playername -o outputdevice -d all=debug -s slimstreamerhost:3484
[01:50:16.200439] stream_init:290 init stream
[01:50:16.200635] stream_init:291 streambuf size: 2097152
[01:50:16.202250] output_init_alsa:817 init output
[01:50:16.202339] output_init_alsa:846 requested alsa_buffer: 40 alsa_period: 4 format: any mmap: 1
[01:50:16.202396] output_init_common:346 outputbuf size: 3528000
[01:50:16.202466] output_init_common:370 idle timeout: 0
[01:50:16.206681] output_init_common:410 supported rates: 384000 352800 192000 176400 96000 88200 48000 24000 22500 16000 12000 8000 
...
```

Injoy streaming ;)


# Development Status

SlimStreamer works and can be used for alpha testing, however the development is still ongoing:  
  * Capture-and-deliver PCM data to SlimStreamer in a bit-perfect way is done
  * TCP server (running on 3484 port) required for serving SlimProto commands is implemented (although only few commands are supported for now)
  * SlimProto players are able to connect to SlimStreamer (although tested only with squeezelite)
  * TCP server (running on 9001 port), required for streaming PCM data, works
  * HTTP streaming functionality works
  * Streaming functionality supports multiple sampling rates
  * Streams synchronization is still missing
  * Streaming data should be encoded in lossless way (to prevent from sending 'raw' PCM data over the network)
  * There are many 'shortcuts' left in the codebase so in many ways it is still requires maturing

Any feedback and comments are much appreciated!
