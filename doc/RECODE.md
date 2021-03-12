
mv dl-slates.m2v dl-slates-orig.m2v

ffmpeg -i dl-slates-orog.m2v -vf scale=640:480 -codec:v mpeg2video -b 4182k dl-slates.m2v
