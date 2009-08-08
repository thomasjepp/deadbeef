#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "codec.h"
#include "cvorbis.h"
#include "playlist.h"

static FILE *file;
static OggVorbis_File vorbis_file;
static vorbis_info *vi;
static int cur_bit_stream;

void
cvorbis_free (void);

int
cvorbis_init (const char *fname, int track, float start, float end) {
    file = NULL;
    vi = NULL;
    cur_bit_stream = -1;

    file = fopen (fname, "rb");
    if (!file) {
        return -1;
    }

    memset (&cvorbis.info, 0, sizeof (fileinfo_t));
    ov_open (file, &vorbis_file, NULL, 0);
    vi = ov_info (&vorbis_file, -1);
    if (!vi) { // not a vorbis stream
        cvorbis_free ();
        return -1;
    }
    cvorbis.info.bitsPerSample = 16;
    //cvorbis.info.dataSize = ov_pcm_total (&vorbis_file, -1) * vi->channels * 2;
    cvorbis.info.channels = vi->channels;
    cvorbis.info.samplesPerSecond = vi->rate;
    cvorbis.info.position = 0;
//    printf ("vorbis info: bps: %d, size: %d, chan: %d, rate: %d, dur: %f\n", cvorbis.info.bitsPerSample, cvorbis.info.dataSize, cvorbis.info.channels, cvorbis.info.samplesPerSecond, cvorbis.info.duration);
    return 0;
}

void
cvorbis_free (void) {
    if (file) {
        ov_clear (&vorbis_file);
        //fclose (file); //-- ov_clear closes it
        file = NULL;
        vi = NULL;
    }
}

int
cvorbis_read (char *bytes, int size) {
    if (!file)
        return 0;
    int initsize = size;
    for (;;)
    {
        // read ogg
        long ret=ov_read (&vorbis_file, bytes, size, 0, 2, 1, &cur_bit_stream);
        if (ret < 0)
        {
            break;
        }
        else if (ret == 0) {
            break;
        }
        else if (ret < size)
        {
            size -= ret;
            bytes += ret;
        }
        else {
            size = 0;
            break;
        }
    }
    cvorbis.info.position = ov_time_tell(&vorbis_file);

    return initsize - size;
}

int
cvorbis_seek (float time) {
    if (!file) {
        return -1;
    }
//    printf ("seeking for %f\n");
    int res = ov_time_seek (&vorbis_file, time);
    if (res != 0 && res != OV_ENOSEEK)
        return -1;
//    printf ("seek result: %d\n", res);
    return 0;
}

playItem_t *
cvorbis_insert (playItem_t *after, const char *fname) {
    // check for validity
    FILE *fp = fopen (fname, "rb");
    if (!fp) {
        return NULL;
    }
    OggVorbis_File vorbis_file;
    vorbis_info *vi;
    ov_open (fp, &vorbis_file, NULL, 0);
    vi = ov_info (&vorbis_file, -1);
    if (!vi) { // not a vorbis stream
        return NULL;
    }
    playItem_t *it = malloc (sizeof (playItem_t));
    memset (it, 0, sizeof (playItem_t));
    it->codec = &cvorbis;
    it->fname = strdup (fname);
    it->tracknum = 0;
    it->timestart = 0;
    it->timeend = 0;

    // metainfo
    int title_added = 0;
    vorbis_comment *vc = ov_comment (&vorbis_file, -1);
    if (vc) {
        ps_add_meta (it, "vendor", vc->vendor);
        for (int i = 0; i < vc->comments; i++) {
            if (!strncmp (vc->user_comments[i], "artist=", 7)) {
                ps_add_meta (it, "artist", vc->user_comments[i] + 7);
            }
            else if (!strncmp (vc->user_comments[i], "title=", 6)) {
                ps_add_meta (it, "title", vc->user_comments[i] + 6);
                title_added = 1;
            }
            else if (!strncmp (vc->user_comments[i], "date=", 5)) {
                ps_add_meta (it, "date", vc->user_comments[i] + 5);
            }
        }
    }
    it->filetype = "OggVorbis";
    it->duration = ov_seekable (&vorbis_file) ? ov_time_total (&vorbis_file, -1) : -1;
    if (!title_added) {
        ps_add_meta (it, "title", NULL);
    }
    ov_clear (&vorbis_file);
    after = ps_insert_item (after, it);
    return after;
}

static const char * exts[]=
{
	"ogg",NULL
};

const char **cvorbis_getexts (void) {
    return exts;
}

codec_t cvorbis = {
    .init = cvorbis_init,
    .free = cvorbis_free,
    .read = cvorbis_read,
    .seek = cvorbis_seek,
    .insert = cvorbis_insert,
    .getexts = cvorbis_getexts
};

