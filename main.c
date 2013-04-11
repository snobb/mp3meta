/*  main.c  */
/*  Copyright (C) 2013 Alex Kozadaev [akozadaev at yahoo com]  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

/* ========================================================================= */
#define ID3VER(a)         ((a)->comment.czt.zr == 0) ? 1.1f : 1.0f
#define ID3TXTGENRE(a)    ((a)->genre >= 0 && ((a)->genre <= 125)) ?\
                                         genre[(int)(a)->genre] : "Unknown"
#define GETTAG(d, s)      memcpy((d), (s), sizeof(s))
#define SETTAG(d, s)      if (s) {\
                              size_t sz = strlen(s);\
                              memcpy((d), (s), sizeof(d) > (sz) ?\
                              (sz) : sizeof(d));}
#define ID3SZ             128

struct id3meta {
    char header[3];
    char title[30];
    char artist[30];
    char album[30];
    char year[4];
    union {
        struct {
            char c[28];
            char zr;
            char tr;
        } czt;
        char c[30];
    } comment;
    unsigned char genre;
};

enum { WRITE = 0x01, HASFILE = 0x02 };  /* options flags */

struct options {
    char flags;
    char *fname;
    char *title;
    char *artist;
    char *album;
    char *year;
    char *comment;
    int track;
    int genre;
};

void read_args(int argc, char **argv, struct options *opts);
bool is_mp3(FILE *fp);
void *readbuf(FILE *fp, void *buf, size_t sz, long offset, int start);
void writebuf(FILE *fp, const void *buf, size_t sz, long offset, int start);
void updatebuf(struct id3meta *mp3meta, const struct options *opts);
void printid3v1(const struct id3meta *mp3);
void die(const char *fmt, ...);
void usage(const char *prog);

char *genre[] = {
    "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk",
    "Grunge", "Hip-Hop", "Jazz", "Metal", "New Age", "Oldies",
    "Other", "Pop", "R&B", "Rap", "Reggae", "Rock", "Techno",
    "Industrial", "Alternative", "Ska", "Death Metal", "Pranks",
    "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop", "Vocal",
    "Jazz+Funk", "Fusion", "Trance", "Classical", "Instrumental",
    "Acid", "House", "Game", "Sound Clip", "Gospel", "Noise",
    "AlternRock", "Bass", "Soul", "Punk", "Space", "Meditative",
    "Instrumental Pop", "Instrumental Rock", "Ethnic", "Gothic",
    "Darkwave", "Techno-Industrial", "Electronic", "Pop-Folk",
    "Eurodance", "Dream", "Southern Rock", "Comedy", "Cult",
    "Gangsta", "Top 40", "Christian Rap", "Pop/Funk", "Jungle",
    "Native American", "Cabaret", "New Wave", "Psychadelic",
    "Rave", "Showtunes", "Trailer", "Lo-Fi", "Tribal", "Acid Punk",
    "Acid Jazz", "Polka", "Retro", "Musical", "Rock & Roll",
    "Hard Rock", "Folk", "Folk-Rock", "National Folk", "Swing",
    "Fast Fusion", "Bebob", "Latin", "Revival", "Celtic",
    "Bluegrass", "Avantgarde", "Gothic Rock", "Progressive Rock",
    "Psychedelic Rock", "Symphonic Rock", "Slow Rock", "Big Band",
    "Chorus", "Easy Listening", "Acoustic", "Humour", "Speech",
    "Chanson", "Opera", "Chamber Music", "Sonata", "Symphony",
    "Booty Bass", "Primus", "Porn Groove", "Satire", "Slow Jam",
    "Club", "Tango", "Samba", "Folklore", "Ballad", "Power Ballad",
    "Rhythmic Soul", "Freestyle", "Duet", "Punk Rock", "Drum Solo",
    "A capella", "Euro-House", "Dance Hall", NULL
};


/* ========================================================================= */
int main(int argc, char **argv)
{
    FILE *mp3fp;
    struct id3meta mp3meta;
    struct options opts = { 0, NULL, NULL, NULL, NULL, NULL, NULL, -1, 257 };

    if (argc == 1)
        usage(argv[0]);
    read_args(argc, argv, &opts);

    if (!(opts.flags & HASFILE))
        die("error: arguments error - no file");

    if ((mp3fp = fopen(opts.fname, "r+")) == NULL)
        die("error: cannot open the file %s", opts.fname);

    if (!is_mp3(mp3fp))
        die("error: the file is not an MP3 file");

    readbuf(mp3fp, &mp3meta, ID3SZ, -ID3SZ, SEEK_END);
    updatebuf(&mp3meta, &opts);

    if (opts.flags & (WRITE | HASFILE))
        writebuf(mp3fp, &mp3meta, ID3SZ, -ID3SZ, SEEK_END);

    fclose(mp3fp);

    printid3v1(&mp3meta);

    return 0;
}

/* ========================================================================= */
void read_args(int argc, char **argv, struct options *opts)
{
    char *arg, **list, *prog = *argv;

    argc--;
    if (argc == 0)
        usage(prog);

    while (*++argv != NULL) {
        arg = *argv;
        if (*arg == '-') {
            while (*++arg != '\0') {
                /* TODO: add error checking in the arguments processing */
                switch(*arg) {
                    case 'h':
                        usage(prog); break;
                    case 'l':
                        list = genre;
                        for (int i = 0; *list != NULL; i++)
                            printf("%d: %s\n", i, *list++);
                        break;
                    case 'w':
                        opts->flags |= WRITE; break;
                    case 'T':
                        opts->title = *++argv; break;
                    case 'a':
                        opts->artist = *++argv; break;
                    case 'b':
                        opts->album = *++argv; break;
                    case 'y':
                        opts->year = *++argv; break;
                    case 't':
                        opts->track = atoi(*++argv); break;
                    case 'c':
                        opts->comment = *++argv; break;
                    case 'g':
                        opts->genre = atoi(*++argv); break;
                    default:
                        printf("error: unrecognized option \"-%c\"", *arg);
                        usage(prog);
                }
            }
        } else {
            opts->flags |= HASFILE;
            opts->fname = arg;
        }
    }
}

/* ========================================================================= */
bool is_mp3(FILE *fp)
{
    char id[3];
    bool is_mp3 = false;
    rewind(fp);
    if (fread(id, 3, 1, fp) == 0)
        return false; /* false - not a valid mp3 file */

    is_mp3 = (strncmp(id, "ID3", 3) == 0);
    /* no need to rewind since we shall move the pointer anyway */
    /* rewind(fp); */
    return is_mp3;
}

/* ========================================================================= */
void *readbuf(FILE *fp, void *buf, size_t sz, long offset, int start)
{
    fseek(fp, offset, start);

    if (fread(buf, ID3SZ, 1, fp) != 1)
        die("error: cannot read data");

    return buf;
}

/* ========================================================================= */
void writebuf(FILE *fp, const void *buf, size_t sz, long offset, int start)
{
    fseek(fp, offset, start);
    if (fwrite(buf, ID3SZ, 1, fp) != 1)
        die("error: cannot write data");
}

/* ========================================================================= */
void updatebuf(struct id3meta *mp3meta, const struct options *opts)
{
    SETTAG(mp3meta->title, opts->title);
    SETTAG(mp3meta->artist, opts->artist);
    SETTAG(mp3meta->album, opts->album);
    SETTAG(mp3meta->year, opts->year);
    SETTAG(mp3meta->artist, opts->artist);
    SETTAG(mp3meta->comment.c, opts->comment);
    if (opts->track > 0 && mp3meta->comment.czt.tr > 0) {
        mp3meta->comment.czt.zr = 0;
        mp3meta->comment.czt.tr = opts->track;
    }
    if (opts->genre < 0xff) {
        mp3meta->genre = opts->genre;
    }
}

/* ========================================================================= */
void printid3v1(const struct id3meta *mp3meta)
{
    char str[31] = { 0 };
    if (strncmp(mp3meta->header, "TAG", 3) != 0)
        die("error: id3v1 tag isn't found");

    printf("id3 version: %1.1f\n", ID3VER(mp3meta));
    printf("title: %s\n", GETTAG(str, mp3meta->title));
    printf("artist: %s\n", GETTAG(str, mp3meta->artist));
    printf("album: %s\n", GETTAG(str, mp3meta->album));

    str[sizeof(mp3meta->year)] = '\0';
    printf("year: %s\n", GETTAG(str, mp3meta->year));

    if (ID3VER(mp3meta) > 1) {
        printf("track: %d\n", mp3meta->comment.czt.tr);
        GETTAG(str, mp3meta->comment.czt.c);
        str[sizeof(mp3meta->comment.czt.c)] = '\0';
        printf("comment: %s\n", str);
    } else {
        printf("comment: %s\n", GETTAG(str, mp3meta->comment.c));
    }

    printf("genre: %s(%u)\n", ID3TXTGENRE(mp3meta),
           (unsigned char)mp3meta->genre);
}

/* ========================================================================= */
void die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
    exit(1);
}

/* ========================================================================= */
void usage(const char *prog)
{
    printf("Usage: %s [options] <mp3 file>", prog);
    puts("    -h      help\n"
         "    -l      list genres\n"
         "    -w      write changes - otherwise no changes\n"
         "    -T      title - max 30\n"
         "    -a      artist - max 30\n"
         "    -b      album - max 30\n"
         "    -y      year - int\n"
         "    -t      track - int\n"
         "    -g      genre - int\n"
         "    -c      comment - max 28-30\n");
    exit(1);
}

/* vim: ts=4 sts=8 sw=4 smarttab et si tw=80 ci cino+=t0(0 list */

