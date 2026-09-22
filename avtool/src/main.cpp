#include <cstdio>
#include <string>
extern "C" {
#include<libavformat/avformat.h>
#include<libavcodec/avcodec.h>
#include<libavutil/avutil.h>
#include<libswscale/swscale.h>
#include<libavutil/imgutils.h>
}
static int cmdInfo(const std::string& path);
static int cmdPackets(const std::string& path);
static int cmdFrame(const std::string& path);
static void writePPM(const char* path, const uint8_t* data, int linesize, int w, int h);
int main(int argc, char** argv) {
	if (argc < 3) {
		printf("用法: avtool<info/packets/frame/decode> <文件> [附加参数]\n");
		return 1;
	}
	std::string cmd = argv[1];
	std::string path = argv[2];
	if (cmd == "info") return cmdInfo(path);
	if (cmd == "packets") return cmdPackets(path);
	if (cmd == "frame") return cmdFrame(path);
	printf("未知命令: %s\n", cmd.c_str());
	return 1;
	

}
static int cmdInfo(const std::string& path) {
	AVFormatContext* fmt = nullptr;
	int ret = avformat_open_input(&fmt, path.c_str() , nullptr, nullptr);
	if (ret < 0) {
		char errbuf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
		av_strerror(ret, errbuf, sizeof(errbuf));
		printf("打开失败: %s\n", errbuf);
		return ret;
	}
	printf("----我的汇总----\n");
	printf("时长: %.2f 秒\n", fmt->duration / (double)AV_TIME_BASE);
	printf("总码率: %lld bps\n", (long long)fmt->bit_rate);
	for (unsigned i = 0; i < fmt->nb_streams;i++) {
		AVStream* st = fmt->streams[i];
		const char* cname = avcodec_get_name(st->codecpar->codec_id);
		if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			printf("[视频] %s %dx%d %.2f fps timebase=%d/%d\n", cname, st->codecpar->width,
				st->codecpar->height, av_q2d(st->avg_frame_rate),st->time_base.num, st->time_base.den );
		}
		if (st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			printf("[音频] %s %d Hz %d 声道  timebase=%d/%d\n", cname, st->codecpar->sample_rate, st->codecpar->ch_layout.nb_channels, st->time_base.num,st->time_base.den);
		}
		
	}
	avformat_close_input(&fmt);
	return 0;
	
}
static int cmdPackets(const std::string& path) {
	AVFormatContext* fmt = nullptr;
	int ret = avformat_open_input(&fmt, path.c_str(), nullptr, nullptr);
	if (ret < 0) {
		char errbuf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
		av_strerror(ret, errbuf, sizeof(errbuf));
		printf("文件打开失败: %s\n", errbuf);
		return ret;
	}
	AVPacket* pkt = av_packet_alloc();
	if (!pkt) return -1;
	int64_t videoPkts = 0, audioPkts = 0, videoKeys = 0;
	int printed = 0;
	while (av_read_frame(fmt, pkt) >= 0)
	{
		if (pkt->stream_index == 0) {
			++videoPkts;
			if (pkt->flags & AV_PKT_FLAG_KEY) ++videoKeys;

		}
		if (printed < 50) {
			printf("流=%d pts=%8lld dts=%8lld key=%d size=%6d\n", pkt->stream_index, (long long)pkt->pts,(long long)pkt->dts,
				(long long)(pkt->flags & AV_PKT_FLAG_KEY) ? 1 : 0, pkt->size);
			++printed;
		}
		av_packet_unref(pkt);
	}
	printf("----统计----\n");
	printf("0号流包数=%lld,其中关键帧包=%lld\n", (long long)videoPkts, (long long)videoKeys);
	av_packet_free(&pkt);
	avformat_close_input(&fmt);
	return 0;
}
static int cmdFrame(const std::string& path) {
	AVFormatContext* fmt = nullptr;
	int ret = avformat_open_input(&fmt, path.c_str(), nullptr, nullptr);
	if (ret < 0) {
		char errbuf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
		av_strerror(ret, errbuf, sizeof(errbuf));
		printf("文件打开失败: %s\n", errbuf);
		return ret;
	}
	int vIdx = av_find_best_stream(fmt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
	if (vIdx < 0) { printf("没找到视频流\n"); return -1; }
	const AVCodec* decoder = avcodec_find_decoder(fmt->streams[vIdx]->codecpar->codec_id);
	if (!decoder) { printf("没有找到对应解码器\n"); return -1; }
	AVCodecContext* cctx = avcodec_alloc_context3(decoder);
	avcodec_parameters_to_context(cctx, fmt->streams[vIdx]->codecpar);
	if (avcodec_open2(cctx, decoder, nullptr) < 0) { printf("打开解码器失败\n"); return -1; }

	AVPacket* pkt = av_packet_alloc();
	AVFrame* frame = av_frame_alloc();
	int gotFrame = 0;
	while (!gotFrame && av_read_frame(fmt, pkt) >= 0) {
		if (pkt->stream_index != vIdx) {
			av_packet_unref(pkt);
			continue;
		}
		int ret = avcodec_send_packet(cctx, pkt);
		av_packet_unref(pkt);
		if (ret < 0) { printf("send 失败\n"); break; }
		ret = avcodec_receive_frame(cctx, frame);
		if (ret == 0) {
			gotFrame = 1;
			int w = frame->width, h = frame->height;
			SwsContext* sws = sws_getContext(w, h, (AVPixelFormat)frame->format, w, h, AV_PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr,nullptr);
			uint8_t* rgb[4] = { nullptr };
			int rgbLinesize[4] = { 0 };
			av_image_alloc(rgb, rgbLinesize, w, h, AV_PIX_FMT_RGB24, 1);
			sws_scale(sws, frame->data, frame->linesize, 0, h, rgb, rgbLinesize);
			writePPM("out.ppm", rgb[0], rgbLinesize[0], w, h);
			av_freep(&rgb[0]);
			sws_freeContext(sws);
			av_frame_free(&frame);
			av_packet_free(&pkt);
			avcodec_free_context(&cctx);
			avformat_close_input(&fmt);
		}
	}

	if (!gotFrame) { printf("没解出帧\n");return -1; }
	printf("第一帧: %dx%d 像素格式=%d\n", frame->width, frame->height, frame->format);
}
static void writePPM(const char* path, const uint8_t* data, int linesize, int w, int h) {
	FILE* f = fopen(path, "wb");
	if (!f) { printf("写文件失败\n");return; }
	fprintf(f, "P6\n%d %d\n255\n", w, h);
	for (int y = 0;y < h;++y) {
		fwrite(data + (size_t)y * linesize, 1, (size_t)w * 3, f);
	}
	fclose(f);
}