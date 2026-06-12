// G1R Fix Tool — GUI version (Win32, no dependencies), RU/EN interface
#define WIN32_LEAN_AND_MEAN
#include "stdafx.h"
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <vector>
#include <string>
#include <algorithm>
#include <time.h>

int Kraken_Decompress(const byte *src, size_t src_len, byte *dst, size_t dst_len);

static uint64 rd64(const byte* p){ uint64 v; memcpy(&v,p,8); return v; }
static uint32 rd32(const byte* p){ uint32 v; memcpy(&v,p,4); return v; }
static void wr64(byte* p, uint64 v){ memcpy(p,&v,8); }
static void wr32(byte* p, uint32 v){ memcpy(p,&v,4); }

// ======================= localization =======================
enum { LRU=0, LEN=1 };
static int g_lang = LRU;

enum StrId {
  S_TITLE, S_BROWSE, S_REFRESH, S_APPLY,
  S_COL_FIX, S_COL_VAR, S_COL_CUR, S_COL_NEW, S_COL_STATUS,
  S_SELECT_SAVE, S_NO_SAVES, S_OPENING, S_LOADED_FMT,
  S_CANT_OPEN, S_NOT_APPLICABLE_ST, S_ALREADY_FIXED_ST,
  S_ST_APPLICABLE, S_ST_ALREADY, S_ST_NOTFOUND,
  S_MB_NONE_SELECTED, S_MB_CONFIRM_TITLE, S_MB_CONFIRM_HEAD, S_MB_CONFIRM_TAIL,
  S_BUILDING, S_VERIFY_FAIL_HEAD, S_VERIFY_FAIL_TAIL, S_VERIFY_FAIL_ST,
  S_ERR_TITLE, S_CANT_BACKUP, S_CANT_BACKUP_ST, S_CANT_WRITE_FMT, S_CANT_WRITE_ST,
  S_DONE_FMT, S_INFO_TITLE,
  S_FILTER,
  E_NOTGSAV, E_NOSTREAM, E_TAG, E_TRUNC_TABLE, E_TRUNC_DATA, E_HDR, E_DECOMP,
  S__COUNT
};

static const wchar_t* STR[S__COUNT][2] = {
  /*S_TITLE*/ {L"G1R Fix Tool — исправление багов Gothic 1 Remake", L"G1R Fix Tool — Gothic 1 Remake save fixer"},
  /*S_BROWSE*/ {L"Обзор…", L"Browse…"},
  /*S_REFRESH*/ {L"Обновить список", L"Refresh list"},
  /*S_APPLY*/ {L"Применить исправления", L"Apply fixes"},
  /*S_COL_FIX*/ {L"Исправление", L"Fix"},
  /*S_COL_VAR*/ {L"Переменная", L"Variable"},
  /*S_COL_CUR*/ {L"Сейчас", L"Current"},
  /*S_COL_NEW*/ {L"Будет", L"New"},
  /*S_COL_STATUS*/ {L"Статус", L"Status"},
  /*S_SELECT_SAVE*/ {L"Выберите сохранение.", L"Select a save file."},
  /*S_NO_SAVES*/ {L"Сейвы не найдены автоматически — нажмите «Обзор…» или перетащите .sav в окно.",
                  L"No saves found automatically — click “Browse…” or drop a .sav onto the window."},
  /*S_OPENING*/ {L"Открываю и распаковываю сейв…", L"Opening and decompressing the save…"},
  /*S_LOADED_FMT*/ {L"Сейв загружен. Доступно исправлений: %d. Отметьте нужные и нажмите «Применить».",
                    L"Save loaded. Applicable fixes: %d. Check the ones you need and press “Apply”."},
  /*S_CANT_OPEN*/ {L"Не удалось открыть файл (возможно, занят игрой — закройте игру).",
                   L"Could not open the file (it may be locked by the game — close the game)."},
  /*S_NOT_APPLICABLE_ST*/ {L"Это исправление не применимо: переменной нет в сейве.",
                           L"This fix is not applicable: the variable is not present in this save."},
  /*S_ALREADY_FIXED_ST*/ {L"Это уже исправлено в выбранном сейве.", L"This is already fixed in the selected save."},
  /*S_ST_APPLICABLE*/ {L"применимо", L"applicable"},
  /*S_ST_ALREADY*/ {L"уже исправлено", L"already fixed"},
  /*S_ST_NOTFOUND*/ {L"не найдено в сейве", L"not found in save"},
  /*S_MB_NONE_SELECTED*/ {L"Не отмечено ни одного применимого исправления.", L"No applicable fixes are checked."},
  /*S_MB_CONFIRM_TITLE*/ {L"Подтверждение", L"Confirm"},
  /*S_MB_CONFIRM_HEAD*/ {L"Будут применены исправления:\n\n", L"The following fixes will be applied:\n\n"},
  /*S_MB_CONFIRM_TAIL*/ {L"\nОригинал будет сохранён рядом как *.backup.\nПродолжить?",
                         L"\nThe original will be kept next to it as *.backup.\nContinue?"},
  /*S_BUILDING*/ {L"Собираю файл и проверяю…", L"Rebuilding and verifying the file…"},
  /*S_VERIFY_FAIL_HEAD*/ {L"Самопроверка собранного файла не прошла:\n", L"Self-verification of the rebuilt file failed:\n"},
  /*S_VERIFY_FAIL_TAIL*/ {L"\n\nФайл НЕ записан, оригинал не изменён.", L"\n\nNothing was written, the original is untouched."},
  /*S_VERIFY_FAIL_ST*/ {L"Ошибка самопроверки — файл не изменён.", L"Self-verification failed — file not modified."},
  /*S_ERR_TITLE*/ {L"Ошибка", L"Error"},
  /*S_CANT_BACKUP*/ {L"Не удалось создать резервную копию. Файл не изменён.",
                     L"Could not create a backup copy. File not modified."},
  /*S_CANT_BACKUP_ST*/ {L"Не удалось создать резервную копию.", L"Could not create a backup copy."},
  /*S_CANT_WRITE_FMT*/ {L"Не удалось записать сейв (возможно, занят игрой).\nОригинал цел, копия: %ls",
                        L"Could not write the save (it may be locked by the game).\nThe original is intact, backup: %ls"},
  /*S_CANT_WRITE_ST*/ {L"Не удалось записать файл.", L"Could not write the file."},
  /*S_DONE_FMT*/ {L"Готово! Применено исправлений: %d\n\nРезервная копия:\n%ls\n\nЕсли включён Steam Cloud — при первом запуске после правки временно отключите его, чтобы облако не вернуло старый сейв.",
                  L"Done! Fixes applied: %d\n\nBackup copy:\n%ls\n\nIf Steam Cloud is enabled, temporarily disable it before the first launch so the cloud doesn't restore the old save."},
  /*S_INFO_TITLE*/ {L"G1R Fix Tool", L"G1R Fix Tool"},
  /*S_FILTER*/ {L"Сохранения Gothic 1 Remake (*.sav)\0*.sav\0Все файлы\0*.*\0",
                L"Gothic 1 Remake saves (*.sav)\0*.sav\0All files\0*.*\0"},
  /*E_NOTGSAV*/ {L"Это не сохранение Gothic 1 Remake (нет заголовка GSAV).",
                 L"This is not a Gothic 1 Remake save (GSAV header missing)."},
  /*E_NOSTREAM*/ {L"Не найден сжатый поток Oodle — формат отличается (новая версия игры?).",
                  L"Oodle stream marker not found — different format (newer game version?)."},
  /*E_TAG*/ {L"Неожиданный маркер сжатого потока.", L"Unexpected compressed stream tag."},
  /*E_TRUNC_TABLE*/ {L"Файл обрезан: таблица блоков неполная.", L"Truncated file: incomplete chunk table."},
  /*E_TRUNC_DATA*/ {L"Файл обрезан: не хватает сжатых данных.", L"Truncated file: compressed data missing."},
  /*E_HDR*/ {L"Проверка смещения в заголовке не прошла.", L"Header offset check failed."},
  /*E_DECOMP*/ {L"Не удалось распаковать блок данных (сейв от новой версии игры?).",
                L"Failed to decompress a data block (save from a newer game version?)."},
};
static const wchar_t* T(StrId id){ return STR[id][g_lang]; }

// ======================= core =======================
struct Fix {
  std::wstring title[2], desc[2];
  std::string var;
  int32 target = 0;
  long off = -1;
  int32 current = 0;
  const std::wstring& Title() const { return title[g_lang].empty() ? title[LRU] : title[g_lang]; }
  const std::wstring& Desc()  const { return desc[g_lang].empty()  ? desc[LRU]  : desc[g_lang]; }
};
struct SaveMeta {
  std::vector<byte> data;
  long summaryPos=0, tablePos=0, dataStart=0, footerOff=0;
  uint64 totalComp=0, totalUnc=0;
  std::vector<uint64> bc, bu;
};

static std::wstring towide(const std::string& s){
  int n = MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,NULL,0);
  std::wstring r(n>0?n-1:0,0);
  if(n>1) MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,&r[0],n);
  return r;
}

static bool load_file(const std::wstring& p, std::vector<byte>& out){
  FILE* f = _wfopen(p.c_str(), L"rb"); if(!f) return false;
  fseek(f,0,SEEK_END); long sz=ftell(f); fseek(f,0,SEEK_SET);
  out.resize(sz);
  bool ok = fread(out.data(),1,sz,f)==(size_t)sz;
  fclose(f); return ok;
}
static bool save_file(const std::wstring& p, const std::vector<byte>& d){
  FILE* f = _wfopen(p.c_str(), L"wb"); if(!f) return false;
  bool ok = fwrite(d.data(),1,d.size(),f)==d.size();
  fclose(f); return ok;
}
static bool file_exists(const std::wstring& p){ DWORD a=GetFileAttributesW(p.c_str()); return a!=INVALID_FILE_ATTRIBUTES; }

// returns 0 = OK, otherwise the StrId of the error
static int parse_save(SaveMeta& m){
  if (m.data.size() < 0x1000 || memcmp(m.data.data(),"GSAV",4)!=0) return E_NOTGSAV;
  const byte pat[10] = {6,0,0,0,'O','o','d','l','e',0};
  long i=-1;
  for (long k=0;k+10<(long)m.data.size() && k<0x10000;k++)
    if (!memcmp(&m.data[k],pat,10)){ i=k; break; }
  if (i<0) return E_NOSTREAM;
  long pos=i+10;
  if (rd64(&m.data[pos]) != 0x222222229E2A83C1ull) return E_TAG;
  m.summaryPos = pos+17;
  m.totalComp = rd64(&m.data[m.summaryPos]);
  m.totalUnc  = rd64(&m.data[m.summaryPos+8]);
  m.tablePos = m.summaryPos+16;
  long q=m.tablePos; uint64 s=0;
  m.bc.clear(); m.bu.clear();
  while (s < m.totalUnc){
    if (q+16 > (long)m.data.size()) return E_TRUNC_TABLE;
    m.bc.push_back(rd64(&m.data[q])); m.bu.push_back(rd64(&m.data[q+8])); q+=16; s+=m.bu.back();
  }
  m.dataStart = q;
  m.footerOff = m.dataStart + (long)m.totalComp;
  if (m.footerOff > (long)m.data.size()) return E_TRUNC_DATA;
  if (rd32(&m.data[5]) != (uint32)m.footerOff) return E_HDR;
  return 0;
}
static int decompress_all(const SaveMeta& m, std::vector<byte>& payload){
  payload.assign(m.totalUnc+64, 0);
  uint64 off=m.dataStart, dst=0;
  for (size_t n=0;n<m.bc.size();n++){
    int r = Kraken_Decompress(&m.data[off], (size_t)m.bc[n], &payload[dst], (size_t)m.bu[n]);
    if (r != (int)m.bu[n]) return E_DECOMP;
    off+=m.bc[n]; dst+=m.bu[n];
  }
  payload.resize(m.totalUnc);
  return 0;
}
static long find_var(const std::vector<byte>& pl, const std::string& name){
  size_t nlen = name.size()+1;
  if (pl.size() < nlen+8) return -1;
  for (size_t k=4; k+nlen+4 <= pl.size(); k++)
    if (pl[k+nlen-1]==0 && !memcmp(&pl[k], name.c_str(), name.size()))
      if (rd32(&pl[k-4]) == (uint32)nlen) return (long)(k+nlen);
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

// ======================= fixes.ini + config =======================
static std::string trim(std::string s){
  size_t a=s.find_first_not_of(" \t\r\n"); size_t b=s.find_last_not_of(" \t\r\n");
  return a==std::string::npos ? "" : s.substr(a,b-a+1);
}
static std::wstring exe_dir(){
  wchar_t buf[MAX_PATH]; GetModuleFileNameW(NULL, buf, MAX_PATH);
  std::wstring p(buf); size_t sl = p.find_last_of(L'\\');
  return (sl==std::wstring::npos) ? L"." : p.substr(0,sl);
}
static std::vector<Fix> builtin_fixes(){
  std::vector<Fix> v; Fix f;
  f.title[LRU] = L"Стража и маги Воды агрятся перед Сатурасом (глава 3)";
  f.title[LEN] = L"Guards and Water Mages turn hostile before Saturas (Chapter 3)";
  f.desc[LRU]  = L"Сбрасывает предупреждение стражи Нового лагеря, из-за которого вместо диалога все атакуют.";
  f.desc[LEN]  = L"Resets the New Camp guard warning that makes everyone attack instead of talking.";
  f.var   = "GuardPassageWaterMagesWarning_NC";
  f.target= 0;
  v.push_back(f);
  return v;
}
static std::vector<Fix> load_fixes(){
  std::vector<byte> raw;
  if (!load_file(exe_dir() + L"\\fixes.ini", raw)) return builtin_fixes();
  size_t st = (raw.size()>=3 && raw[0]==0xEF && raw[1]==0xBB && raw[2]==0xBF) ? 3 : 0;
  std::string text((char*)raw.data()+st, raw.size()-st);
  std::vector<Fix> v; Fix cur; bool open=false;
  auto flush=[&]{ if(open && !cur.var.empty() && !cur.title[LRU].empty()) v.push_back(cur); cur=Fix(); };
  size_t pos=0;
  while (pos <= text.size()){
    size_t e = text.find('\n', pos);
    std::string line = trim(text.substr(pos, e==std::string::npos? std::string::npos : e-pos));
    pos = e==std::string::npos ? text.size()+1 : e+1;
    if (line.empty() || line[0]=='#' || line[0]==';') continue;
    if (line == "[fix]"){ flush(); open=true; continue; }
    size_t eq = line.find('=');
    if (eq==std::string::npos || !open) continue;
    std::string k = trim(line.substr(0,eq)), val = trim(line.substr(eq+1));
    if (k=="title") cur.title[LRU] = towide(val);
    else if (k=="title_en") cur.title[LEN] = towide(val);
    else if (k=="desc") cur.desc[LRU] = towide(val);
    else if (k=="desc_en") cur.desc[LEN] = towide(val);
    else if (k=="var") cur.var = val;
    else if (k=="set") cur.target = (int32)strtol(val.c_str(),NULL,10);
  }
  flush();
  return v.empty() ? builtin_fixes() : v;
}
static std::wstring cfg_path(){ return exe_dir() + L"\\G1RFixTool.cfg"; }
static void load_lang(){
  std::vector<byte> raw;
  if (load_file(cfg_path(), raw)){
    std::string s((char*)raw.data(), raw.size());
    if (s.find("lang=en") != std::string::npos){ g_lang = LEN; return; }
    if (s.find("lang=ru") != std::string::npos){ g_lang = LRU; return; }
  }
  LANGID id = GetUserDefaultUILanguage();
  WORD prim = PRIMARYLANGID(id);
  g_lang = (prim==LANG_RUSSIAN || prim==LANG_UKRAINIAN || prim==LANG_BELARUSIAN) ? LRU : LEN;
}
static void save_lang(){
  const char* s = (g_lang==LEN) ? "lang=en\r\n" : "lang=ru\r\n";
  std::vector<byte> d(s, s+strlen(s));
  save_file(cfg_path(), d);
}

// ======================= GUI =======================
#define IDC_COMBO   101
#define IDC_BROWSE  102
#define IDC_LIST    103
#define IDC_DESC    104
#define IDC_STATUS  105
#define IDC_APPLY   106
#define IDC_REFRESH 107
#define IDC_LANG    108

static HWND hMain, hCombo, hBrowse, hList, hDesc, hStatus, hApply, hRefresh, hLang;
static HFONT hFont, hFontBold;
static std::vector<std::wstring> g_savePaths;
static std::vector<Fix> g_fixes;
static SaveMeta g_meta;
static std::vector<byte> g_payload;
static bool g_loaded=false, g_updatingChecks=false;
static int g_dpi=96;
static int S(int v){ return MulDiv(v, g_dpi, 96); }

static void set_status(const std::wstring& s){ SetWindowTextW(hStatus, s.c_str()); }

static std::wstring save_dir(){
  wchar_t* la = _wgetenv(L"LOCALAPPDATA");
  return la ? std::wstring(la) + L"\\G1R\\Saved\\SaveGames" : L"";
}


// in-game save name (m_PlayerSaveName) from the uncompressed header
static std::wstring read_game_name(const std::wstring& path){
  FILE* f=_wfopen(path.c_str(),L"rb"); if(!f) return L"";
  std::vector<byte> h(65536);
  size_t n=fread(h.data(),1,h.size(),f); fclose(f); h.resize(n);
  if (n<64 || memcmp(h.data(),"GSAV",4)!=0) return L"";
  const char key[]="m_PlayerSaveName";
  long i=-1;
  for (size_t k=0;k+sizeof(key)<h.size() && k<0x4000;k++)
    if (!memcmp(&h[k],key,sizeof(key)-1)){ i=(long)k; break; }
  if (i<0) return L"";
  const char st[]="StrProperty";
  long t=-1;
  for (size_t k=i;k+12<h.size() && k<(size_t)i+64;k++)
    if (!memcmp(&h[k],st,11) && h[k+11]==0){ t=(long)(k+12); break; }
  if (t<0) return L"";
  auto try_at=[&](long p)->std::wstring{
    if (p+4 > (long)h.size()) return L"";
    int32 L; memcpy(&L,&h[p],4);
    if (L==0) return L"";
    if (L<0){                       // UTF-16LE, -L characters incl. NUL
      long chars=-(long)L;
      if (chars<2 || chars>257 || p+4+chars*2 > (long)h.size()) return L"";
      std::wstring w; w.reserve(chars);
      for (long c=0;c<chars;c++){ uint16 u; memcpy(&u,&h[p+4+c*2],2); if(u) w.push_back((wchar_t)u); }
      return w;
    } else {                        // single-byte string incl. NUL
      if (L<2 || L>257 || p+4+L > (long)h.size()) return L"";
      if (h[p+4+L-1]!=0) return L"";
      for (long c=0;c<L-1;c++) if (h[p+4+c]<32 || h[p+4+c]>=127) return L"";
      std::string a((char*)&h[p+4], L-1);
      return towide(a);
    }
  };
  std::wstring w = try_at(t+9);     // 4 padding bytes + int32 size + 1 flag byte
  for (long d=5; w.empty() && d<=16; d++) w = try_at(t+d);
  if (w.size() > 48){ w.resize(47); w += L'…'; }
  return w;
}

static void scan_saves(const std::wstring& preselect){
  SendMessageW(hCombo, CB_RESETCONTENT, 0, 0);
  g_savePaths.clear();
  std::wstring dir = save_dir();
  struct Entry{ std::wstring path, name; FILETIME ft; };
  std::vector<Entry> es;
  if (!dir.empty()){
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((dir + L"\\*.sav").c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE){
      do { es.push_back({dir + L"\\" + fd.cFileName, fd.cFileName, fd.ftLastWriteTime}); } while (FindNextFileW(h,&fd));
      FindClose(h);
    }
  }
  std::sort(es.begin(), es.end(), [](const Entry&a, const Entry&b){ return CompareFileTime(&a.ft,&b.ft) > 0; });
  int sel = -1;
  for (auto& e : es){
    SYSTEMTIME st; FILETIME lt; FileTimeToLocalFileTime(&e.ft,&lt); FileTimeToSystemTime(&lt,&st);
    std::wstring gname = read_game_name(e.path);
    wchar_t label[420];
    if (!gname.empty())
      swprintf(label, 420, L"%ls  —  «%ls»  —  %02d.%02d.%04d %02d:%02d", e.name.c_str(), gname.c_str(), st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute);
    else
      swprintf(label, 420, L"%ls  —  %02d.%02d.%04d %02d:%02d", e.name.c_str(), st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute);
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)label);
    g_savePaths.push_back(e.path);
    if (!preselect.empty() && _wcsicmp(e.path.c_str(), preselect.c_str())==0) sel = (int)g_savePaths.size()-1;
  }
  if (!preselect.empty() && sel<0){
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)preselect.c_str());
    g_savePaths.push_back(preselect);
    sel = (int)g_savePaths.size()-1;
  }
  if (sel>=0) SendMessageW(hCombo, CB_SETCURSEL, sel, 0);
}

static void refresh_list(){
  ListView_DeleteAllItems(hList);
  g_updatingChecks = true;
  for (size_t k=0;k<g_fixes.size();k++){
    Fix& f = g_fixes[k];
    LVITEMW it{}; it.mask=LVIF_TEXT; it.iItem=(int)k;
    it.pszText=(LPWSTR)f.Title().c_str();
    ListView_InsertItem(hList,&it);
    std::wstring var = towide(f.var);
    ListView_SetItemText(hList,(int)k,1,(LPWSTR)var.c_str());
    wchar_t cur[32]=L"—", tgt[32];
    swprintf(tgt,32,L"%d",f.target);
    std::wstring stw;
    if (!g_loaded) stw=L"";
    else if (f.off<0) stw=T(S_ST_NOTFOUND);
    else { swprintf(cur,32,L"%d",f.current); stw = (f.current==f.target)? T(S_ST_ALREADY) : T(S_ST_APPLICABLE); }
    ListView_SetItemText(hList,(int)k,2,cur);
    ListView_SetItemText(hList,(int)k,3,tgt);
    ListView_SetItemText(hList,(int)k,4,(LPWSTR)stw.c_str());
    ListView_SetCheckState(hList,(int)k, g_loaded && f.off>=0 && f.current!=f.target);
  }
  g_updatingChecks = false;
}

static void relabel_ui(){
  SetWindowTextW(hMain, T(S_TITLE));
  SetWindowTextW(hBrowse, T(S_BROWSE));
  SetWindowTextW(hRefresh, T(S_REFRESH));
  SetWindowTextW(hApply, T(S_APPLY));
  const StrId cols[5] = {S_COL_FIX, S_COL_VAR, S_COL_CUR, S_COL_NEW, S_COL_STATUS};
  for (int k=0;k<5;k++){
    LVCOLUMNW c{}; c.mask=LVCF_TEXT; c.pszText=(LPWSTR)T(cols[k]);
    ListView_SetColumn(hList, k, &c);
  }
  SetWindowTextW(hDesc, L"");
  if (g_loaded){
    int applicable=0;
    for (auto& f : g_fixes) if (f.off>=0 && f.current!=f.target) applicable++;
    wchar_t st[200]; swprintf(st,200,T(S_LOADED_FMT),applicable);
    set_status(st);
  } else {
    set_status(g_savePaths.empty()? T(S_NO_SAVES) : T(S_SELECT_SAVE));
  }
  refresh_list();
}

static void load_save(const std::wstring& path){
  g_loaded=false;
  HCURSOR old = SetCursor(LoadCursor(NULL, IDC_WAIT));
  set_status(T(S_OPENING));
  UpdateWindow(hStatus);
  do{
    g_meta = SaveMeta();
    if (!load_file(path, g_meta.data)){ set_status(T(S_CANT_OPEN)); break; }
    int err = parse_save(g_meta);
    if (err){ set_status(T((StrId)err)); break; }
    err = decompress_all(g_meta, g_payload);
    if (err){ set_status(T((StrId)err)); break; }
    for (auto& f : g_fixes){
      f.off = find_var(g_payload, f.var);
      if (f.off>=0) f.current = (int32)rd32(&g_payload[f.off]);
    }
    g_loaded = true;
    int applicable = 0;
    for (auto& f : g_fixes) if (f.off>=0 && f.current!=f.target) applicable++;
    wchar_t st[200];
    swprintf(st,200,T(S_LOADED_FMT),applicable);
    set_status(st);
  } while(0);
  refresh_list();
  EnableWindow(hApply, g_loaded);
  SetCursor(old);
}

static std::wstring current_path(){
  int sel = (int)SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
  return (sel>=0 && sel<(int)g_savePaths.size()) ? g_savePaths[sel] : L"";
}

static void apply_fixes(){
  if (!g_loaded) return;
  std::wstring path = current_path();
  std::vector<std::pair<long,int32>> edits;
  std::wstring names;
  for (size_t k=0;k<g_fixes.size();k++){
    Fix& f = g_fixes[k];
    if (ListView_GetCheckState(hList,(int)k) && f.off>=0 && f.current!=f.target){
      edits.push_back({f.off, f.target});
      names += L"  • " + f.Title() + L"\n";
    }
  }
  if (edits.empty()){ MessageBoxW(hMain, T(S_MB_NONE_SELECTED), T(S_INFO_TITLE), MB_ICONINFORMATION); return; }

  std::wstring q = std::wstring(T(S_MB_CONFIRM_HEAD)) + names + T(S_MB_CONFIRM_TAIL);
  if (MessageBoxW(hMain, q.c_str(), T(S_MB_CONFIRM_TITLE), MB_OKCANCEL|MB_ICONQUESTION) != IDOK) return;

  HCURSOR old = SetCursor(LoadCursor(NULL, IDC_WAIT));
  set_status(T(S_BUILDING));
  UpdateWindow(hStatus);

  std::vector<byte> out = rebuild(g_meta, g_payload, edits);

  {
    SaveMeta m2; m2.data = out;
    std::vector<byte> pl2;
    int err = parse_save(m2);
    if (!err) err = decompress_all(m2, pl2);
    bool valfail = false;
    if (!err)
      for (auto& e : edits)
        if ((int32)rd32(&pl2[e.first]) != e.second){ valfail = true; break; }
    if (err || valfail){
      SetCursor(old);
      std::wstring msg = std::wstring(T(S_VERIFY_FAIL_HEAD)) + (err? T((StrId)err) : L"") + T(S_VERIFY_FAIL_TAIL);
      MessageBoxW(hMain, msg.c_str(), T(S_ERR_TITLE), MB_ICONERROR);
      set_status(T(S_VERIFY_FAIL_ST));
      return;
    }
  }

  std::wstring bak = path + L".backup";
  if (file_exists(bak)){
    wchar_t suf[32]; swprintf(suf,32,L".backup%u",(unsigned)(time(NULL)%1000000));
    bak = path + suf;
  }
  if (!save_file(bak, g_meta.data)){
    SetCursor(old);
    MessageBoxW(hMain, T(S_CANT_BACKUP), T(S_ERR_TITLE), MB_ICONERROR);
    set_status(T(S_CANT_BACKUP_ST));
    return;
  }
  if (!save_file(path, out)){
    SetCursor(old);
    wchar_t msg[600]; swprintf(msg,600,T(S_CANT_WRITE_FMT),bak.c_str());
    MessageBoxW(hMain, msg, T(S_ERR_TITLE), MB_ICONERROR);
    set_status(T(S_CANT_WRITE_ST));
    return;
  }
  SetCursor(old);
  wchar_t done[700]; swprintf(done,700,T(S_DONE_FMT),(int)edits.size(),bak.c_str());
  MessageBoxW(hMain, done, T(S_INFO_TITLE), MB_ICONINFORMATION);
  load_save(path);
}

static void browse(){
  wchar_t file[MAX_PATH] = L"";
  OPENFILENAMEW ofn{}; ofn.lStructSize=sizeof ofn;
  ofn.hwndOwner=hMain;
  ofn.lpstrFilter=T(S_FILTER);
  ofn.lpstrFile=file; ofn.nMaxFile=MAX_PATH;
  std::wstring dir = save_dir();
  ofn.lpstrInitialDir = dir.empty()? NULL : dir.c_str();
  ofn.Flags=OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
  if (GetOpenFileNameW(&ofn)){
    scan_saves(file);
    load_save(file);
  }
}

static void layout(){
  RECT rc; GetClientRect(hMain,&rc);
  int W=rc.right, H=rc.bottom, M=S(12);
  int langW=S(110), browseW=S(90), refreshW=S(120);
  int comboW = W - M*2 - browseW - refreshW - langW - S(18);
  int x=M;
  MoveWindow(hCombo,  x, M, comboW, S(200), TRUE); x+=comboW+S(6);
  MoveWindow(hBrowse, x, M-S(1), browseW, S(26), TRUE); x+=browseW+S(6);
  MoveWindow(hRefresh,x, M-S(1), refreshW, S(26), TRUE); x+=refreshW+S(6);
  MoveWindow(hLang,   x, M, langW, S(200), TRUE);
  int listTop = M + S(34);
  int descH = S(40), btnH = S(30), statH = S(20);
  int listH = H - listTop - descH - btnH - statH - M*3;
  MoveWindow(hList, M, listTop, W-M*2, listH, TRUE);
  MoveWindow(hDesc, M, listTop+listH+S(6), W-M*2, descH, TRUE);
  MoveWindow(hApply, W-M-S(230), listTop+listH+descH+S(10), S(230), btnH, TRUE);
  MoveWindow(hStatus, M, listTop+listH+descH+S(14), W-M*2-S(240), statH+S(8), TRUE);
}

static LRESULT CALLBACK WndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp){
  switch(msg){
  case WM_CREATE:{
    hMain = h;
    HDC dc = GetDC(h); g_dpi = GetDeviceCaps(dc, LOGPIXELSX); ReleaseDC(h,dc);
    NONCLIENTMETRICSW ncm{sizeof ncm};
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof ncm, &ncm, 0);
    hFont = CreateFontIndirectW(&ncm.lfMessageFont);
    LOGFONTW bf = ncm.lfMessageFont; bf.lfWeight = FW_SEMIBOLD;
    hFontBold = CreateFontIndirectW(&bf);

    hCombo = CreateWindowW(L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL,
      0,0,0,0, h,(HMENU)IDC_COMBO,NULL,NULL);
    hBrowse = CreateWindowW(L"BUTTON", L"", WS_CHILD|WS_VISIBLE|WS_TABSTOP, 0,0,0,0, h,(HMENU)IDC_BROWSE,NULL,NULL);
    hRefresh = CreateWindowW(L"BUTTON", L"", WS_CHILD|WS_VISIBLE|WS_TABSTOP, 0,0,0,0, h,(HMENU)IDC_REFRESH,NULL,NULL);
    hLang = CreateWindowW(L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST,
      0,0,0,0, h,(HMENU)IDC_LANG,NULL,NULL);
    SendMessageW(hLang, CB_ADDSTRING, 0, (LPARAM)L"Русский");
    SendMessageW(hLang, CB_ADDSTRING, 0, (LPARAM)L"English");
    SendMessageW(hLang, CB_SETCURSEL, g_lang, 0);
    hList = CreateWindowW(WC_LISTVIEWW, L"", WS_CHILD|WS_VISIBLE|WS_BORDER|LVS_REPORT|LVS_SINGLESEL|WS_TABSTOP,
      0,0,0,0, h,(HMENU)IDC_LIST,NULL,NULL);
    ListView_SetExtendedListViewStyle(hList, LVS_EX_CHECKBOXES|LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);
    hDesc = CreateWindowW(L"STATIC", L"", WS_CHILD|WS_VISIBLE, 0,0,0,0, h,(HMENU)IDC_DESC,NULL,NULL);
    hStatus = CreateWindowW(L"STATIC", L"", WS_CHILD|WS_VISIBLE, 0,0,0,0, h,(HMENU)IDC_STATUS,NULL,NULL);
    hApply = CreateWindowW(L"BUTTON", L"", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,
      0,0,0,0, h,(HMENU)IDC_APPLY,NULL,NULL);
    EnableWindow(hApply, FALSE);
    for (HWND w : {hCombo,hBrowse,hRefresh,hLang,hList,hDesc,hStatus}) SendMessageW(w,WM_SETFONT,(WPARAM)hFont,TRUE);
    SendMessageW(hApply,WM_SETFONT,(WPARAM)hFontBold,TRUE);

    LVCOLUMNW c{}; c.mask=LVCF_TEXT|LVCF_WIDTH;
    const StrId colid[5]={S_COL_FIX,S_COL_VAR,S_COL_CUR,S_COL_NEW,S_COL_STATUS};
    const int colw[5]={S(330),S(230),S(64),S(60),S(140)};
    for (int k=0;k<5;k++){ c.pszText=(LPWSTR)T(colid[k]); c.cx=colw[k]; ListView_InsertColumn(hList,k,&c); }

    g_fixes = load_fixes();
    relabel_ui();
    scan_saves(L"");
    DragAcceptFiles(h, TRUE);
    if (!g_savePaths.empty()) load_save(g_savePaths[0]);
    else set_status(T(S_NO_SAVES));
    return 0;
  }
  case WM_SIZE: layout(); return 0;
  case WM_GETMINMAXINFO:{
    MINMAXINFO* mm=(MINMAXINFO*)lp;
    mm->ptMinTrackSize.x=S(700); mm->ptMinTrackSize.y=S(380);
    return 0;
  }
  case WM_DROPFILES:{
    wchar_t file[MAX_PATH];
    if (DragQueryFileW((HDROP)wp, 0, file, MAX_PATH)){
      scan_saves(file);
      load_save(file);
    }
    DragFinish((HDROP)wp);
    return 0;
  }
  case WM_COMMAND:
    switch (LOWORD(wp)){
    case IDC_BROWSE: browse(); return 0;
    case IDC_REFRESH: scan_saves(current_path()); return 0;
    case IDC_APPLY: apply_fixes(); return 0;
    case IDC_COMBO:
      if (HIWORD(wp)==CBN_SELCHANGE){ std::wstring p=current_path(); if(!p.empty()) load_save(p); }
      return 0;
    case IDC_LANG:
      if (HIWORD(wp)==CBN_SELCHANGE){
        int sel = (int)SendMessageW(hLang, CB_GETCURSEL, 0, 0);
        if (sel==LRU || sel==LEN){ g_lang = sel; save_lang(); relabel_ui(); }
      }
      return 0;
    }
    break;
  case WM_NOTIFY:{
    NMHDR* nm=(NMHDR*)lp;
    if (nm->idFrom==IDC_LIST && nm->code==LVN_ITEMCHANGED && !g_updatingChecks){
      NMLISTVIEW* lv=(NMLISTVIEW*)lp;
      if ((lv->uNewState & LVIS_SELECTED) && lv->iItem>=0 && lv->iItem<(int)g_fixes.size())
        SetWindowTextW(hDesc, g_fixes[lv->iItem].Desc().c_str());
      bool wasCheck = (lv->uOldState & LVIS_STATEIMAGEMASK) != (lv->uNewState & LVIS_STATEIMAGEMASK);
      if (wasCheck && lv->iItem>=0 && lv->iItem<(int)g_fixes.size()){
        Fix& f = g_fixes[lv->iItem];
        bool nowChecked = ((lv->uNewState & LVIS_STATEIMAGEMASK) >> 12) == 2;
        if (nowChecked && (!g_loaded || f.off<0 || f.current==f.target)){
          g_updatingChecks=true;
          ListView_SetCheckState(hList, lv->iItem, FALSE);
          g_updatingChecks=false;
          if (g_loaded) set_status(f.off<0 ? T(S_NOT_APPLICABLE_ST) : T(S_ALREADY_FIXED_ST));
        }
      }
    }
    break;
  }
  case WM_DESTROY: PostQuitMessage(0); return 0;
  }
  return DefWindowProcW(h,msg,wp,lp);
}

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE, LPWSTR, int nShow){
  load_lang();
  INITCOMMONCONTROLSEX icc{sizeof icc, ICC_LISTVIEW_CLASSES};
  InitCommonControlsEx(&icc);
  WNDCLASSW wc{};
  wc.lpfnWndProc=WndProc; wc.hInstance=hi;
  wc.hCursor=LoadCursor(NULL,IDC_ARROW);
  wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);
  wc.lpszClassName=L"G1RFixTool";
  wc.hIcon=LoadIcon(NULL,IDI_APPLICATION);
  RegisterClassW(&wc);
  HWND h = CreateWindowW(L"G1RFixTool", T(S_TITLE),
    WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 920, 480, NULL, NULL, hi, NULL);
  ShowWindow(h,nShow); UpdateWindow(h);
  MSG m;
  while (GetMessageW(&m,NULL,0,0)>0){
    if (!IsDialogMessageW(h,&m)){ TranslateMessage(&m); DispatchMessageW(&m); }
  }
  return 0;
}
