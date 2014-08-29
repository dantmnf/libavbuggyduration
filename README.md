libavbuggyduration
==================

Generate media files with buggy duration to bypass re-encoding (for some online video sites only)

Mainly in 3 ways:

1.  Repeat first three video frames in specified time, may be recognized as VFR by most players (aka. 后黑)
2.  Repeat first audio frame in specified time
3.  Multiply all timestamps and make the last timestamp to specified time (aka. ~~红色有角三~~倍速, for playback on Adobe Flash only)

Docs
------

### Get Started
* For GNU/Linux, if you are lucky enough, install FFmpeg and its development files, then `make`
* Otherwise, run `bootstrap.sh`, which will build a specially configured FFmpeg first, and build a static binary

### Usage
`avbuggyduration -i input_file -d target_duration -m method -o output_file`

Methods avaliable: `video`(1), `audio`(2), `both`(1 and 2), `speed`(3)

For different video and audio duration, run the program twice : )

For Windows users: you had better redirect output to deivce `NUL`, like this:

    Microsoft Windows [Version 6.3.9600]
    (c) 2013 Microsoft Corporation. All rights reserved.
     
    C:\Users\dant>avbuggyduration -i test_input.mp4 -o test_output.mp4 -m audio -d 1024 > NUL

Because Windows Command Prompt is extremely slow (compared to virtual terminals on Linux / OS X, mintty.exe, andeven Linux tty)
