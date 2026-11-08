#ifndef LOL_QUERY_CORE_H
#define LOL_QUERY_CORE_H
#include <wchar.h>
#include <stddef.h>
typedef struct {
 const wchar_t *id,*name,*hero,*quality,*price,*date,*access,*limited,*note;
 const wchar_t *search,*detail,*source;
 const wchar_t **cells;
} Skin;
static int separator(wchar_t c) {
 return c==L' '||c==L'\t'||c==L'\r'||c==L'\n'||c==0x3000;
}
static int ignored(wchar_t c) {
 return separator(c)||c==L'/'||c==L'\\'||c==L':'||c==0xFF1A||c==L'-'||c==L'_'||c==0x00B7;
}
static void normalize(const wchar_t *s,wchar_t *out,size_t cap) {
 size_t n=0;
 for(;*s && n+1<cap;s++) if(!ignored(*s)) {
  wchar_t c=*s;
  if(c>=L'A'&&c<=L'Z') c+=L'a'-L'A';
  out[n++]=c;
 }
 out[n]=0;
}
static int skinMatches(const Skin *skin,const wchar_t *query) {
 wchar_t word[512],norm[512];
 const wchar_t *p=query;while(separator(*p))p++;
 const wchar_t *digits=p;while(*p>=L'0'&&*p<=L'9')p++;
 if(p>digits){const wchar_t *tail=p;while(separator(*tail))tail++;if(!*tail){size_t n=(size_t)(p-digits);return wcslen(skin->id)==n&&!wcsncmp(skin->id,digits,n);}}
 while(*query) {
  while(separator(*query))query++;
  size_t n=0;
  while(*query&&!separator(*query)) { if(n+1<512)word[n++]=*query; query++; }
  word[n]=0;
  normalize(word,norm,512);
  if(norm[0]&&!wcsstr(skin->search,norm)) return 0;
 }
 return 1;
}
#endif
