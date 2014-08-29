#!/bin/bash

FFMPEG_REPOSITORY="git://source.ffmpeg.org/ffmpeg.git"
FFMPEG_CONFIGURE_OPTIONS="--enable-gpl --enable-version3 --enable-shared --enable-static --enable-small --disable-yasm --disable-hwaccels \
                          --disable-swresample --disable-swscale --disable-bsfs --disable-protocols --enable-protocol=http,file,data,pipe \
                          --disable-filters --disable-avfilter --disable-devices --disable-avdevice --disable-encoders --disable-decoders \
                          --disable-decoder=h263,h264,hevc,vc1 --disable-postproc \
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
gcc -I${PWD}/bootstrap/include -lm -oavbuggyduration avbuggyduration.c bootstrap/lib/lib{avutil,avcodec,avformat}.a || exit $!
echo 'done.'
