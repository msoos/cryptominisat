#include "File.h"

void File::open(int file_descr, FileMode m, bool own)
{
    if (fd != -1) ::close(fd);
    fd     = file_descr;
    mode   = m;
    own_fd = own;
    pos    = 0;
    buf    = xmalloc<uchar>(File_BufSize);
    if (mode == READ) size = read(fd, buf, File_BufSize);
    else              size = -1;
}

void File::open(cchar* name, cchar* mode_)
{
    if (fd != -1) ::close(fd);
    bool    has_r = strchr(mode_, 'r') != NULL;
    bool    has_w = strchr(mode_, 'w') != NULL;
    bool    has_a = strchr(mode_, 'a') != NULL;
    bool    has_p = strchr(mode_, '+') != NULL;
    assert(!(has_r && has_w));
    assert(has_r || has_w || has_a);

    int mask = 0;
    if      (has_p) mask |= O_RDWR;
    else if (has_r) mask |= O_RDONLY;
    else            mask |= O_WRONLY;

    if (!has_r) mask |= O_CREAT;
    if (has_w)  mask |= O_TRUNC;

    fd = open64(name, mask, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);

    if (fd != -1){
        mode   = has_r ? READ : WRITE;
        own_fd = true;
        pos    = 0;
        if (has_a) lseek64(fd, 0, SEEK_END);
        buf    = xmalloc<uchar>(File_BufSize);
        if (mode == READ) size = read(fd, buf, File_BufSize);
        else              size = -1;
    }
}


void File::close(void)
{
    if (fd == -1) return;
    if (mode == WRITE)
        flush();
    xfree(buf); buf = NULL;
    if (own_fd)
        ::close(fd);
    fd = -1;
}

void File::seek(int64 file_pos, int whence)
{
    if (mode == WRITE){
        flush();
        pos = 0;
        lseek64(fd, file_pos, whence);
    }else{
        if (whence == SEEK_CUR) lseek64(fd, file_pos - (size - pos), SEEK_CUR);
        else                    lseek64(fd, file_pos, whence);
        size = read(fd, buf, File_BufSize);
        pos = 0;
    }
}

int64 File::tell(void)
{
    if (mode == WRITE)
        return lseek64(fd, 0, SEEK_CUR);
    else
        return lseek64(fd, 0, SEEK_CUR) - (size - pos);
}


//=================================================================================================
// Marshaling:


void putUInt(File& out, uint64 val)
{
    if (val < 0x20000000){
        uint    v = (uint)val;
        if (v < 0x80)
            out.putChar(v);
        else{
            if (v < 0x2000)
                out.putChar(0x80 | (v >> 8)),
                out.putChar((uchar)v);
            else if (v < 0x200000)
                out.putChar(0xA0 | (v >> 16)),
                out.putChar((uchar)(v >> 8)),
                out.putChar((uchar)v);
            else
                out.putChar((v >> 24) | 0xC0),
                out.putChar((uchar)(v >> 16)),
                out.putChar((uchar)(v >> 8)),
                out.putChar((uchar)v);
        }
    }else
        out.putChar(0xE0),
        out.putChar((uchar)(val >> 56)),
        out.putChar((uchar)(val >> 48)),
        out.putChar((uchar)(val >> 40)),
        out.putChar((uchar)(val >> 32)),
        out.putChar((uchar)(val >> 24)),
        out.putChar((uchar)(val >> 16)),
        out.putChar((uchar)(val >> 8)),
        out.putChar((uchar)val);
}


uint64 getUInt(File& in)    // Returns 0 at end-of-file.
{
    uint byte0, byte1, byte2, byte3, byte4, byte5, byte6, byte7;
    byte0 = in.getChar();
    if (byte0 == (uint)EOF) return 0;
    if (!(byte0 & 0x80))
        return byte0;
    else{
        switch ((byte0 & 0x60) >> 5){
        case 0:
            byte1 = in.getChar();
            return ((byte0 & 0x1F) << 8) | byte1;
        case 1:
            byte1 = in.getChar();
            byte2 = in.getChar();
            return ((byte0 & 0x1F) << 16) | (byte1 << 8) | byte2;
        case 2:
            byte1 = in.getChar();
            byte2 = in.getChar();
            byte3 = in.getChar();
            return ((byte0 & 0x1F) << 24) | (byte1 << 16) | (byte2 << 8) | byte3;
        case 3:
            byte0 = in.getChar();
            byte1 = in.getChar();
            byte2 = in.getChar();
            byte3 = in.getChar();
            byte4 = in.getChar();
            byte5 = in.getChar();
            byte6 = in.getChar();
            byte7 = in.getChar();
            return ((uint64)((byte0 << 24) | (byte1 << 16) | (byte2 << 8) | byte3) << 32)
                 |  (uint64)((byte4 << 24) | (byte5 << 16) | (byte6 << 8) | byte7);
        }
        assert(false);
    }
}
