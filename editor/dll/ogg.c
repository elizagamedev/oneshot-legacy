#include "ogg.h"

#define SIZE_SNDFILE_BUFFER 2048
#define SIZE_BLOCK_BUFFER   4096
#define SIZE_POINTER_BUFFER (1024)

static sf_count_t wav_size = 0;
static sf_count_t wav_pos = 0;
static int wav_data_nAllocs = 0;
static void** wav_data = NULL;

sf_count_t
wav_vio_filelen(void* data)
{
    return wav_size;
}

sf_count_t
wav_vio_seek(sf_count_t offset, int whence, void* data)
{
    switch(whence)
    {
    case SEEK_CUR:
        return (wav_pos += offset);
    case SEEK_SET:
        return (wav_pos = offset);
    case SEEK_END:
        return (wav_pos = wav_size + offset);
    }
    return 0;
}

sf_count_t
wav_vio_read(void* ptr, sf_count_t count, void* data)
{
    sf_count_t max;
    if(wav_pos + count > wav_size)
        max = wav_size - wav_pos;
    else
        max = count;

    sf_count_t nRead = 0;

    while(nRead < max)
    {
        int blkmax = SIZE_BLOCK_BUFFER - wav_pos % SIZE_BLOCK_BUFFER;
        if(wav_pos + blkmax > max)
            blkmax = max - wav_pos;
        memcpy(ptr + nRead, wav_data[wav_pos / SIZE_BLOCK_BUFFER] + wav_pos % SIZE_BLOCK_BUFFER, blkmax);
        wav_pos += blkmax;
        nRead += blkmax;
    }
    return nRead;
}

sf_count_t
wav_vio_write(const void* ptr, sf_count_t count, void* data)
{
    if(wav_pos + count > wav_size)
        wav_size = wav_pos + count;

    if(!wav_data)
    {
        wav_data = malloc(SIZE_POINTER_BUFFER * sizeof(void*));
        memset(wav_data, 0, SIZE_POINTER_BUFFER * sizeof(void*));
        wav_data_nAllocs = 1;
    }

    while(wav_size / SIZE_BLOCK_BUFFER >= SIZE_POINTER_BUFFER * wav_data_nAllocs)
    {
        wav_data = realloc(wav_data, SIZE_POINTER_BUFFER * (wav_data_nAllocs + 1) * sizeof(void*));
        memset(&wav_data[SIZE_POINTER_BUFFER * wav_data_nAllocs], 0, SIZE_POINTER_BUFFER * sizeof(void*));
        wav_data_nAllocs++;
    }

    sf_count_t nWritten = 0;

    while(nWritten < count)
    {
        if(!wav_data[wav_pos / SIZE_BLOCK_BUFFER])
            wav_data[wav_pos / SIZE_BLOCK_BUFFER] = malloc(SIZE_BLOCK_BUFFER);

        int blkmax = SIZE_BLOCK_BUFFER - wav_pos % SIZE_BLOCK_BUFFER;
        if(blkmax > wav_size - wav_pos)
            blkmax = wav_size - wav_pos;
        memcpy(wav_data[wav_pos / SIZE_BLOCK_BUFFER] + wav_pos % SIZE_BLOCK_BUFFER, ptr + nWritten, blkmax);
        wav_pos += blkmax;
        nWritten += blkmax;
    }
    return count;
}

sf_count_t
wav_vio_tell(void* data)
{
    return wav_pos;
}

SF_VIRTUAL_IO sfio =
{
    wav_vio_filelen,
    wav_vio_seek,
    wav_vio_read,
    wav_vio_write,
    wav_vio_tell,
};

void
ogg_read(LPSTR szFile)
{
    SNDFILE* ogg = NULL;
    SNDFILE* wav = NULL;
    SF_INFO info;

    float buffer[SIZE_SNDFILE_BUFFER];
    sf_count_t count;

    //Set up the ogg and wav SNDFILEs
    //OGG
    if(!(ogg = sf_open(szFile, SFM_READ, &info)))
        goto end;

    //WAV
    info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    if(!sf_format_check(&info))
        goto end;

    if(!(wav = sf_open_virtual(&sfio, SFM_WRITE, &info, NULL)))
        goto end;

    //Write to the wav
    while((count = sf_read_float(ogg, buffer, SIZE_SNDFILE_BUFFER)) > 0)
        sf_write_float(wav, buffer, count);

end:
    if(ogg)
        sf_close(ogg);
    if(wav)
        sf_close(wav);

    //Seek to the beginning
    wav_pos = 0;
}

void
ogg_free()
{
    if(wav_data)
    {
        int i = 0;
        for(; wav_data[i]; i++)
            free(wav_data[i]);
    }
    free(wav_data);
    wav_data = NULL;
    wav_size = 0;
    wav_pos = 0;
}
