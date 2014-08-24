CFLAGS+=`pkg-config --libs --cflags libavcodec libavformat libavutil`

avbuggyduration: avbuggyduration.c
