#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "../update_core.h"
static int release(const char *s){char tag[64];return parseRelease(s,strlen(s),tag);}
int main(void){
 assert(newerVersion("v1.1","1.0"));assert(newerVersion("v1.0.1","1.0"));
 assert(newerVersion("v1.10","1.9"));assert(newerVersion("v2.0","1.99.99"));
 assert(!newerVersion("v1.0.0","1.0"));assert(!newerVersion("v0.9","1.0"));
 assert(!newerVersion("v1.1-beta","1.0"));assert(!newerVersion("1.1+build","1.0"));
 assert(!newerVersion("1.1/../../evil","1.0"));assert(!newerVersion("1.9999999999999999","1.0"));
 assert(!newerVersion("1.1.1.1.1","1.0"));assert(!newerVersion("1.","1.0"));
 assert(validRepository(L"owner/LOL-Skin-Information-Searching"));
 assert(validRepository(L"abc123/my_repo.test"));
 const wchar_t *bad[]={L"",L"owner",L"/repo",L"owner/",L"owner/../../repo",L"owner/repo?x",L"owner/repo#x",L"https://github.com/owner/repo",L"owner/..",L"owner/repo\r\n",L"-owner/repo"};
 for(size_t i=0;i<sizeof(bad)/sizeof(*bad);i++)assert(!validRepository(bad[i]));
 assert(release("{\"tag_name\":\"v1.1\",\"draft\":false,\"prerelease\":false}"));
 assert(release("{\"body\":\"tag_name: v99.0 \\\"\",\"assets\":[{\"tag_name\":\"v999.0\"}],\"tag_name\":\"v1.0.1\",\"draft\":false,\"prerelease\":false}"));
 assert(release("{\"tag_name\":\"v1.\\u0031\",\"draft\":false,\"prerelease\":false}"));
 assert(!release("{\"tag_name\":\"v1.1\",\"draft\":true,\"prerelease\":false}"));
 assert(!release("{\"tag_name\":\"v1.1\",\"draft\":false,\"prerelease\":true}"));
 assert(!release("{\"tag_name\":\"v1.1\"}"));
 assert(!release("{\"tag_name\":\"v1.1\",\"draft\":false,\"prerelease\":false,\"tag_name\":\"v9.0\"}"));
 assert(!release("{\"tag_name\":\"v1.1\\u0000evil\",\"draft\":false,\"prerelease\":false}"));
 assert(!release("{\"tag_name\":\"v1.1\",\"draft\":false,\"prerelease\":false}evil"));
 assert(!release("{\"tag_name\":\"v1.1\",\"draft\":false,\"prerelease\":false,\"x\":01}"));
 const char *s="{\"body\":[1,true,false,null,{},[1.5e-2]],\"tag_name\":\"v1.1\",\"draft\":false,\"prerelease\":false}";
 assert(release(s));char tag[64];
 for(size_t n=0;n<strlen(s);n++)assert(!parseRelease(s,n,tag));
 /* Fixed-seed malformed byte inputs exercise bounds without network access. */
 unsigned state=137;char buffer[512];
 for(int i=0;i<20000;i++){
  size_t len=(size_t)i%sizeof(buffer);
  for(size_t k=0;k<len;k++){state=state*1664525+1013904223;buffer[k]=(char)(state>>24);}
  (void)parseRelease(buffer,len,tag);
 }
 puts("PASS: version ordering, repository validation, stable release JSON and malformed input bounds");
 return 0;
}
