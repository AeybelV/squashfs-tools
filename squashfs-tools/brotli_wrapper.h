#ifndef BROTLI_WRAPPER_H
#define BROTLI_WRAPPER_H

struct gzip_comp_opts {
	int compression_level;
	short window_size;
	short strategy;
};

struct strategy {
	char *name;
	int strategy;
	int selected;
};

struct gzip_strategy {
	int strategy;
	int length;
	void *buffer;
};

struct gzip_stream {
	z_stream stream;
	int strategies;
	struct gzip_strategy strategy[0];
};
#endif
