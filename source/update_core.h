#ifndef LOL_UPDATE_CORE_H
#define LOL_UPDATE_CORE_H
#include <string.h>
#include <stddef.h>
#include <wchar.h>

/* Strict numeric release tags: v1.0, 1.0.1 or v1.0.1.2. */
static inline int parseVersion(const char *s,unsigned v[4]){
 if(*s=='v'||*s=='V')s++;
 for(int i=0;i<4;i++)v[i]=0;
 int count=0;
 while(*s){
  if(count==4||*s<'0'||*s>'9')return 0;
  unsigned n=0;
  do{n=n*10+(unsigned)(*s++-'0');if(n>1000000)return 0;}while(*s>='0'&&*s<='9');
  v[count++]=n;
  if(!*s)break;
  if(*s++!='.'||!*s)return 0;
 }
 return count>=2;
}
static inline int newerVersion(const char *remote,const char *local){
 unsigned a[4],b[4];if(!parseVersion(remote,a)||!parseVersion(local,b))return 0;
 for(int i=0;i<4;i++)if(a[i]!=b[i])return a[i]>b[i];
 return 0;
}
static inline int validRepository(const wchar_t *s){
 size_t n=wcslen(s);if(n<3||n>140)return 0;
 const wchar_t *slash=wcschr(s,L'/');if(!slash||slash==s||!slash[1]||wcschr(slash+1,L'/'))return 0;
 if(slash-s>39||wcslen(slash+1)>100)return 0;
 for(const wchar_t *p=s;*p;p++){
  wchar_t c=*p;
  if(c==L'/')continue;
  if(!((c>=L'a'&&c<=L'z')||(c>=L'A'&&c<=L'Z')||(c>=L'0'&&c<=L'9')||c==L'-'||(p>slash&&(c==L'_'||c==L'.'))))return 0;
 }
 if(s[0]==L'-'||slash[-1]==L'-'||!wcscmp(slash+1,L".")||!wcscmp(slash+1,L".."))return 0;
 return 1;
}
/* A bounded JSON reader; only root release fields are considered. */
typedef struct{const char *p,*end;} UpdateJson;
static inline void jsonSpace(UpdateJson *j){while(j->p<j->end&&(*j->p==' '||*j->p=='\t'||*j->p=='\r'||*j->p=='\n'))j->p++;}
static inline int jsonString(UpdateJson *j,char *out,size_t cap){
 if(j->p==j->end||*j->p++!='"')return 0;
 size_t n=0;int overflow=0;
 while(j->p<j->end){
  unsigned char c=(unsigned char)*j->p++;
  if(c=='"'){if(out){out[n]=0;if(overflow)return 0;}return 1;}
  if(c<32)return 0;
  if(c=='\\'){
   if(j->p==j->end)return 0;
   c=(unsigned char)*j->p++;
   if(c=='u'){
    unsigned code=0;for(int k=0;k<4;k++){
     if(j->p==j->end)return 0;
     unsigned char h=(unsigned char)*j->p++;unsigned d;
     if(h>='0'&&h<='9')d=h-'0';else if(h>='a'&&h<='f')d=h-'a'+10;else if(h>='A'&&h<='F')d=h-'A'+10;else return 0;
     code=code*16+d;
    }
    c=code>127?'?':(unsigned char)code;
   }else if(c=='n')c='\n';else if(c=='r')c='\r';else if(c=='t')c='\t';else if(c=='b')c='\b';else if(c=='f')c='\f';else if(c!='"'&&c!='\\'&&c!='/')return 0;
  }
  if(out){if(!c)overflow=1;if(n+1<cap)out[n++]=(char)c;else overflow=1;}
 }
 return 0;
}
static inline int jsonLiteral(UpdateJson *j,const char *s){size_t n=strlen(s);if((size_t)(j->end-j->p)<n||memcmp(j->p,s,n))return 0;j->p+=n;return 1;}
static int jsonValue(UpdateJson *j,int depth){
 jsonSpace(j);if(depth>32||j->p==j->end)return 0;
 char c=*j->p;
 if(c=='"')return jsonString(j,NULL,0);
 if(c=='{'||c=='['){
  j->p++;jsonSpace(j);char close=c=='{'?'}':']';
  if(j->p<j->end&&*j->p==close){j->p++;return 1;}
  for(;;){
   if(c=='{'){if(!jsonString(j,NULL,0))return 0;
   jsonSpace(j);if(j->p==j->end||*j->p++!=':')return 0;}
   if(!jsonValue(j,depth+1))return 0;
   jsonSpace(j);if(j->p==j->end)return 0;
   if(*j->p==close){j->p++;return 1;}
   if(*j->p++!=',')return 0;
   jsonSpace(j);
  }
 }
 if(c=='t')return jsonLiteral(j,"true");
 if(c=='f')return jsonLiteral(j,"false");
 if(c=='n')return jsonLiteral(j,"null");
 if(c=='-')j->p++;
 if(j->p==j->end)return 0;
 if(*j->p=='0')j->p++;else{if(*j->p<'1'||*j->p>'9')return 0;while(j->p<j->end&&*j->p>='0'&&*j->p<='9')j->p++;}
 if(j->p<j->end&&*j->p=='.'){j->p++;const char *s=j->p;while(j->p<j->end&&*j->p>='0'&&*j->p<='9')j->p++;if(s==j->p)return 0;}
 if(j->p<j->end&&(*j->p=='e'||*j->p=='E')){j->p++;if(j->p<j->end&&(*j->p=='+'||*j->p=='-'))j->p++;const char *s=j->p;while(j->p<j->end&&*j->p>='0'&&*j->p<='9')j->p++;if(s==j->p)return 0;}
 return 1;
}
static inline int parseRelease(const char *data,size_t length,char tag[64]){
 UpdateJson j={data,data+length};int seenTag=0,seenDraft=0,seenPre=0;jsonSpace(&j);
 if(j.p==j.end||*j.p++!='{')return 0;
  jsonSpace(&j);
 for(;;){
  char key[256];if(!jsonString(&j,key,sizeof(key)))return 0;
  jsonSpace(&j);
  if(j.p==j.end||*j.p++!=':')return 0;
  jsonSpace(&j);
  if(!strcmp(key,"tag_name")){if(seenTag++||!jsonString(&j,tag,64))return 0;}
  else if(!strcmp(key,"draft")){if(seenDraft++||!jsonLiteral(&j,"false"))return 0;}
  else if(!strcmp(key,"prerelease")){if(seenPre++||!jsonLiteral(&j,"false"))return 0;}
  else if(!jsonValue(&j,1))return 0;
  jsonSpace(&j);if(j.p==j.end)return 0;
  if(*j.p=='}'){j.p++;break;}
  if(*j.p++!=',')return 0;
  jsonSpace(&j);
 }
 jsonSpace(&j);unsigned v[4];return j.p==j.end&&seenTag&&seenDraft&&seenPre&&parseVersion(tag,v);
}
#endif
