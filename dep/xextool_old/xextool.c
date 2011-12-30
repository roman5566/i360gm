#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/stat.h>

#include "msvc/stdint.h"

#if !defined(NULL)
#define NULL    0
#endif

#include "./aes/aes.h"
#include "./sha1/sha1.h"
#include "./mspack/mspack.h"
#include "./mspack/system.h"
#include "./mspack/lzx.h"

typedef struct Header
{
    unsigned char Magic[4];
    uint32 ModuleFlags;
    uint32 DataOffset;
    uint32 Reserved;
    uint32 FileHeaderOffset;
    uint32 OptionalHeaderEntries;
} Header;

typedef struct OptionalHeaderEntry
{
    uint32 ID;
    uint32 Data;
} OptionalHeaderEntry;

typedef struct Library
{
    char Name[8];
    unsigned short Version1;
    unsigned short Version2;
    unsigned short Version3;
    unsigned short Version4;
} Library;

struct CompressionInfo
{
    uint32 Reserved;
    uint32 CompressionWindow;
    uint32 Size1;
    unsigned char Hash[20];
    uint32 Size2;
} CompressionInfo;

typedef struct file_header
{
    uint32 offset;
    uint32 value;
    char *desc;
} file_header; 

file_header data[6] =
{
    { 0x00000000, 0x00000000, "module flags",       },
    { 0x00000110, 0x00000000, "load address",       },
    { 0x00000004, 0x00000000, "image size",         },
    { 0x00000178, 0x00000000, "game region",        },
    { 0x0000010C, 0x00000000, "image flags",        },
    { 0x0000017C, 0x00000000, "allowed media types" }
};

typedef struct media_type
{
    uint32 flag;
    char *desc;
} media_type;

media_type types[15] =
{
    { 0x00000001, "hard disk"              },
    { 0x00000002, "DVD-X2"                 },
    { 0x00000004, "DVD/CD"                 },
    { 0x00000008, "DVD-5"                  },
    { 0x00000010, "DVD-9"                  },
    { 0x00000020, "system flash"           },
    { 0x00000080, "memory unit"            },
    { 0x00000100, "mass storage device"    },
    { 0x00000200, "SMB filesystem"         },
    { 0x00000400, "direct-from-RAM"        },
    { 0x01000000, "insecure package"       },
    { 0x02000000, "save game package"      },
    { 0x04000000, "locally signed package" },
    { 0x08000000, "Live-signed package"    },
    { 0x10000000, "Xbox platform package"  }
};

typedef struct info_type
{
    uint32 ID;
    char *desc;
    void* (*f)(FILE*, OptionalHeaderEntry *);
} info_type;

static void* decodeString(FILE*, OptionalHeaderEntry *);
static void* decodeLibraries(FILE*, OptionalHeaderEntry *);
static void* decodeExecutionIDs(FILE*, OptionalHeaderEntry *);
static void* decodeCompression(FILE*, OptionalHeaderEntry *);
static void* decodeSystemImportLibraries(FILE*, OptionalHeaderEntry *);

static info_type infoTable[23] =
{
    { 0x00010201, "image base address",              NULL              },
    { 0x00010001, "original base address",           NULL              },
    { 0x00010100, "entry point",                     NULL              },
    { 0x00018002, "image checksum, image timestamp", NULL              },
    { 0x00020104, "TLS stuff",                       NULL              },
    { 0x00020200, "default stack size",              NULL              },
    { 0x00020301, "default file system cache size",  NULL              },
    { 0x00020401, "default heap size",               NULL              },
    { 0x00040201, "title workspace size",            NULL              },
    { 0x00018102, "image is enabled for callcap",    NULL              },
    { 0x00018200, "image is enabled for fastcap",    NULL              },
    { 0x00E10402, "image includes export by name",   NULL              },
    { 0x000080FF, "Bound pathname",                  decodeString      },
    { 0x00040310, "Image game rating specified",     NULL              },
    { 0x00040404, "LAN Key",                         NULL              },
    { 0x00040006, "EXECUTION ID",                    NULL              },
    { 0x000103FF, "OTHER IMPORT LIBRARIES",          NULL              },
    { 0x000200FF, "Library Versions",                decodeLibraries   },
    { 0x000002FF, "Resource Section",                NULL              },
    { 0x000003FF, "decompression information",       decodeCompression },
    { 0x00030000, "System Flags",                    NULL              },
    { 0x000405FF, "???",                             NULL              },
    { 0x000183FF, "???2",                            NULL              }
};

static unsigned char LCT[16];      /* last ciphertext block */

uint32 swap(unsigned char *p)
{
    uint32 l;
    *(uint32 *)p = l = ((uint32)p[0]<<24)|((uint32)p[1]<<16)|((uint32)p[2]<<8)|(uint32)p[3];
    return l;
}

void swapblock(void* d, int s, int o)
{
    int l;
    for(l=0; l<(s-o); l+=4)
    {
        swap(((unsigned char *)d)+o+l);
    }
}

void fromFile(FILE *fd, void* d, int s, int o)
{
    fread(d,1,s,fd);
    swapblock(d, s, o);
}

char * pad(char * c, int l)
{
    int i;
    static char str[255];

    for(i=0; i<l; i++)
        if( *c != 0 )
            str[i] = *c++;
        else
            str[i] = ' ';

    str[i] = 0;

    return str;
}

static void* decodeString(FILE *fd, OptionalHeaderEntry *entry)
{
    static char str[255];
    uint32 s;

    fseek(fd,entry->Data,SEEK_SET);
    fromFile(fd, &s, 4, 0);

    memset(str, 0, sizeof(str));

    fread(str,1,s,fd);
    printf("    %s\n", str);

    return (void *)str;
}

static void* decodeLibraries(FILE *fd, OptionalHeaderEntry *entry)
{
    char a;
    uint32 s, i;
    Library l;

    fseek(fd,entry->Data,SEEK_SET);
    fromFile(fd, &s, 4, 0);

    for(i=0; i<s/sizeof(l); i++)
    {
        fromFile(fd, &l, sizeof(l), 8);
        if(l.Version4 & 0x8000)
            a = 0;
        else
            a = 1;

        printf("    [\"%s\", %d.%d.%d.%d (%s)]\n", pad(l.Name, 8), l.Version1, l.Version2, l.Version3, l.Version4 &~0x8000, a?"approved":"unapproved" );
    }
    return NULL;
}

static void* decodeExecutionIDs(FILE *fd, OptionalHeaderEntry *entry)
{
    fd=fd; // get rid of warning (will be optimized out)
    entry=entry; // get rid of warning (will be optimized out)
    return 0;
}

static void* decodeCompression(FILE *fd, OptionalHeaderEntry *entry)
{
    uint32 s;

    fseek(fd,entry->Data,SEEK_SET);
    fromFile(fd, &s, 4, 0);

    if( s != 0x24 )
    {
        printf("    ???\n");
        return NULL;
    }

    fromFile(fd, &CompressionInfo, sizeof(CompressionInfo), 0);
    swapblock(CompressionInfo.Hash, 20, 0);

    printf("    Reserved: 0x%08X\n",CompressionInfo.Reserved);
    printf("    CompressionWindow: 0x%08X\n",CompressionInfo.CompressionWindow);
    printf("    Size1: 0x%08X\n",CompressionInfo.Size1);
    printf("    Hash:");
    for(s=0; s<sizeof(CompressionInfo.Hash); s++)
        printf(" %02X", CompressionInfo.Hash[s]);

    printf("\n    Size2: 0x%08X\n",CompressionInfo.Size2);

    return NULL;
}

static void* decodeSystemImportLibraries(FILE *fd, OptionalHeaderEntry *entry)
{
    uint32 s;

    fseek(fd,entry->Data,SEEK_SET);
    fromFile(fd, &s, 4, 0);

    return NULL;
}

void aes_dec_cbc(aes_context *ctx, unsigned char *buffer, int blk_len)
{
    int i, j;
    unsigned char temp[16];

    for( i = 0; i < blk_len; i += 16 )
    {
        memcpy( temp, &buffer[i], 16 );

        aes_decrypt( ctx, &buffer[i], &buffer[i] );

        for( j = 0; j < 16; j++ )
            buffer[i + j] ^= LCT[j];

        memcpy( LCT, temp, 16 );
     }
}


int main(int argc, char* argv[])
{
    unsigned char *buf, *lzx_data;
    FILE *fd, *fc;
    uint32 i, j, s;
    unsigned short slen;
    Header hdr;
    OptionalHeaderEntry entry;
    aes_context ctx_aes;
    sha1_context ctx_sha1;
    uint32 block_size;
    unsigned char block_hash[20], block_hash_calculated[20];
    unsigned char aes_key[16], aes_key_encrypted[16];

    struct mspack_system *sys = mspack_default_system;

    struct mspack_file *lzxinput, *lzxoutput;
    struct lzxd_stream *lzxd;

//    static unsigned char buffer[16];
    static unsigned char output[16];

    if ( argc < 3 )
    {
        printf("Usage: %s in.xex out.pe\n",argv[0]);
        return 1;
    }

    fd = fopen(argv[1], "rb");
    if (fd == NULL)
    {
        perror(argv[1]);
        return 1;
    }

    fromFile(fd, &hdr, sizeof(hdr), 4);

    printf("Magic: '%s'\n", pad(hdr.Magic,4));
    printf("ModuleFlags: %u\n", hdr.ModuleFlags);
    printf("DataOffset: %u\n", hdr.DataOffset);
    printf("Reserved:  %u\n", hdr.Reserved);
    printf("FileHeaderOffset: %u\n", hdr.FileHeaderOffset);
    printf("OptionalHeaderEntries: %u\n", hdr.OptionalHeaderEntries);

    if(!!memcmp(hdr.Magic, "XEX2", 4))
    {
        printf("%s: Not a XEX2 executable!", argv[1]);
        return 1;
    }

    s = ftell(fd);

    printf("FILE HEADER\n");
    for(i=0; i<6; i++)
    {
        fseek(fd,data[i].offset+hdr.FileHeaderOffset,SEEK_SET);
        fromFile(fd, &data[i].value, 4, 0);
        printf("%s: 0x%08X\n", data[i].desc, data[i].value);
    }

    for(i=0; i<15; i++)
        if( data[5].value & types[i].flag )
            printf("    %s\n", types[i].desc);

    fseek(fd,s,SEEK_SET);

    printf("OPTIONAL HEADER VALUES\n");

    for(i=0; i<hdr.OptionalHeaderEntries; i++)
    {
        fromFile(fd, &entry, sizeof(entry), 0);

        for(j=0; j<23; j++)
            if(entry.ID == infoTable[j].ID)
                break;

        printf("0x%08X %s 0x%X\n", entry.ID, infoTable[j].desc, entry.Data);

        s = ftell(fd);

        if( infoTable[j].f != NULL )
            infoTable[j].f(fd,&entry);

        fseek(fd,s,SEEK_SET);
    }

    // -----------------

    fseek(fd, hdr.FileHeaderOffset + 0x150, SEEK_SET);
    fread(aes_key_encrypted,1, 16,fd);

    memset(&ctx_aes, 0, sizeof(ctx_aes));
    
    memset(&aes_key, 0, 16);

    aes_set_key(&ctx_aes, aes_key, 128);
    aes_decrypt(&ctx_aes, aes_key_encrypted, aes_key);

    printf("session key:");
    for(i=0; i<sizeof(aes_key); i++)
        printf(" %02X", aes_key[i]);

    memset(&ctx_aes, 0, sizeof(ctx_aes));
    aes_set_key(&ctx_aes, aes_key, 128);

    fseek(fd, hdr.DataOffset, SEEK_SET);

    //------------

    // # starting from here, this only works for compressed files.

    fc = fopen("compressed_data", "wb" );
    if (fc == NULL)
    {
        perror("compressed_data");
        return 1;
    }

    block_size = CompressionInfo.Size1;
    memcpy(block_hash, CompressionInfo.Hash, 20);
    
    // # the format is a multiple of the following:
    // # |SIZE(next block) HASH(next block) DATA |
    // # the first size, hash is given in the "compressionInfo" in the file header
    // # every block contains the size&hash of the NEXT block
    // # thus, for unpacking, you can check them in order
    // # for packing, you have to start hashing on the last block, of course.

    while(block_size)
    {
        buf = malloc(block_size);
        fread(buf,1, block_size,fd);

        aes_dec_cbc(&ctx_aes, buf, block_size);
        
        sha1_starts(&ctx_sha1);
        sha1_update(&ctx_sha1, buf, block_size);
        sha1_finish(&ctx_sha1, block_hash_calculated);

        if(!!memcmp(block_hash, block_hash_calculated, 20))
        {
            printf("\ncompressed data is corrupt!!!\n");
            break;
        }
        else
        {
            printf("\ncompressed block with size %u -> ",block_size);
        }

        block_size = swap((unsigned char *)buf);
        memcpy(block_hash, buf+4, 20);

        s = 0;
        lzx_data = buf+24;
        while(1)
        {
            s += slen = (lzx_data[0] << 8) | lzx_data[1];

            if(slen == 0)
                break;

            fwrite(lzx_data+2,1, slen,fc);

            lzx_data = &lzx_data[slen+2];
        }

        printf("%u",s);


        free(buf);
    }

    fclose(fc);
    fclose(fd);

    //------------

    lzxinput = sys->open(sys, "compressed_data", MSPACK_SYS_OPEN_READ);
    if (!lzxinput)
    {
        perror("compressed_data");
        return 1;
    }

    lzxoutput = sys->open(sys, argv[2], MSPACK_SYS_OPEN_WRITE);
    if (!output)
    {
        perror(argv[2]);
        return 1;
    }

    lzxd = lzxd_init(sys, lzxinput, lzxoutput, 15, 0, 100*1024*1024, 0);

    if (!lzxd)
        printf("lzxd_init failed\n");

    printf("\ndecompress result: %d\n", lzxd_decompress(lzxd, 100*1024*1024));

    lzxd_free(lzxd);

    return 0;
}
