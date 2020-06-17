#include <assert.h>
#include <lzma.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include "bitmanip.h"
#include "configfile.h"
#include "defines.h"
#include "fileutil.h"
#include "logging.h"
#include "precomp.h"

string getFileNameForCombo(const char *combo) {
  return cfgEgtbPath + "/" + combo + ".egt";
}

string getCompressedFileNameForCombo(const char *combo) {
  return cfgEgtbPath + "/" + combo + ".egt.xz";
}

string getIndexFileNameForCombo(const char *combo) {
  return cfgEgtbPath + "/" + combo + ".idx";
}

bool fileExists(const char *fileName) {
  return !access(fileName, F_OK);
}

unsigned getFileSize(const char *fileName) {
  struct stat st;
  stat(fileName, &st);
  return st.st_size;
}

void appendEgtbNote(const char *note, const char *combo) {
  string fileName = string(cfgEgtbPath) + "/notes.txt";
  FILE *f = fopen(fileName.c_str(), "at");
  fprintf(f, "Combo %s: %s\n", combo, note);
  fclose(f);
}

/* Encodes at most blockSize bytes from fin to fout. Returns the size of the encoded block. */
int lzmaEncodeWrapper(lzma_stream *strm, FILE *fin, FILE *fout, int blockSize) {
  uint8_t in[blockSize];
  uint8_t out[blockSize];
  int result = 0;

  strm->avail_in = fread(in, 1, blockSize, fin);
  strm->next_in = in;

  do {
    strm->next_out = out;
    strm->avail_out = blockSize;
    lzma_ret ret = lzma_code(strm, LZMA_FULL_FLUSH);
    assert (ret == LZMA_OK || ret == LZMA_STREAM_END);
    result += blockSize - strm->avail_out;
    fwrite(out, 1, blockSize - strm->avail_out, fout);
  } while (!strm->avail_out);

  return result;
}

void lzmaEncodeEnd(lzma_stream *strm, FILE *fout, int blockSize) {
  uint8_t out[blockSize];

  do {
    strm->next_out = out;
    strm->avail_out = blockSize;
    lzma_ret ret = lzma_code(strm, LZMA_FINISH);
    assert(ret == LZMA_OK || ret == LZMA_STREAM_END);
    fwrite(out, 1, blockSize - strm->avail_out, fout);
  } while (!strm->avail_out);

  lzma_end(strm);
}

void compressBlocks(FILE *fin, FILE *fout, FILE *fidx, int blockSize) {
  // Configure and initialize the encoder
  lzma_stream strm = LZMA_STREAM_INIT;
	lzma_options_lzma opt_lzma;
	assert(!lzma_lzma_preset(&opt_lzma, 6));

  // A preset means only one filter
	lzma_filter filters[LZMA_FILTERS_MAX + 1];
	filters[0].id = LZMA_FILTER_LZMA2;
	filters[0].options = &opt_lzma;
	filters[1].id = LZMA_VLI_UNKNOWN;
	assert(lzma_stream_encoder(&strm, filters, LZMA_CHECK_CRC32) == LZMA_OK);

  unsigned offset = 0;  // Keep track of current place in fout
  unsigned block0 = 12; // Block 0 starts on byte 12, after the header
  fwrite(&block0, sizeof(unsigned), 1, fidx);
  while (!feof(fin)) {
    int size = lzmaEncodeWrapper(&strm, fin, fout, blockSize);
    if (size) {
      offset += size;
      fwrite(&offset, sizeof(unsigned), 1, fidx);
    }
  }
  lzmaEncodeEnd(&strm, fout, blockSize);
}

void compressFile(const char *name, const char *compressed, const char *index, int blockSize, bool removeOriginal) {
  FILE *fin = fopen(name, "rb");
  if (!fin) {
    log(LOG_WARNING, "File %s does not exist, so not compressing", name);
    return;
  }
  log(LOG_INFO, "Compressing %s to %s and %s", name, compressed, index);
  FILE *fout = fopen(compressed, "wb");
  FILE *fidx = fopen(index, "wb");
  compressBlocks(fin, fout, fidx, blockSize);
  fclose(fin);
  fclose(fout);
  fclose(fidx);
  if (removeOriginal) {
    log(LOG_INFO, "Removing uncompressed file %s", name);
    unlink(name);
  }
}

/* Decodes a block starting at the current position in fin. Assumes that outSize bytes are sufficient to hold all the data. */
void decodeBlock(lzma_stream *strm, FILE *fin, char *out, int outSize, int compressedLength) {
	uint8_t in[EGTB_CHUNK_SIZE];

  strm->next_in = in;
  strm->avail_in = fread(in, 1, compressedLength, fin);
  strm->next_out = (uint8_t*)out;
  strm->avail_out = EGTB_CHUNK_SIZE;

  while (strm->avail_in) {
		lzma_ret ret = lzma_code(strm, LZMA_RUN);
		assert(ret == LZMA_OK || ret == LZMA_STREAM_END);
	}
}

char* decompressBlock(const char *compressed, const char *index, int blockNum) {
  FILE *fin = fopen(compressed, "rb");
  FILE *fidx = fopen(index, "rb");
  if (!fin || !fidx) {
    return NULL;
  }

  unsigned offset[2]; // Offset of our block and the next
  fseek(fidx, sizeof(unsigned) * blockNum, SEEK_SET);
  assert(fread(&offset, sizeof(unsigned), 2, fidx) == 2);
  fclose(fidx);

  // Configure and initialize the decoder
  lzma_stream strm = LZMA_STREAM_INIT;
	assert(lzma_stream_decoder(&strm, UINT64_MAX, 0) == LZMA_OK);

  // Read the header, then our sector
  char *result = (char*)malloc(EGTB_CHUNK_SIZE);
  decodeBlock(&strm, fin, result, EGTB_CHUNK_SIZE, 12);
  fseek(fin, offset[0], SEEK_SET);
  decodeBlock(&strm, fin, result, EGTB_CHUNK_SIZE, offset[1] - offset[0]);
  fclose(fin);
	lzma_end(&strm);
  return result;
}

void writeVlq(u64 x, FILE* f) {
  static byte b[10];
  int size = 0;

  while (x || (size == 0)) {
    b[size++] = 128 ^ (x & 127); // number continues
    x >>= 7;
  }

  b[0] ^= 128; // remove continuation bit from the least significant group

  while (size--) {
    fwrite(b + size, 1, 1, f);
  }
}

u64 readVlq(FILE* f) {
  u64 result = 0;
  byte b;
  do {
    fread(&b, 1, 1, f);
    result = (result << 7) ^ (b & 127);
  } while (b & 128);
  return result;
}
