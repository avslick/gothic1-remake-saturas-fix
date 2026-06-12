// G1R Fix Tool — console menu utility for fixing known bugs in Gothic 1 Remake saves
#include "stdafx.h"
#include <vector>
#include <string>
#include <algorithm>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
typedef std::wstring pstr;
#define PSEP L"\\"
#else
typedef std::string pstr;
#define PSEP "/"
#endif

int Kraken_Decompress(const byte *src, size_t src_len, byte *dst, size_t dst_len);

static uint64 rd64(const byte* p){ uint64 v; memcpy(&v,p,8); return v; }
static uint32 rd32(const byte* p){ uint32 v; memcpy(&v,p,4); return v; }
static void wr64(byte* p, uint64 v){ memcpy(p,&v,8); }
static void wr32(byte* p, uint32 v){ memcpy(p,&v,4); }

// ---------- encodings / files ----------
static std::string disp(const pstr& s){
#ifdef _WIN32
  int n = WideCharToMultiByte(CP_UTF8,0,s.c_str(),-1,NULL,0,NULL,NULL);
  std::string r(n>0?n-1:0,0);
  if(n>1) WideCharToMultiByte(CP_UTF8,0,s.c_str(),-1,&r[0],n,NULL,NULL);
  return r;
#else
  return s;
#endif
}
static FILE* fopen_p(const pstr& p, const char* mode){
#ifdef _WIN32
  wchar_t wm[8]; for(int i=0;;i++){ wm[i]=mode[i]; if(!mode[i]) break; }
  return _wfopen(p.c_str(), wm);
#else
  return fopen(p.c_str(), mode);
#endif
}
static bool load_file(const pstr& p, std::vector<byte>& out){
  FILE* f = fopen_p(p,"rb"); if(!f) return false;
  fseek(f,0,SEEK_END); long sz=ftell(f); fseek(f,0,SEEK_SET);
  out.resize(sz);
  bool ok = fread(out.data(),1,sz,f)==(size_t)sz;
  fclose(f); return ok;
}
static bool save_file(const pstr& p, const std::vector<byte>& d){
  FILE* f = fopen_p(p,"wb"); if(!f) return false;
  bool ok = fwrite(d.data(),1,d.size(),f)==d.size();
  fclose(f); return ok;
}
static bool file_exists(const pstr& p){ FILE* f=fopen_p(p,"rb"); if(f){fclose(f);return true;} return false; }

static void pause_exit(int code){
  printf("\nPress Enter to exit...");
  getchar();
  exit(code);
}
static std::string read_line(){
  char buf[256];
  if(!fgets(buf,sizeof buf,stdin)){ printf("\n(stdin closed — exiting without changes)\n"); exit(0); }
  std::string s(buf);
  while(!s.empty() && (s.back()=='\n'||s.back()=='\r'||s.back()==' ')) s.pop_back();
  return s;
}

// ---------- fix definition ----------
struct Fix {
  std::string title, desc, var;
  int32 target = 0;
  // runtime:
  long off = -1;       // offset of the value inside the payload
  int32 current = 0;
  bool selected = false;
};

static std::string trim(std::string s){
  size_t a=s.find_first_not_of(" \t\r\n"); size_t b=s.find_last_not_of(" \t\r\n");
  return a==std::string::npos ? "" : s.substr(a,b-a+1);
}

static std::vector<Fix> builtin_fixes(){
  std::vector<Fix> v;
  Fix f;
  f.title = "Guards and Water Mages turn hostile before Saturas (Chapter 3)";
  f.desc  = "Resets the New Camp guard warning that makes everyone attack instead of talking";
  f.var   = "GuardPassageWaterMagesWarning_NC";
  f.target= 0;
  v.push_back(f);
  return v;
}

static std::vector<Fix> load_fixes(const pstr& exe_dir){
  pstr ini = exe_dir + PSEP +
#ifdef _WIN32
    L"fixes.ini";
#else
    "fixes.ini";
#endif
  std::vector<byte> raw;
  if (!load_file(ini, raw)) return builtin_fixes();
  // strip UTF-8 BOM
  size_t st = (raw.size()>=3 && raw[0]==0xEF && raw[1]==0xBB && raw[2]==0xBF) ? 3 : 0;
  std::string text((char*)raw.data()+st, raw.size()-st);
  std::vector<Fix> v; Fix cur; bool open=false;
  std::string cur_ten, cur_den;   // English title/desc — preferred in the console UI
  size_t pos=0;
  auto flush=[&]{
    if(!cur_ten.empty()) cur.title = cur_ten;
    if(!cur_den.empty()) cur.desc  = cur_den;
    if(open && !cur.var.empty() && !cur.title.empty()) v.push_back(cur);
    cur=Fix(); cur_ten.clear(); cur_den.clear();
  };
  while (pos <= text.size()){
    size_t e = text.find('\n', pos);
    std::string line = trim(text.substr(pos, e==std::string::npos? std::string::npos : e-pos));
    pos = e==std::string::npos ? text.size()+1 : e+1;
    if (line.empty() || line[0]=='#' || line[0]==';') continue;
    if (line == "[fix]"){ flush(); open=true; continue; }
    size_t eq = line.find('=');
    if (eq==std::string::npos || !open) continue;
    std::string k = trim(line.substr(0,eq)), val = trim(line.substr(eq+1));
    if (k=="title") cur.title = val;
    else if (k=="title_en") cur_ten = val;
    else if (k=="desc") cur.desc = val;
    else if (k=="desc_en") cur_den = val;
    else if (k=="var") cur.var = val;
    else if (k=="set") cur.target = (int32)strtol(val.c_str(),NULL,10);
  }
  flush();
  if (v.empty()){
    printf("[!] fixes.ini found, but it contains no valid fixes — using the built-in list.\n");
    return builtin_fixes();
  }
  printf("Loaded %zu fix(es) from fixes.ini\n", v.size());
  return v;
}

// ---------- save parsing ----------
struct SaveMeta {
  std::vector<byte> data;
  long summaryPos, tablePos, dataStart, footerOff;
  uint64 totalComp, totalUnc;
  std::vector<uint64> bc, bu;
};

static const char* parse_save(SaveMeta& m){
  if (m.data.size() < 0x1000 || memcmp(m.data.data(),"GSAV",4)!=0)
    return "This is not a Gothic 1 Remake save (no GSAV header).";
  const byte pat[10] = {6,0,0,0,'O','o','d','l','e',0};
  long i=-1;
  for (long k=0;k+10<(long)m.data.size() && k<0x10000;k++)
    if (!memcmp(&m.data[k],pat,10)){ i=k; break; }
  if (i<0) return "Oodle compressed stream not found — different format (newer game version?).";
  long pos=i+10;
  if (rd64(&m.data[pos]) != 0x222222229E2A83C1ull) return "Unexpected compressed stream tag.";
  m.summaryPos = pos+17;
  m.totalComp = rd64(&m.data[m.summaryPos]);
  m.totalUnc  = rd64(&m.data[m.summaryPos+8]);
  m.tablePos = m.summaryPos+16;
  long q=m.tablePos; uint64 s=0;
  while (s < m.totalUnc){
    if (q+16 > (long)m.data.size()) return "Truncated file: incomplete chunk table.";
    m.bc.push_back(rd64(&m.data[q])); m.bu.push_back(rd64(&m.data[q+8])); q+=16; s+=m.bu.back();
  }
  m.dataStart = q;
  m.footerOff = m.dataStart + (long)m.totalComp;
  if (m.footerOff > (long)m.data.size()) return "Truncated file: compressed data missing.";
  if (rd32(&m.data[5]) != (uint32)m.footerOff) return "Header offset check failed.";
  return NULL;
}

static const char* decompress_all(const SaveMeta& m, std::vector<byte>& payload){
  payload.assign(m.totalUnc+64, 0);
  uint64 off=m.dataStart, dst=0;
  for (size_t n=0;n<m.bc.size();n++){
    int r = Kraken_Decompress(&m.data[off], (size_t)m.bc[n], &payload[dst], (size_t)m.bu[n]);
    if (r != (int)m.bu[n]) return "Could not decompress a data chunk (save from a newer game version?).";
    off+=m.bc[n]; dst+=m.bu[n];
  }
  payload.resize(m.totalUnc);
  return NULL;
}

static long find_var(const std::vector<byte>& pl, const std::string& name){
  size_t nlen = name.size()+1; // incl. NUL
  if (pl.size() < nlen+8) return -1;
  for (size_t k=4; k+nlen+4 <= pl.size(); k++){
    if (pl[k+nlen-1]==0 && !memcmp(&pl[k], name.c_str(), name.size())){
      // sanity: the uint32 right before the name must be the string length incl. NUL
      if (rd32(&pl[k-4]) == (uint32)nlen) return (long)(k+nlen);
    }
  }
  return -1;
}

static std::vector<byte> rebuild(const SaveMeta& m, const std::vector<byte>& payload,
                                 const std::vector<std::pair<long,int32>>& edits){
  std::vector<byte> pl = payload;
  std::vector<bool> dirty(m.bc.size(), false);
  const uint64 CH = 0x20000;
  for (auto& e : edits){
    wr32(&pl[e.first], (uint32)e.second);
    dirty[e.first/CH] = true;
    dirty[(e.first+3)/CH] = true;
  }
  std::vector<byte> out(m.data.begin(), m.data.begin()+m.tablePos);
  std::vector<byte> stream;
  uint64 src=m.dataStart, pay=0, newTotal=0;
  for (size_t n=0;n<m.bc.size();n++){
    uint64 c, u=m.bu[n];
    if (dirty[n]){
      c = u+2;
      byte hdr[2]={0xCC,0x06};
      stream.insert(stream.end(), hdr, hdr+2);
      stream.insert(stream.end(), pl.begin()+pay, pl.begin()+pay+u);
    } else {
      c = m.bc[n];
      stream.insert(stream.end(), m.data.begin()+src, m.data.begin()+src+c);
    }
    byte e[16]; wr64(e,c); wr64(e+8,u);
    out.insert(out.end(), e, e+16);
    newTotal += c;
    src+=m.bc[n]; pay+=u;
  }
  wr64(&out[m.summaryPos], newTotal);
  out.insert(out.end(), stream.begin(), stream.end());
  uint32 newFooter = (uint32)out.size();
  out.insert(out.end(), m.data.begin()+m.footerOff, m.data.end());
  wr32(&out[5], newFooter);
  return out;
}

// ---------- save selection ----------
#ifdef _WIN32
static std::vector<std::pair<pstr,FILETIME>> list_saves(const pstr& dir){
  std::vector<std::pair<pstr,FILETIME>> v;
  WIN32_FIND_DATAW fd;
  HANDLE h = FindFirstFileW((dir + L"\\*.sav").c_str(), &fd);
  if (h == INVALID_HANDLE_VALUE) return v;
  do { v.push_back({dir + L"\\" + fd.cFileName, fd.ftLastWriteTime}); } while (FindNextFileW(h,&fd));
  FindClose(h);
  std::sort(v.begin(), v.end(), [](auto&a, auto&b){ return CompareFileTime(&a.second,&b.second) > 0; });
  return v;
}
#endif

static pstr pick_save(int argc,
#ifdef _WIN32
  wchar_t** argv
#else
  char** argv
#endif
){
  if (argc >= 2) return argv[1];
#ifdef _WIN32
  wchar_t* la = _wgetenv(L"LOCALAPPDATA");
  if (la){
    pstr dir = pstr(la) + L"\\G1R\\Saved\\SaveGames";
    auto saves = list_saves(dir);
    if (!saves.empty()){
      printf("Saves found in %s\n(most recent first):\n\n", disp(dir).c_str());
      size_t show = saves.size() < 15 ? saves.size() : 15;
      for (size_t k=0;k<show;k++){
        SYSTEMTIME st; FILETIME lt; FileTimeToLocalFileTime(&saves[k].second,&lt); FileTimeToSystemTime(&lt,&st);
        pstr name = saves[k].first.substr(saves[k].first.find_last_of(L'\\')+1);
        printf("  %2zu. %-20s %02d.%02d.%04d %02d:%02d\n", k+1, disp(name).c_str(),
               st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute);
      }
      printf("\nSave number (Enter = 1): ");
      std::string in = read_line();
      size_t idx = in.empty() ? 1 : (size_t)atoi(in.c_str());
      if (idx >= 1 && idx <= saves.size()) return saves[idx-1].first;
      printf("Invalid number.\n");
      pause_exit(1);
    }
  }
  printf("Could not find the save folder automatically.\n");
  printf("Drag & drop a .sav file onto the exe, or pass its path as an argument.\n");
  pause_exit(1);
#endif
  return pstr();
}

// ---------- main ----------
#ifdef _WIN32
int wmain(int argc, wchar_t** argv){
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
#else
int main(int argc, char** argv){
#endif
  printf("==========================================================\n");
  printf("  Gothic 1 Remake — Fix Tool (known save-bug fixer)\n");
  printf("==========================================================\n\n");

  // exe directory — where fixes.ini lives
  pstr exe_dir;
#ifdef _WIN32
  { wchar_t buf[MAX_PATH]; GetModuleFileNameW(NULL, buf, MAX_PATH);
    pstr p(buf); size_t sl = p.find_last_of(L'\\');
    exe_dir = sl==pstr::npos ? L"." : p.substr(0,sl); }
#else
  { std::string p(argv[0]); size_t sl=p.find_last_of('/');
    exe_dir = sl==std::string::npos ? "." : p.substr(0,sl); }
#endif

  std::vector<Fix> fixes = load_fixes(exe_dir);
  pstr savepath = pick_save(argc, argv);

  SaveMeta m;
  if (!load_file(savepath, m.data)){ printf("[ERROR] Could not open the file.\n"); pause_exit(1); }
  printf("\nFile: %s (%zu bytes)\n", disp(savepath).c_str(), m.data.size());
  const char* err = parse_save(m);
  if (err){ printf("[ERROR] %s\nThe file was not modified.\n", err); pause_exit(1); }
  std::vector<byte> payload;
  printf("Decompressing (%zu chunks)...\n", m.bc.size());
  err = decompress_all(m, payload);
  if (err){ printf("[ERROR] %s\nThe file was not modified.\n", err); pause_exit(1); }

  // locate the variables of all fixes
  for (auto& f : fixes){
    f.off = find_var(payload, f.var);
    if (f.off >= 0) f.current = (int32)rd32(&payload[f.off]);
  }

  // menu
  for (;;){
    printf("\n----------------------------------------------------------\n");
    for (size_t k=0;k<fixes.size();k++){
      Fix& f = fixes[k];
      const char* st;
      char mark = f.selected ? 'x' : ' ';
      if (f.off < 0) st = "not found in this save — not applicable";
      else if (f.current == f.target) st = "already fixed";
      else st = "applicable";
      printf(" %2zu. [%c] %s\n", k+1, mark, f.title.c_str());
      if (!f.desc.empty()) printf("        %s\n", f.desc.c_str());
      if (f.off >= 0) printf("        %s: %d -> %d   (%s)\n", f.var.c_str(), f.current, f.target, st);
      else            printf("        %s   (%s)\n", f.var.c_str(), st);
    }
    printf("----------------------------------------------------------\n");
    printf("Number — toggle a fix, A — apply selected, Q — quit: ");
    std::string in = read_line();
    if (in=="q"||in=="Q"||in=="й"||in=="Й"){ printf("Exiting without changes.\n"); pause_exit(0); }
    if (in=="a"||in=="A"||in=="ф"||in=="Ф"){
      std::vector<std::pair<long,int32>> edits;
      for (auto& f : fixes)
        if (f.selected && f.off>=0 && f.current != f.target)
          edits.push_back({f.off, f.target});
      if (edits.empty()){ printf("Nothing selected (or the selection is already fixed).\n"); continue; }

      std::vector<byte> out = rebuild(m, payload, edits);

      // self-verification: re-parse the rebuilt file and confirm the values were applied
      {
        SaveMeta m2; m2.data = out;
        if (parse_save(m2)){ printf("[ERROR] Structure self-check failed. Nothing was written.\n"); pause_exit(1); }
        std::vector<byte> pl2;
        if (decompress_all(m2, pl2)){ printf("[ERROR] Decompression self-check failed. Nothing was written.\n"); pause_exit(1); }
        for (auto& e : edits)
          if ((int32)rd32(&pl2[e.first]) != e.second){ printf("[ERROR] Value self-check failed. Nothing was written.\n"); pause_exit(1); }
      }

      // backup, then overwrite
      pstr bak = savepath +
#ifdef _WIN32
        L".backup";
#else
        ".backup";
#endif
      if (file_exists(bak)){
        char suf[32]; snprintf(suf,sizeof suf,".backup%u",(unsigned)(time(NULL)%1000000));
#ifdef _WIN32
        wchar_t wsuf[32]; for(int i=0;;i++){ wsuf[i]=suf[i]; if(!suf[i])break; }
        bak = savepath + wsuf;
#else
        bak = savepath + suf;
#endif
      }
      if (!save_file(bak, m.data)){ printf("[ERROR] Could not create a backup copy. The file was not modified.\n"); pause_exit(1); }
      if (!save_file(savepath, out)){ printf("[ERROR] Could not write the file! The original is saved as %s\n", disp(bak).c_str()); pause_exit(1); }

      printf("\nDone! Fixes applied: %zu\n", edits.size());
      printf("Backup copy: %s\n", disp(bak).c_str());
      printf("\nRemember: Steam Cloud may restore the old save — temporarily disable\n");
      printf("it in the game's properties before the first launch after the fix.\n");
      pause_exit(0);
    }
    int n = atoi(in.c_str());
    if (n>=1 && n<=(int)fixes.size()){
      Fix& f = fixes[n-1];
      if (f.off<0) printf("This fix is not applicable: variable not found in this save.\n");
      else if (f.current==f.target) printf("Already fixed — no need to select it.\n");
      else f.selected = !f.selected;
    } else if (!in.empty()) printf("Unrecognized input.\n");
  }
}
