#ifndef LOL_UPDATER_H
#define LOL_UPDATER_H
#include <winhttp.h>
#include <shlobj.h>
#include <stdlib.h>
#include "update_core.h"
#include "update_config.h"

enum{UPDATE_FAILED,UPDATE_CURRENT,UPDATE_NEW,UPDATE_NOT_FOUND,UPDATE_LIMITED,UPDATE_INVALID};
typedef struct{wchar_t repository[160];char tag[64];int status,manual;} UpdateJob;
static HANDLE updateThread;
static UpdateJob *updateJob;
static wchar_t repository[160],settingsFile[MAX_PATH];

static void loadUpdateSettings(void){
 wchar_t folder[MAX_PATH];settingsFile[0]=0;
 if(SUCCEEDED(SHGetFolderPathW(NULL,CSIDL_LOCAL_APPDATA,NULL,SHGFP_TYPE_CURRENT,folder))){
  if(wcslen(folder)+65<MAX_PATH){
   swprintf(settingsFile,MAX_PATH,L"%ls\\LOL-Skin-Information-Searching\\settings.ini",folder);
  }
 }
 if(settingsFile[0])GetPrivateProfileStringW(L"Updates",L"Repository",DEFAULT_GITHUB_REPOSITORY,repository,160,settingsFile);
 else lstrcpynW(repository,DEFAULT_GITHUB_REPOSITORY,160);
}
static int saveUpdateSettings(void){
 if(!settingsFile[0])return 0;
 wchar_t folder[MAX_PATH];lstrcpynW(folder,settingsFile,MAX_PATH);wchar_t *last=wcsrchr(folder,L'\\');if(!last)return 0;*last=0;
 if(!CreateDirectoryW(folder,NULL)&&GetLastError()!=ERROR_ALREADY_EXISTS)return 0;
 return WritePrivateProfileStringW(L"Updates",L"Repository",repository,settingsFile)!=0;
}
static DWORD WINAPI checkUpdateWorker(void *param){
 UpdateJob *job=(UpdateJob*)param;HINTERNET session=NULL,connection=NULL,request=NULL;char *data=NULL;
 job->status=UPDATE_FAILED;
 session=WinHttpOpen(L"LOL-Skin-Information-Searching/" APP_VERSION_W,WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);
 if(!session)goto done;
 if(!WinHttpSetTimeouts(session,5000,5000,5000,5000))goto done;
 connection=WinHttpConnect(session,L"api.github.com",INTERNET_DEFAULT_HTTPS_PORT,0);if(!connection)goto done;
 wchar_t path[256];swprintf(path,256,L"/repos/%ls/releases/latest",job->repository);
 request=WinHttpOpenRequest(connection,L"GET",path,NULL,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE);if(!request)goto done;
 DWORD disable=WINHTTP_DISABLE_REDIRECTS;WinHttpSetOption(request,WINHTTP_OPTION_DISABLE_FEATURE,&disable,sizeof(disable));
 const wchar_t *headers=L"Accept: application/vnd.github+json\r\nX-GitHub-Api-Version: 2026-03-10\r\n";
 if(!WinHttpSendRequest(request,headers,(DWORD)-1L,WINHTTP_NO_REQUEST_DATA,0,0,0)||!WinHttpReceiveResponse(request,NULL))goto done;
 DWORD code=0,size=sizeof(code);
 if(!WinHttpQueryHeaders(request,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&code,&size,WINHTTP_NO_HEADER_INDEX))goto done;
 if(code==404){job->status=UPDATE_NOT_FOUND;goto done;}
 if(code==403||code==429){job->status=UPDATE_LIMITED;goto done;}
 if(code!=200)goto done;
 const DWORD limit=512*1024;DWORD used=0,got=0;ULONGLONG deadline=GetTickCount64()+20000;
 data=(char*)malloc(limit+1);if(!data)goto done;
 for(;;){
  if(GetTickCount64()>deadline)goto done;
  if(used==limit){job->status=UPDATE_INVALID;goto done;}
  DWORD want=limit-used;if(want>8192)want=8192;
  if(!WinHttpReadData(request,data+used,want,&got))goto done;
  if(!got)break;used+=got;
 }
 data[used]=0;
 if(!parseRelease(data,used,job->tag)){job->status=UPDATE_INVALID;goto done;}
 job->status=newerVersion(job->tag,APP_VERSION)?UPDATE_NEW:UPDATE_CURRENT;
done:
 free(data);if(request)WinHttpCloseHandle(request);if(connection)WinHttpCloseHandle(connection);if(session)WinHttpCloseHandle(session);return 0;
}
static void startUpdateCheck(HWND window,int manual){
 if(updateThread)return;
 if(!validRepository(repository)){if(manual)MessageBoxW(window,L"请先在更新设置中填写 GitHub 仓库：owner/repository。",L"尚未配置仓库",MB_OK|MB_ICONINFORMATION);return;}
 updateJob=(UpdateJob*)calloc(1,sizeof(UpdateJob));if(!updateJob){if(manual)MessageBoxW(window,L"内存不足，请稍后重试。",L"检查更新",MB_OK|MB_ICONINFORMATION);return;}
 lstrcpynW(updateJob->repository,repository,160);updateJob->manual=manual;
 updateThread=CreateThread(NULL,0,checkUpdateWorker,updateJob,0,NULL);
 if(!updateThread){free(updateJob);updateJob=NULL;if(manual)MessageBoxW(window,L"更新检查未能启动，请稍后重试。",L"检查更新",MB_OK|MB_ICONINFORMATION);return;}
 SetWindowTextW(hUpdate,L"正在检查…");EnableWindow(hUpdate,FALSE);SetTimer(window,3,200,NULL);
}
static void pollUpdateCheck(HWND window){
 if(!updateThread||WaitForSingleObject(updateThread,0)!=WAIT_OBJECT_0)return;
 KillTimer(window,3);CloseHandle(updateThread);updateThread=NULL;
 UpdateJob job=*updateJob;free(updateJob);updateJob=NULL;
 SetWindowTextW(hUpdate,L"更新设置");EnableWindow(hUpdate,TRUE);
 if(job.status==UPDATE_NEW){
  wchar_t tag[64],message[512],url[384];MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,job.tag,-1,tag,64);
  swprintf(message,512,L"发现新版本 %ls（当前版本 " APP_VERSION_W L"）。\n\n是否打开 GitHub 版本页面下载？",tag);
  if(MessageBoxW(window,message,L"发现新版本",MB_YESNO|MB_ICONINFORMATION)==IDYES){
   swprintf(url,384,L"https://github.com/%ls/releases/tag/%ls",job.repository,tag);
   if((INT_PTR)ShellExecuteW(window,L"open",url,NULL,NULL,SW_SHOWNORMAL)<=32)MessageBoxW(window,L"浏览器未能打开，请手动访问该仓库的 Releases 页面。",L"打开下载页面",MB_OK|MB_ICONINFORMATION);
  }
 }else if(job.manual){
  const wchar_t *text=job.status==UPDATE_CURRENT?L"当前已是最新版本。":job.status==UPDATE_NOT_FOUND?L"未找到公开正式版本。请确认仓库地址，并在 GitHub Releases 中发布版本；仅上传源码或创建标签不会生成正式版本。":job.status==UPDATE_LIMITED?L"GitHub 暂时限制了请求，请稍后重试。":job.status==UPDATE_INVALID?L"版本信息格式不受支持。正式版本标签应为 v1.0、v1.0.1 等数字版本。":L"暂时未能连接 GitHub，请检查网络后重试。";
  MessageBoxW(window,text,L"检查更新",MB_OK|MB_ICONINFORMATION);
 }
}
static INT_PTR CALLBACK updateSettingsDialog(HWND dialog,UINT msg,WPARAM wp,LPARAM lp){
 (void)lp;
 if(msg==WM_INITDIALOG){SetDlgItemTextW(dialog,301,repository);SendDlgItemMessageW(dialog,301,EM_SETLIMITTEXT,240,0);return TRUE;}
 if(msg==WM_COMMAND&&LOWORD(wp)==IDCANCEL){EndDialog(dialog,IDCANCEL);return TRUE;}
 if(msg==WM_COMMAND&&LOWORD(wp)==IDOK){
  wchar_t raw[256];GetDlgItemTextW(dialog,301,raw,256);wchar_t *p=raw;while(*p==L' '||*p==L'\t')p++;
  size_t n=wcslen(p);while(n&&(p[n-1]==L' '||p[n-1]==L'\t'||p[n-1]==L'/'))p[--n]=0;
  if(!wcsncmp(p,L"https://github.com/",19))p+=19;
  n=wcslen(p);if(n>4&&!wcscmp(p+n-4,L".git"))p[n-4]=0;
  if(*p&&!validRepository(p)){MessageBoxW(dialog,L"请输入 owner/repository，或 https://github.com/owner/repository。",L"仓库地址有误",MB_OK|MB_ICONINFORMATION);return TRUE;}
  wchar_t old[160];lstrcpynW(old,repository,160);lstrcpynW(repository,p,160);
  if(!saveUpdateSettings()){lstrcpynW(repository,old,160);MessageBoxW(dialog,L"设置未能保存，请检查当前用户目录是否可写。",L"保存失败",MB_OK|MB_ICONINFORMATION);return TRUE;}
  EndDialog(dialog,IDOK);return TRUE;
 }
 return FALSE;
}
static void showUpdateSettings(HWND window){
 if(DialogBoxParamW(instance,MAKEINTRESOURCEW(300),window,updateSettingsDialog,0)==IDOK&&repository[0])startUpdateCheck(window,1);
}
#endif
