#ifndef __FILEUTIL_H__
#define __FILEUTIL_H__
#include <string>

/* Returns the file name for a table, e.g. /path/to/RRvNN.egt */
string getFileNameForCombo(const char *combo);

/* Returns the file name for a compressed table, e.g. /path/to/RRvNN.egt.xz */
string getCompressedFileNameForCombo(const char *combo);

/* Returns the file name for a compressed table index, e.g. /path/to/RRvNN.idx */
string getIndexFileNameForCombo(const char *combo);

/* Returns true iff the file exists) */
bool fileExists(const char *fileName);

/* Returns the size of the specified file, in bytes */
unsigned getFileSize(const char *fileName);

/* Log a note of interesting events during EGTB generation / probing */
void appendEgtbNote(const char *note, const char *combo);

/**
 * Compress a file and build a block index so we have block access at decompression time.
 * DISCLAIMER: I use LZMA for compression / decompression, but my implementation is hacky.
 * I did not understand LZMA's block indexing mechanism, in particular decoding the index so I can seek to the wanted block.
 * In the interest of living the rest of my life, I gave up trying.
 * The code silently assumes the XZ header is 12 bytes and builds its own index file.
 * Bad engineer! Bad!
 **/
void compressFile(const char *name, const char *compressed, const char *index, int blockSize, bool removeOriginal);

/**
 * Returns the blockNum block (0-based) from the compressed file.
 * The block size was fixed at compression and should match EGTB_CHUNK_SIZE.
 * Returns NULL if the compressed or index files are missing or corrupt.
 **/
char* decompressBlock(const char *compressed, const char *index, int blockNum);

/**
 * Encodes x to a 7-bit variable-length quantity and writes it to f.
 */
void writeVlq(u64 x, FILE* f);

/**
 * Reads a 7-bit encoded variable-length quantity from f.
 */
u64 readVlq(FILE* f);

#endif
