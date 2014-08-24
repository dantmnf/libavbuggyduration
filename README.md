libavbuggyduration
==================

Generate media files with buggy duration to bypass re-encoding (for some online video sites only)

Mainly in 3 ways:
1. Repeat first three video frames in specified time, may be recognized as VFR by most players (aka. 后黑)
2. Repeat first audio frame in specified time
3. Multiply all timestamps and make the last timestamp to specified time (aka. ~~红色有角三~~倍速)

There is nothing now. Please check it months later.

Docs
------

### Get Started
* For GNU/Linux and OS X, install `FFmpeg` or `LibAV` and its development files, then `make`. (`pkg-config` must work well)
* For Windows, it is possible to build with MinGW or Cygwin

### Usage
`avbuggyduration -i input_file -d target_duration -m method -o output_file`
Methods avaliable: `video`(1), `audio`(2), `both`(1 and 2), `speed`(3)