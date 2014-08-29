/*
 * Copyright (c) 2013 Stefano Sabatini
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file
 * libavformat/libavcodec demuxing and muxing API example.
 *
 * Remux streams from one container format to another.
 * @example remuxing.c
 */
#include <stdlib.h>
#include <unistd.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>

static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt, const char *tag)
{
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    printf("%s: pts@%s(%ss)  dts@%s(%ss) +%s(%ss) Stream #%d\n",
           tag,
           av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
           pkt->stream_index);
}

void display_usage_and_exit(char** argv, int exit_status) {
    printf("usage: %s -i input -o output -d duration_in_second -m method\n"
            "Give media files buggy duration.\n"
            "The output format must be the same to input format.\n"
            "Methods: audio, video, both, speed"
            "\n", argv[0]);
    exit(exit_status);
}

int main(int argc, char **argv)
{
    AVOutputFormat *ofmt = NULL;
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
    AVPacket pkt, vpacket_for_buggy[3], apacket_for_buggy[1];
    const char *in_filename = NULL, *out_filename = NULL;
    int ret, i, opt, video_stream_id, audio_stream_id, buggy_duration, buggy_video_packet_count = 0, buggy_audio_packet_count = 0;
    int buggy_video_required = 0, buggy_audio_required = 0, buggy_speed_required = 0;
    uint32_t opts_flags = 0x00;
    double speed_factor = 1.0;

    opt = getopt(argc, argv, "i:o:d:m:");
    while (opt != -1) {
        switch (opt) {
            case 'i':
                in_filename = malloc(strlen(optarg)+1);
                strcpy(in_filename, optarg);
                opts_flags |= 0x01;
                break;
                
            case 'o':
                out_filename = malloc(strlen(optarg)+1);
                strcpy(out_filename, optarg);
                opts_flags |= 0x02;
                break;
                
            case 'd':
                sscanf(optarg, "%d", &buggy_duration);
                opts_flags |= 0x04;
                break;
                
            case 'm':
                if (strcmp(optarg, "video") == 0)
                    buggy_video_required = 1;
                if (strcmp(optarg, "audio") == 0)
                    buggy_audio_required = 1;
                if (strcmp(optarg, "both") == 0) {
                    buggy_audio_required = 1;
                    buggy_video_required = 1;
                }
                if (strcmp(optarg, "speed") == 0)
                    buggy_speed_required = 1;
                if (buggy_video_required + buggy_speed_required + buggy_audio_required == 0)
                    display_usage_and_exit(argv, 1);
                opts_flags |= 0x08;
                break;
                
            case 'h':
            case '?':
                display_usage_and_exit(argv, 0);
                break;
                
            default:
                break;
        }
        opt = getopt(argc, argv, "i:o:d:m:");
    }

    if (opts_flags != 0x0F) 
        display_usage_and_exit(argv, 1);

    av_register_all();

    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
        fprintf(stderr, "Could not open input file '%s'", in_filename);
        goto end;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        fprintf(stderr, "Failed to retrieve input stream information");
        goto end;
    }

    av_dump_format(ifmt_ctx, 0, in_filename, 0);

    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
    if (!ofmt_ctx) {
        fprintf(stderr, "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }

    ofmt = ofmt_ctx->oformat;

    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream *in_stream = ifmt_ctx->streams[i];
        if (in_stream->codec->codec_type == AVMEDIA_TYPE_VIDEO) video_stream_id = i;
        if (in_stream->codec->codec_type == AVMEDIA_TYPE_AUDIO) audio_stream_id = i;
        AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
        if (!out_stream) {
            fprintf(stderr, "Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }

        ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
        if (ret < 0) {
            fprintf(stderr, "Failed to copy context from input to output stream codec context\n");
            goto end;
        }
        out_stream->codec->codec_tag = 0;
        if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
    av_dump_format(ofmt_ctx, 0, out_filename, 1);

    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open output file '%s'", out_filename);
            goto end;
        }
    }

    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        goto end;
    }


    //calcluate speed factor
    if(buggy_speed_required) {
        speed_factor = (double)(buggy_duration * AV_TIME_BASE) / (double)ifmt_ctx->duration;
    }


    while (1) {
        AVStream *in_stream, *out_stream;

        ret = av_read_frame(ifmt_ctx, &pkt);
        if (ret < 0)
            break;


        in_stream  = ifmt_ctx->streams[pkt.stream_index];
        out_stream = ofmt_ctx->streams[pkt.stream_index];

        /* copy packet */
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;


        if (buggy_speed_required) {
            pkt.pts = (uint64_t)((double)pkt.pts * speed_factor);
            pkt.dts = (uint64_t)((double)pkt.dts * speed_factor);
            goto __write_packet;
        }


        if (buggy_video_required && pkt.stream_index == video_stream_id && buggy_video_packet_count < 3) {
            AVPacket swap_packet;
            av_copy_packet(&swap_packet, &pkt);
            av_copy_packet(&vpacket_for_buggy[buggy_video_packet_count], &pkt);
            av_copy_packet_side_data(&swap_packet, &pkt);
            av_copy_packet_side_data(&vpacket_for_buggy[buggy_video_packet_count], &pkt);
            vpacket_for_buggy[buggy_video_packet_count].pts = av_add_stable(out_stream->time_base, pkt.pts, av_make_q(1,1), buggy_duration);
            vpacket_for_buggy[buggy_video_packet_count].dts = av_add_stable(out_stream->time_base, pkt.dts, av_make_q(1,1), buggy_duration);
            av_free_packet(&pkt); //original pkt seems to be dirty
            pkt = swap_packet;
            buggy_video_packet_count++;
            
        } 

        if (buggy_audio_required && pkt.stream_index == audio_stream_id && buggy_audio_packet_count < 1) {
            AVPacket swap_packet;
            av_copy_packet(&swap_packet, &pkt);
            av_copy_packet_side_data(&swap_packet, &pkt);
            av_copy_packet(&apacket_for_buggy[buggy_audio_packet_count], &pkt);
            av_copy_packet_side_data(&apacket_for_buggy[buggy_audio_packet_count], &pkt);
            
            apacket_for_buggy[buggy_audio_packet_count].pts = av_add_stable(out_stream->time_base, pkt.pts, av_make_q(1,1), buggy_duration);
            apacket_for_buggy[buggy_audio_packet_count].dts = av_add_stable(out_stream->time_base, pkt.dts, av_make_q(1,1), buggy_duration);

            av_free_packet(&pkt);
            pkt = swap_packet;
            buggy_audio_packet_count++;
        }

__write_packet:
        log_packet(ofmt_ctx, &pkt, "out");

        ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
        if (ret < 0) {
            fprintf(stderr, "Error muxing packet\n");
            break;
        }


        av_free_packet(&pkt);
        
    }


    if(buggy_video_required)
    for (i=0; i<3; i++) {
        puts("writing buggy v pkt...");
        log_packet(ofmt_ctx, &vpacket_for_buggy[i], "out");
        ret = av_interleaved_write_frame(ofmt_ctx, &vpacket_for_buggy[i]);
        if (ret < 0) {
            fprintf(stderr, "Error muxing packet\n");
            break;
        }
        av_free_packet(&vpacket_for_buggy[i]);
    }

    if(buggy_audio_required)
    for (i=0; i<1; i++) {
        log_packet(ofmt_ctx, &apacket_for_buggy[i], "out");
        ret = av_interleaved_write_frame(ofmt_ctx, &apacket_for_buggy[i]);
        if (ret < 0) {
            fprintf(stderr, "Error muxing packet\n");
            break;
        }
        av_free_packet(&apacket_for_buggy[i]);;
    }


    av_write_trailer(ofmt_ctx);
end:

    avformat_close_input(&ifmt_ctx);

    /* close output */
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
        avio_close(ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);

    if (ret < 0 && ret != AVERROR_EOF) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        return 1;
    }

    free((void*)in_filename);
    free((void*)out_filename);

    return 0;
}
