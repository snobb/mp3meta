#!/usr/bin/env python
#
# { mp3meta.py }
# Copyright (C) 2013 Alex Kozadaev [akozadaev at yahoo com]
#

import struct, getopt, sys, os

class ID3NotFoundError(Exception):
    pass

class id3v1meta:
    """ id3v1 handler """
    def __init__(self, infile):
        self.__htmpl = "3s30s30s30s4s30s1B"
        self.version = 1.0
        self.title = ""
        self.artist = ""
        self.album = ""
        self.year = 1970
        self.comment = ""
        self.track = -1
        self.tagexists = False
        self._genre = 255
        self._genrelist = [
            "Blues", "Classic Rock", "Country", "Dance",
            "Disco", "Funk", "Grunge", "Hip-Hop", "Jazz",
            "Metal", "New Age", "Oldies", "Other", "Pop",
            "R&B", "Rap", "Reggae", "Rock", "Techno",
            "Industrial", "Alternative", "Ska", "Death Metal",
            "Pranks", "Soundtrack", "Euro-Techno", "Ambient",
            "Trip-Hop", "Vocal", "Jazz+Funk", "Fusion",
            "Trance", "Classical", "Instrumental", "Acid",
            "House", "Game", "Sound Clip", "Gospel", "Noise",
            "AlternRock", "Bass", "Soul", "Punk", "Space",
            "Meditative", "Instrumental Pop",
            "Instrumental Rock", "Ethnic", "Gothic", "Darkwave",
            "Techno-Industrial", "Electronic", "Pop-Folk",
            "Eurodance", "Dream", "Southern Rock", "Comedy",
            "Cult", "Gangsta", "Top 40", "Christian Rap",
            "Pop/Funk", "Jungle", "Native American", "Cabaret",
            "New Wave", "Psychadelic", "Rave", "Showtunes",
            "Trailer", "Lo-Fi", "Tribal", "Acid Punk",
            "Acid Jazz", "Polka", "Retro", "Musical",
            "Rock & Roll", "Hard Rock", "Folk", "Folk-Rock",
            "National Folk", "Swing", "Fast Fusion", "Bebob",
            "Latin", "Revival", "Celtic", "Bluegrass",
            "Avantgarde", "Gothic Rock", "Progressive Rock",
            "Psychedelic Rock", "Symphonic Rock", "Slow Rock",
            "Big Band", "Chorus", "Easy Listening", "Acoustic",
            "Humour", "Speech", "Chanson", "Opera",
            "Chamber Music", "Sonata", "Symphony", "Booty Bass",
            "Primus", "Porn Groove", "Satire", "Slow Jam",
            "Club", "Tango", "Samba", "Folklore", "Ballad",
            "Power Ballad", "Rhythmic Soul", "Freestyle",
            "Duet", "Punk Rock", "Drum Solo", "A capella",
            "Euro-House", "Dance Hall" ]
        if infile:
            try:
                self.read(infile)
            except: pass

    @property
    def genre(self):
        """ return a textual version of the genre """
        if 0 <= self._genre < len(self._genrelist):
            return self._genrelist[self._genre]
        else:
            return "Unknown"

    def read(self, fname):
        """ read the id3v1 tag from a file """
        try:
            self._parsebuf(self._readbuf(fname))
        except ValueError:
            print >> sys.stderr, "error: expected int format"
            exit(1)
        except ID3NotFoundError:
            print >> sys.stderr, "error: id3v1 tag was not found"

    def write(self, fname):
        """ write the id3v1 tag to the file """
        try:
            self._writebuf(fname, self._packbuf())
        except ValueError as e:
            print >> sys.stderr, e.message
            exit(1)

    def _readbuf(self, fname):
        """ read 128 bytes from the end of the file """
        try:
            with open(fname, "rb") as f:
                    f.seek(-128, 2)
                    buf = f.read(128)
        except IOError:
            print >> sys.stderr, ("error: cannot read from file {}"\
                    .format(fname))
            exit(1)
        return buf

    def _writebuf(self, fname, buf):
        try:
            with open(fname, "rb+") as f:
                if (f.read(3) == "ID3"):
                    f.seek(-128, 2)
                else:
                    f.seek(0, 2)
                    f.write(struct.pack("I", 0))
                f.write(buf)
        except IOError:
            print >> sys.stderr, ("error: cannot write to file {}"\
                    .format(fname))

    def _packbuf(self):
        if self.track > 0:
            self.comment = struct.pack("28s1b1b", self.comment,\
                    0, self.track)

        return struct.pack(self.__htmpl, "TAG", self.title,\
                self.artist, self.album, self.year, self.comment,\
                self._genre)

    def _parsebuf(self, buf): # throws ValueError, ID3NotFoundError
        """ parse the 128byte buffer """
        try:
            tag = list(struct.unpack(self.__htmpl, buf))
        except struct.error:
            self._packbuf()
        if str(tag.pop(0)) != "TAG":
            self._packbuf()
        self.tagexists = True
        self.title = tag.pop(0)
        self.artist = tag.pop(0)
        self.album = tag.pop(0)
        self.year = tag.pop(0) # throws ValueError
        self.comment = tag.pop(0)
        if ord(self.comment[28]) == 0:
            self.version = 1.1
            self.track = ord(self.comment[29])
            self.comment.rstrip(self.comment[-2:])
        self._genre = tag.pop(0)

    def __str__(self):
        return ("tag: {}\ntitle: {}\nartist: {}\nalbum: {}\n"
                "year: {}\ntrack: {}\ngenre: {}({})\ncomment: {}"\
                        .format(self.version, self.title,
                            self.artist, self.album, self.year,
                            self.track, self.genre, self._genre,
                            self.comment))

def usage():
    print "Usage: {} [options] <mp3 file>".format(prog)
    print ("    -h      help\n"
           "    -l      list genres\n"
           "    -w      write changes - otherwise no changes\n"
           "    -T      title - max 30\n"
           "    -a      artist - max 30\n"
           "    -b      album - max 30\n"
           "    -y      year - int\n"
           "    -t      track - int\n"
           "    -g      genre - int\n"
           "    -c      comment - max 28-30\n")
    exit(1)

def getterm():
    try:
        rows, columns = os.popen("stty size", "r").read().split()
        return (int(rows), int(columns))
    except:
        return (20, 20)

def fexists(infile):
    try:
        with open(infile, "r"): pass
    except IOError:
        print >> sys.stderr, "error: file does not exist {}"\
                .format(infile)
        return False
    else:
        return True

if __name__ == "__main__":
    towrite, hasfile, infile = False, False, None
    prog = sys.argv.pop(0)
    try:
        opts, f = getopt.getopt(sys.argv, "hwlT:a:b:y:t:g:c:")
        if len(f) and fexists(f[0]):
            hasfile = True
            infile = f[0]
    except getopt.GetoptError:
        print >> sys.stderr, "error: unrecognized option"
        exit(1)

    if len(sys.argv) == 0:
        usage()

    id3 = id3v1meta(infile)

    for k,v in opts:
        if k == "-h":
            usage()
        elif k == "-l":
            rows = getterm()[0] - 1
            for k,v in enumerate(id3._genrelist):
                if k % rows == 0 and k > 0:
                    raw_input("press Enter to continue")
                print "{}: {}".format(k, v)
            exit(0)
        elif hasfile:
            if k == "-T":
                id3.title = v
            elif k == "-a":
                id3.artist = v
            elif k == "-b":
                id3.album = v
            elif k == "-y":
                id3.year = v
            elif k == "-t":
                id3.track = int(v)
            elif k == "-g":
                id3._genre = int(v)
            elif k == "-c":
                id3.comment = v
            elif k == "-w":
                towrite = True

    if hasfile and towrite:
        id3.write(infile)

    if hasfile:
        print id3


# vim: set ts=4 sts=4 sw=4 tw=80 ai smarttab et list
