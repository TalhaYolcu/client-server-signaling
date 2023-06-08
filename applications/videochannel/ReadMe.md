sender:

gst-launch-1.0 v4l2src device=/dev/video0 ! video/x-raw,width=640,height=480 ! videoconvert ! queue ! x264enc tune=zerolatency bitrate=1000 key-int-max=30 ! video/x-h264, profile=constrained-baseline ! rtph264pay pt=96 mtu=1200 ! udpsink host=127.0.0.1 port=6000


receiver:

gst-launch-1.0 udpsrc address=127.0.0.1 port=5000 caps="application/x-rtp" ! queue ! rtph264depay ! video/x-h264,stream-format=byte-stream ! queue ! avdec_h264 ! queue ! autovideosink


save:

gst-launch-1.0 udpsrc location=http://127.0.0.1:5000 ! h264parse ! avdec_h264 ! filesink location=/path_to_file/file.mp4

