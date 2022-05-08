B_play:main.c B_Play.c B_Public.c
	gcc main.c B_Play.c B_Public.c \
	../lib/*.so \
	/usr/local/lib/libSDL2.a -Wl,--no-undefined -lm -ldl -lpthread -lrt \
	-o B_Play  \
	-Iinclude \
	-Iinclude/SDL-2.0.17-3b2fbb1/src/video \
	-Iinclude/SDL-2.0.17-3b2fbb1/src \
	-Iinclude/SDL-2.0.17-3b2fbb1/src/dynapi \
	-Iinclude/SDL-2.0.17-3b2fbb1/include