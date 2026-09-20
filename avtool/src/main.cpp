#include <cstdio>
#include <string>
extern "C" {
#include<libavformat/avformat.h>
#include<libavcodec/avcodec.h>
#include<libavutil/avutil.h>
}
static int cmdInfo(const std::string& path);
static int cmdPackets(const std::string& path);
int main(int argc, char** argv) {
	if (argc < 3) {
		printf("用法: avtool<info/packets/frame/decode> <文件> [附加参数]\n");
		return 1;
	}
	std::string cmd = argv[1];
	std::string path = argv[2];
	if (cmd == "info") return cmdInfo(path);
	if (cmd == "packets") return cmdPackets(path);
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