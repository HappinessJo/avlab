#include <cstdio>
extern "C" {
#include<libavformat/avformat.h>
#include<libavcodec/avcodec.h>
#include<libavutil/avutil.h>
}
int main(int argc, char** argv) {
	printf("av_version: %s\n", av_version_info());
	printf("avformat: %u.%u.%u\n",
		LIBAVFORMAT_VERSION_MAJOR, LIBAVFORMAT_VERSION_MINOR, LIBAVFORMAT_VERSION_MICRO);
	printf("avcodec: %d\n", avcodec_version());
	return 0;

}