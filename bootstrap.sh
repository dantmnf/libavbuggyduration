#!/bin/bash

FFMPEG_REPOSITORY="git://source.ffmpeg.org/ffmpeg.git"
FFMPEG_CONFIGURE_OPTIONS="--enable-gpl --enable-version3 --enable-shared --enable-static --enable-small --disable-hwaccels \
                          --disable-swresample --disable-swscale --disable-bsfs --disable-protocols --enable-protocol=http,file,data,pipe \
                          --disable-filters --disable-avfilter --disable-devices --disable-avdevice --disable-encoders --disable-decoders \
                          --disable-demuxers --disable-muxers --enable-demuxer=adts,asf,avi,flv,mp4,matroska,flac,mpegts,mpeg,ogg,wav \
                          --enable-muxer=adts,asf,avi,flv,mp4,matroska,flac,mpegts,mpeg,ogg,wav --disable-parsers --disable-postproc \
                          --disable-zlib --disable-bzlib --disable-xlib --disable-iconv --disable-pthreads --disable-ffmpeg --disable-ffplay \
                          --enable-ffprobe --disable-ffserver --extra-cflags=-static"
pushd $PWD
error() {
  popd
  exit $1
}

mkdir -p bootstrap
cd bootstrap

echo 'getting FFmpeg...'
if [ -d ffmpeg ]
then
  cd ffmpeg
  git pull
else
  git clone "$FFMPEG_REPOSITORY" ffmpeg
  cd ffmpeg
fi

echo 'configuring FFmpeg...'
LDFLAGS="-lm" ./configure --prefix="$PWD/.." $FFMPEG_CONFIGURE_OPTIONS || exit $!

echo 'building FFmpeg...'
make || exit $!
make install || exit $!
cd .. # to bootstrap

echo 'building avbuggyduration...'
cd ..
rm ffmpeg.a
ar -rcT ffmpeg.a ${PWD}/bootstrap/lib/*.a
gcc -I${PWD}/bootstrap/include -lm -oavbuggyduration avbuggyduration.c ffmpeg.a || exit $!
echo 'done.'