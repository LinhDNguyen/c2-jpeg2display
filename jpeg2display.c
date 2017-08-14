#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <codec.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>

#define INPUT_HD 1
#define INPUT_1K 2
#define INPUT_4K 3

// SPECIFY INPUT RESOLUTION
#define INPUT_TYPE INPUT_1K

// 1K
#if (INPUT_TYPE == INPUT_1K)
#define FILE_NAME "frame1k.jpg"
#define IN_WIDTH  1920
#define IN_HEIGHT 1080
#endif

// HD
#if (INPUT_TYPE == INPUT_HD)
#define FILE_NAME "framehd.jpg"
#define IN_WIDTH  1280
#define IN_HEIGHT 720
#endif

static codec_para_t s_v_codec_para;
static codec_para_t *s_pcodec = NULL;
static FILE* fp = NULL;
static char s_buffer[10 * 1024 * 1024];

static int osd_blank(char *path,int cmd);
static int set_tsync_enable(int enable);
static void signal_handler(int signum);

int main(int argc, char *argv[]) {
	int ret = CODEC_ERROR_NONE;
	int len = 0;
	int size = 0;
	struct buf_status vbuf;

	s_pcodec = &s_v_codec_para;

	osd_blank("/sys/class/graphics/fb0/blank",1);
	osd_blank("/sys/class/graphics/fb1/blank",0);

	memset(s_pcodec, 0, sizeof(codec_para_t));
	
	s_pcodec->has_video = 1;
	s_pcodec->stream_type = STREAM_TYPE_ES_VIDEO;
	s_pcodec->video_type = VFORMAT_MJPEG;
	s_pcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_MJPEG;
	s_pcodec->am_sysinfo.rate = 96000 / 30; // FPS = 30
	s_pcodec->am_sysinfo.height = IN_HEIGHT;
	s_pcodec->am_sysinfo.width = IN_WIDTH;
	s_pcodec->has_audio = 0;
	s_pcodec->noblock = 0;
	
	ret = codec_init(s_pcodec);
	if (ret != CODEC_ERROR_NONE) {
		fprintf(stderr, "Codec init failed\n");
		return -1;
	}
	printf("Video codec init ok\n");

	fp = fopen(FILE_NAME, "rb");
	if (!fp) {
		fprintf(stderr, "Open JPG file failed\n");
		return -2;
	}

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	printf("data length: %d\n", size);
	ret = fread(s_buffer, 1, size, fp);
	printf("%d bytes read\n", ret);
	fclose(fp);

	signal(SIGCHLD, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGHUP, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGQUIT, signal_handler);

	do {
		len = 0;
		do {
			ret = codec_write(s_pcodec, s_buffer + len, size - len);
			if (ret > 0) {
				len += ret;
			} else {
				fprintf(stderr, "write error, offset %d, err %s\n", len, strerror(errno));
			}
		} while (len < size);
	} while (1);

	if (s_pcodec) {
		codec_close(s_pcodec);
		s_pcodec = NULL;
	}

	osd_blank("/sys/class/graphics/fb0/blank",0);
	osd_blank("/sys/class/graphics/fb1/blank",0);
	
	return 0;
}

static int osd_blank(char *path,int cmd)
{
	int fd;
	char  bcmd[16];
	fd = open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);

	if(fd>=0) {
		sprintf(bcmd,"%d",cmd);
		(void)write(fd,bcmd,strlen(bcmd));
		close(fd);
		return 0;
	}

	return -1;
}

static int set_tsync_enable(int enable)
{
	int fd;
	char *path = "/sys/class/tsync/enable";
	char  bcmd[16];
	fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
	if (fd >= 0) {
		sprintf(bcmd, "%d", enable);
		write(fd, bcmd, strlen(bcmd));
		close(fd);
		return 0;
	}

	return -1;
}

static void signal_handler(int signum)
{
	printf("Get signum=%x\n",signum);
	if (s_pcodec) {
		codec_close(s_pcodec);
		s_pcodec = NULL;
	}
	if (fp) {
		fclose(fp);
		fp = NULL;
	}
	osd_blank("/sys/class/graphics/fb0/blank",0);
	osd_blank("/sys/class/graphics/fb1/blank",0);
	exit(0);
	//signal(signum, SIG_DFL);
	//raise (signum);
}

