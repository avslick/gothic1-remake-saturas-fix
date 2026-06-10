// G1R Saturas fix: resets GuardPassageWaterMagesWarning_NC to 0 in a Gothic 1 Remake save
#include "stdafx.h"
#include <vector>
#include <string>
#include <string.h>

int Kraken_Decompress(const byte *src, size_t src_len, byte *dst, size_t dst_len);

static uint64 rd64(const byte* p){ uint64 v; memcpy(&v,p,8); return v; }
static uint32 rd32(const byte* p){ uint32 v; memcpy(&v,p,4); return v; }
static void wr64(byte* p, uint64 v){ memcpy(p,&v,8); }
static void wr32(byte* p, uint32 v){ memcpy(p,&v,4); }

static int fail(const char* msg){
  fprintf(stderr, "\n[ERROR] %s\n", msg);
  fprintf(stderr, "File was NOT modified.\nPress Enter to exit...");
  getchar();
  return 1;
}

int main(int argc, char** argv){
  printf("Gothic 1 Remake - Saturas guards fix (GuardPassageWaterMagesWarning_NC -> 0)\n");
  printf("----------------------------------------------------------------------------\n");
  if (argc < 2){
    printf("Usage: drag & drop your save file (e.g. G1R-013.sav) onto this exe,\n");
    printf("or run: G1R-SaturasFix.exe <path-to-save.sav>\n\nSaves are located in:\n");
    printf("C:\\Users\\<You>\\AppData\\Local\\G1R\\Saved\\SaveGames\n\nPress Enter to exit...");
    getchar();
    return 1;
  }

  // read file
  FILE* f = fopen(argv[1], "rb");
  if(!f) return fail("Cannot open the file.");
  fseek(f,0,SEEK_END); long fsz = ftell(f); fseek(f,0,SEEK_SET);
  std::vector<byte> data(fsz);
  if (fread(data.data(),1,fsz,f) != (size_t)fsz){ fclose(f); return fail("Read failed."); }
  fclose(f);
  printf("Loaded: %s (%ld bytes)\n", argv[1], fsz);

  if (fsz < 0x1000 || memcmp(data.data(), "GSAV", 4) != 0)
    return fail("Not a Gothic 1 Remake save (GSAV header missing).");

  // locate FString "Oodle"
  const byte pat[] = {6,0,0,0,'O','o','d','l','e',0};
  long i = -1;
  for (long k = 0; k + 10 < fsz && k < 0x10000; k++)
    if (!memcmp(&data[k], pat, 10)){ i = k; break; }
  if (i < 0) return fail("Oodle stream marker not found - unexpected save format.");

  long pos = i + 10;
  if (rd64(&data[pos]) != 0x222222229E2A83C1ull) return fail("Compressed stream tag mismatch.");
  long p = pos + 17;                       // tag(8) + chunk size(8) + flags byte(1)
  uint64 total_comp = rd64(&data[p]);
  uint64 total_unc  = rd64(&data[p+8]);
  long table_pos = p + 16;

  std::vector<uint64> bc, bu;
  uint64 su = 0; long q = table_pos;
  while (su < total_unc){
    if (q + 16 > fsz) return fail("Truncated chunk table.");
    bc.push_back(rd64(&data[q])); bu.push_back(rd64(&data[q+8])); q += 16;
    su += bu.back();
  }
  long data_start = q;
  if ((uint64)data_start + total_comp > (uint64)fsz) return fail("Compressed data exceeds file size.");
  long footer_off = data_start + (long)total_comp;
  if (rd32(&data[5]) != (uint32)footer_off) return fail("Header footer-offset check failed - unexpected save format.");
  printf("Parsed: %zu compressed blocks, %llu bytes uncompressed\n", bc.size(), (unsigned long long)total_unc);

  // decompress everything
  std::vector<byte> payload(total_unc + 64);
  uint64 off = data_start, dst = 0;
  for (size_t n = 0; n < bc.size(); n++){
    int r = Kraken_Decompress(&data[off], (size_t)bc[n], &payload[dst], (size_t)bu[n]);
    if (r != (int)bu[n]) return fail("Block decompression failed - save may be from a newer game version.");
    off += bc[n]; dst += bu[n];
  }
  payload.resize(total_unc);
  printf("Decompressed OK\n");

  // find flag
  const char* name = "GuardPassageWaterMagesWarning_NC";
  size_t nlen = strlen(name) + 1; // incl. trailing NUL
  long voff = -1;
  for (uint64 k = 0; k + nlen + 4 <= total_unc; k++)
    if (!memcmp(&payload[k], name, nlen)){ voff = (long)(k + nlen); break; }
  if (voff < 0) return fail("Flag not found in this save (maybe you never talked to the guards - no fix needed).");
  uint32 val = rd32(&payload[voff]);
  printf("Found %s = %u\n", name, val);
  if (val == 0){
    printf("\nThe flag is already 0 - nothing to fix. File not modified.\nPress Enter to exit...");
    getchar();
    return 0;
  }
  wr32(&payload[voff], 0);

  // which blocks does the 4-byte value touch
  const uint64 CH = 0x20000;
  size_t b0 = voff / CH, b1 = (voff + 3) / CH;

  // rebuild
  std::vector<byte> out;
  out.insert(out.end(), data.begin(), data.begin() + table_pos);
  std::vector<byte> stream;
  uint64 src_off = data_start, pay_off = 0, new_total = 0;
  for (size_t n = 0; n < bc.size(); n++){
    if (n >= b0 && n <= b1){
      // store this block uncompressed inside a valid Oodle (Kraken) container
      uint64 raw = bu[n];
      uint64 csz = raw + 2;
      byte hdr[2] = {0xCC, 0x06};
      stream.insert(stream.end(), hdr, hdr+2);
      stream.insert(stream.end(), payload.begin()+pay_off, payload.begin()+pay_off+raw);
      byte e[16]; wr64(e, csz); wr64(e+8, raw);
      out.insert(out.end(), e, e+16);
      new_total += csz;
    } else {
      stream.insert(stream.end(), data.begin()+src_off, data.begin()+src_off+bc[n]);
      byte e[16]; wr64(e, bc[n]); wr64(e+8, bu[n]);
      out.insert(out.end(), e, e+16);
      new_total += bc[n];
    }
    src_off += bc[n]; pay_off += bu[n];
  }
  wr64(&out[p], new_total);                       // summary compressed size
  out.insert(out.end(), stream.begin(), stream.end());
  uint32 new_footer = (uint32)out.size();
  out.insert(out.end(), data.begin()+footer_off, data.end());
  wr32(&out[5], new_footer);                      // header offset field

  // self-verify: decompress our own output
  {
    uint64 o2 = (uint64)table_pos + bc.size()*16, d2 = 0;
    std::vector<byte> check(total_unc + 64);
    long q2 = table_pos; uint64 s2 = 0;
    std::vector<uint64> nc, nu;
    while (s2 < total_unc){ nc.push_back(rd64(&out[q2])); nu.push_back(rd64(&out[q2+8])); q2 += 16; s2 += nu.back(); }
    o2 = q2;
    for (size_t n = 0; n < nc.size(); n++){
      int r = Kraken_Decompress(&out[o2], (size_t)nc[n], &check[d2], (size_t)nu[n]);
      if (r != (int)nu[n]) return fail("Self-verification failed.");
      o2 += nc[n]; d2 += nu[n];
    }
    if (memcmp(check.data(), payload.data(), total_unc) != 0) return fail("Self-verification mismatch.");
  }

  // output path: <orig>-fixed.sav
  std::string outpath = argv[1];
  size_t dot = outpath.rfind(".sav");
  if (dot != std::string::npos) outpath = outpath.substr(0, dot);
  outpath += "-fixed.sav";
  f = fopen(outpath.c_str(), "wb");
  if(!f) return fail("Cannot write output file.");
  fwrite(out.data(), 1, out.size(), f);
  fclose(f);

  printf("\nDone! Written: %s\n", outpath.c_str());
  printf("\nNext steps:\n");
  printf(" 1. Back up your original save.\n");
  printf(" 2. Replace the original .sav with the fixed file (rename it back, e.g. G1R-013.sav).\n");
  printf(" 3. Temporarily disable Steam Cloud for the game so it does not overwrite the file.\n");
  printf(" 4. Load the save - the guards in front of Saturas will talk to you again.\n");
  printf("\nPress Enter to exit...");
  getchar();
  return 0;
}
