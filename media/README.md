ffmpeg -f lavfi -i testsrc2=size=1280x720:rate=30 -f lavfi -i sine=frequency=440:sample_rate=48000 -c:v libx264 -pix_fmt yuv420p -c:a aac -ac 2 -shortest media/test_720p.mp4

ffmpeg -f lavfi -i testsrc2=1920x1080:rate=25 -f lavfi -i sine=frequency=880:sample_rate=44100 -c:v libx264 -pix_fmt yuv420p -c:a aac -ac 2 -shortest media/test_1080p25.mp4

这两个指令可以生成两条彩色视频测试素材
