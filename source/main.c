#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <stdio.h>
#include <string.h>
#include "core.h"
#include "skins_data.h"

#define ID_QUERY 101
#define ID_RESULTS 102
#define ID_CLEAR 103
#define ID_COPY 104
#define ID_SOURCE 105
#define ID_ABOUT 106
#define ID_UPDATE 107
#define DETAIL_REFRESH (WM_APP+1)
static HWND hQuery,hList,hDetail,hTitle,hSearchLabel,hDetailLabel,hCopy,hClear,hResultsLabel,hUpdate;
static HFONT font,titleFont,boldFont,smallFont,sectionFont;
static HBRUSH pageBrush,navBrush,whiteBrush;
static HIMAGELIST rowSpacing;
static int selected=-1,scale=96;
static HINSTANCE instance;
static int detailScroll=0,detailTotal=0,detailHeights[64];
static const wchar_t *detailMessage=L"选择左侧皮肤，查看表格详情。";
static int px(int v){return MulDiv(v,scale,96);}
static HWND control(HWND parent,const wchar_t *cls,const wchar_t *text,DWORD style,int id,DWORD ex){
 if(!wcscmp(cls,L"BUTTON"))style|=BS_OWNERDRAW;
 HWND h=CreateWindowExW(ex,cls,text,WS_CHILD|WS_VISIBLE|style,0,0,0,0,parent,(HMENU)(INT_PTR)id,instance,NULL);
 SendMessageW(h,WM_SETFONT,(WPARAM)font,TRUE);return h;
}
#include "theme.h"
#include "updater.h"
/* Use the same wrapping pass for measurement and drawing, including long URLs. */
static int wrapped(HDC dc,const wchar_t *text,RECT rect,BOOL draw){
 TEXTMETRICW tm;GetTextMetricsW(dc,&tm);int line=tm.tmHeight+px(3),y=rect.top,width=rect.right-rect.left;
 const wchar_t *p=text;
 if(!*p)return line;
 while(*p){
  const wchar_t *start=p;int count=0,used=0;
  while(*p&&*p!=L'\r'&&*p!=L'\n'){
   int len=(*p>=0xD800&&*p<=0xDBFF&&p[1]>=0xDC00&&p[1]<=0xDFFF)?2:1;SIZE size;GetTextExtentPoint32W(dc,p,len,&size);
   if(count&&used+size.cx>width)break;
   used+=size.cx;count+=len;p+=len;
  }
  if(draw&&count)ExtTextOutW(dc,rect.left,y,ETO_CLIPPED,&rect,start,count,NULL);
  y+=line;
  if(*p==L'\r')p++;
  if(*p==L'\n')p++;
 }
 return y-rect.top;
}
static void measureDetail(HWND h){
 RECT rc;GetClientRect(h,&rc);HDC dc=GetDC(h);HGDIOBJ old=SelectObject(dc,font);
 int width=rc.right-px(136)-px(20);if(width<px(30))width=px(30);detailTotal=0;
 int count=selected>=0?DETAIL_COUNT:1;
 for(int i=0;i<count;i++){
  RECT value={0,0,width,100000},label={0,0,px(116),100000};
  const wchar_t *t=selected>=0?skins[selected].cells[i]:detailMessage;
  const wchar_t *k=selected>=0?detailLabels[i]:L"提示";
  int vh=wrapped(dc,t,value,FALSE),kh=wrapped(dc,k,label,FALSE);
  detailHeights[i]=(vh>kh?vh:kh)+px(20);if(detailHeights[i]<px(42))detailHeights[i]=px(42);
  detailTotal+=detailHeights[i];
 }
 SelectObject(dc,old);ReleaseDC(h,dc);
 int page=rc.bottom-px(34);if(page<1)page=1;int max=detailTotal-page;if(max<0)max=0;if(detailScroll>max)detailScroll=max;
 SCROLLINFO si={0};si.cbSize=sizeof(si);si.fMask=SIF_RANGE|SIF_PAGE|SIF_POS;si.nMin=0;si.nMax=detailTotal?detailTotal-1:0;si.nPage=page;si.nPos=detailScroll;SetScrollInfo(h,SB_VERT,&si,TRUE);
}
static LRESULT CALLBACK detailProcedure(HWND h,UINT msg,WPARAM wp,LPARAM lp){
 switch(msg){
 case WM_LBUTTONDOWN:SetFocus(h);return 0;
 case WM_KEYDOWN:{
  int code=-1;switch(wp){case VK_UP:code=SB_LINEUP;break;case VK_DOWN:code=SB_LINEDOWN;break;case VK_PRIOR:code=SB_PAGEUP;break;case VK_NEXT:code=SB_PAGEDOWN;break;case VK_HOME:code=SB_TOP;break;case VK_END:code=SB_BOTTOM;break;}
  if(code>=0){SendMessageW(h,WM_VSCROLL,code,0);return 0;}break;
 }
 case WM_SETFONT:case WM_SIZE:measureDetail(h);InvalidateRect(h,NULL,FALSE);return 0;
 case DETAIL_REFRESH:detailScroll=0;measureDetail(h);InvalidateRect(h,NULL,FALSE);return 0;
 case WM_ERASEBKGND:return 1;
 case WM_MOUSEWHEEL:{
  int delta=GET_WHEEL_DELTA_WPARAM(wp);detailScroll-=delta*px(60)/WHEEL_DELTA;if(detailScroll<0)detailScroll=0;measureDetail(h);InvalidateRect(h,NULL,FALSE);return 0;
 }
 case WM_VSCROLL:{
  SCROLLINFO si={0};si.cbSize=sizeof(si);si.fMask=SIF_ALL;GetScrollInfo(h,SB_VERT,&si);
  switch(LOWORD(wp)){case SB_LINEUP:detailScroll-=px(25);break;case SB_LINEDOWN:detailScroll+=px(25);break;case SB_PAGEUP:detailScroll-=(int)si.nPage;break;case SB_PAGEDOWN:detailScroll+=(int)si.nPage;break;case SB_THUMBTRACK:case SB_THUMBPOSITION:detailScroll=si.nTrackPos;break;case SB_TOP:detailScroll=0;break;case SB_BOTTOM:detailScroll=detailTotal;break;}
  if(detailScroll<0)detailScroll=0;measureDetail(h);InvalidateRect(h,NULL,FALSE);return 0;
 }
 case WM_PAINT:{
  PAINTSTRUCT ps;HDC target=BeginPaint(h,&ps);RECT rc;GetClientRect(h,&rc);
  if(rc.right<=0||rc.bottom<=0){EndPaint(h,&ps);return 0;}
  HDC dc=CreateCompatibleDC(target);HBITMAP bitmap=CreateCompatibleBitmap(target,rc.right,rc.bottom);HGDIOBJ oldBitmap=SelectObject(dc,bitmap),oldFont=SelectObject(dc,font);
  FillRect(dc,&rc,(HBRUSH)(COLOR_WINDOW+1));SetBkMode(dc,TRANSPARENT);
  int split=px(136),header=px(34),y=header-detailScroll,count=selected>=0?DETAIL_COUNT:1;
  SaveDC(dc);IntersectClipRect(dc,0,header,rc.right,rc.bottom);
  HBRUSH labelBrush=CreateSolidBrush(RGB(249,250,253));HPEN pen=CreatePen(PS_SOLID,1,RGB(233,237,243));HGDIOBJ oldPen=SelectObject(dc,pen);
  for(int i=0;i<count;i++){
   int height=detailHeights[i];if(y+height>=header&&y<rc.bottom){
    RECT bg={0,y,split,y+height};FillRect(dc,&bg,labelBrush);
    RECT key={px(10),y+px(10),split-px(10),y+height-px(5)},value={split+px(10),y+px(10),rc.right-px(10),y+height-px(5)};
    SetTextColor(dc,C_MUTED);wrapped(dc,selected>=0?detailLabels[i]:L"提示",key,TRUE);
    const wchar_t *text=selected>=0?skins[selected].cells[i]:detailMessage;
    if(selected>=0&&i==3)tierPill(dc,text,value);
    else {SetTextColor(dc,wcsstr(text,L"待核实")?RGB(157,112,40):C_TEXT);wrapped(dc,text,value,TRUE);}
    MoveToEx(dc,0,y+height-1,NULL);LineTo(dc,rc.right,y+height-1);MoveToEx(dc,split,y,NULL);LineTo(dc,split,y+height);
   }
   y+=height;
  }
  SelectObject(dc,oldPen);DeleteObject(pen);DeleteObject(labelBrush);RestoreDC(dc,-1);
  RECT hr={0,0,rc.right,header};HBRUSH navy=CreateSolidBrush(RGB(245,247,251));FillRect(dc,&hr,navy);DeleteObject(navy);
  SetTextColor(dc,C_MUTED);SelectObject(dc,smallFont);RECT a={px(10),0,split-px(10),header},b={split+px(10),0,rc.right,header};DrawTextW(dc,L"字段",-1,&a,DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);DrawTextW(dc,L"皮肤信息",-1,&b,DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);
  BitBlt(target,0,0,rc.right,rc.bottom,dc,0,0,SRCCOPY);SelectObject(dc,oldFont);SelectObject(dc,oldBitmap);DeleteObject(bitmap);DeleteDC(dc);EndPaint(h,&ps);return 0;
 }
 }
 return DefWindowProcW(h,msg,wp,lp);
}
static void showSelected(void){
 int row=ListView_GetNextItem(hList,-1,LVNI_SELECTED);
 if(row<0){selected=-1;detailMessage=L"选择左侧的一款皮肤，查看表格详情。支持皮肤名、英雄名、称号、英文英雄名及皮肤ID；多个关键词以空格分开。";}
 else {
  LVITEMW item={0};item.mask=LVIF_PARAM;item.iItem=row;
  if(ListView_GetItem(hList,&item)){selected=(int)item.lParam;}
 }
 EnableWindow(hCopy,selected>=0);
 SendMessageW(hDetail,DETAIL_REFRESH,0,0);
}
static void search(void){
 wchar_t query[512];GetWindowTextW(hQuery,query,512);
 selected=-1;SendMessageW(hList,WM_SETREDRAW,FALSE,0);ListView_DeleteAllItems(hList);
 int count=0;
 for(int i=0;i<SKIN_COUNT;i++)if(skinMatches(&skins[i],query)){
  const wchar_t *price=wcsstr(skins[i].price,L"原价待核实")?L"待核实":skins[i].price;
  const wchar_t *v[]={skins[i].id,skins[i].name,skins[i].hero,skins[i].quality,price};
  LVITEMW item={0};item.mask=LVIF_TEXT|LVIF_PARAM;item.iItem=count;item.pszText=(LPWSTR)v[0];item.lParam=i;
  int row=ListView_InsertItem(hList,&item);
  for(int col=1;col<5;col++)ListView_SetItemText(hList,row,col,(LPWSTR)v[col]);
  count++;
 }
 SendMessageW(hList,WM_SETREDRAW,TRUE,0);InvalidateRect(hList,NULL,TRUE);
 wchar_t countLabel[100];swprintf(countLabel,100,L"查询结果   %d",count);SetWindowTextW(hResultsLabel,countLabel);
 if(count){ListView_SetItemState(hList,0,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);showSelected();}
 else {showSelected();detailMessage=L"未找到匹配结果。试试更短的皮肤名、英雄名或称号，例如：奥拉夫、铁哥们儿、黑暗之女、KDA。清空搜索框可查看全部1970条皮肤记录。";SendMessageW(hDetail,DETAIL_REFRESH,0,0);}
}
static void layout(HWND window){
 RECT rc;GetClientRect(window,&rc);int w=rc.right,h=rc.bottom,m=px(28),gap=px(20),top=px(272),bottom=px(28);
 int left=(w-2*m-gap)*55/100,rightx=m+left+gap,right=w-m-rightx;
 MoveWindow(hTitle,px(94),px(35),w-px(295),px(36),TRUE);
 MoveWindow(hUpdate,w-m-px(132),px(34),px(132),px(40),TRUE);
 MoveWindow(hSearchLabel,m+px(18),px(136),w-2*m-px(160),px(20),TRUE);
 MoveWindow(hQuery,m+px(16),px(164),w-2*m-px(140),px(28),TRUE);
 MoveWindow(hClear,w-m-px(94),px(147),px(76),px(40),TRUE);
 MoveWindow(hResultsLabel,m,px(233),left,px(26),TRUE);
 MoveWindow(hList,m+1,top+1,left-2,h-top-bottom-2,TRUE);
 MoveWindow(hDetailLabel,rightx,px(235),right-px(128),px(26),TRUE);
 MoveWindow(hCopy,w-m-px(108),px(230),px(108),px(34),TRUE);
 MoveWindow(hDetail,rightx+1,top+1,right-2,h-top-bottom-2,TRUE);
 int usable=left-px(20);int widths[]={0,usable*40/100,usable*14/100,usable*16/100,0};
 widths[0]=px(75);widths[4]=usable-widths[0]-widths[1]-widths[2]-widths[3];
 for(int c=0;c<5;c++)ListView_SetColumnWidth(hList,c,widths[c]);
 SetWindowRgn(hList,CreateRoundRectRgn(0,0,left-1,h-top-bottom-1,px(12),px(12)),TRUE);
 SetWindowRgn(hDetail,CreateRoundRectRgn(0,0,right-1,h-top-bottom-1,px(12),px(12)),TRUE);
 InvalidateRect(window,NULL,FALSE);
}
static void copyDetail(HWND window){
 if(selected<0)return;
 size_t bytes=(wcslen(skins[selected].detail)+1)*sizeof(wchar_t);
 HGLOBAL mem=GlobalAlloc(GMEM_MOVEABLE,bytes);if(!mem)return;
 void *p=GlobalLock(mem);if(!p){GlobalFree(mem);return;}memcpy(p,skins[selected].detail,bytes);GlobalUnlock(mem);
 if(OpenClipboard(window)){EmptyClipboard();if(!SetClipboardData(CF_UNICODETEXT,mem))GlobalFree(mem);CloseClipboard();SetWindowTextW(hCopy,L"已复制");SetTimer(window,2,1400,NULL);}else{GlobalFree(mem);MessageBoxW(window,L"剪贴板暂时被其他程序占用，请重试。",L"复制信息",MB_OK|MB_ICONINFORMATION);}
}
static LRESULT CALLBACK procedure(HWND window,UINT msg,WPARAM wp,LPARAM lp){
 switch(msg){
 case WM_ERASEBKGND:return 1;
 case WM_PAINT:paintWindow(window);return 0;
 case WM_DRAWITEM:drawButton((DRAWITEMSTRUCT*)lp);return TRUE;
 case WM_CTLCOLORSTATIC:{HDC dc=(HDC)wp;HWND ctl=(HWND)lp;SetBkMode(dc,TRANSPARENT);SetTextColor(dc,(ctl==hTitle)?RGB(249,250,253):(ctl==hSearchLabel?C_MUTED:C_TEXT));return (LRESULT)((ctl==hTitle)?navBrush:(ctl==hSearchLabel?whiteBrush:pageBrush));}
 case WM_CTLCOLOREDIT:SetTextColor((HDC)wp,C_TEXT);SetBkColor((HDC)wp,RGB(255,255,255));return (LRESULT)whiteBrush;
 case WM_CREATE:{
  HDC dc=GetDC(window);scale=GetDeviceCaps(dc,LOGPIXELSY);ReleaseDC(window,dc);
  font=CreateFontW(-px(15),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei UI");
  titleFont=CreateFontW(-px(26),0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei UI");
  boldFont=CreateFontW(-px(14),0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei UI");
  smallFont=CreateFontW(-px(12),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei UI");
  sectionFont=CreateFontW(-px(17),0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei UI");
  pageBrush=CreateSolidBrush(C_PAGE);navBrush=CreateSolidBrush(C_NAVY);whiteBrush=CreateSolidBrush(RGB(255,255,255));
  hTitle=control(window,L"STATIC",L"英雄联盟 · 皮肤档案",SS_LEFT,0,0);SendMessageW(hTitle,WM_SETFONT,(WPARAM)titleFont,TRUE);
  hSearchLabel=control(window,L"STATIC",L"搜索皮肤或英雄",SS_LEFT,0,0);SendMessageW(hSearchLabel,WM_SETFONT,(WPARAM)smallFont,TRUE);
  hQuery=control(window,L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,ID_QUERY,0);
  SendMessageW(hQuery,EM_SETMARGINS,EC_LEFTMARGIN|EC_RIGHTMARGIN,MAKELPARAM(px(2),px(8)));
  SendMessageW(hQuery,EM_SETLIMITTEXT,500,0);SendMessageW(hQuery,EM_SETCUEBANNER,TRUE,(LPARAM)L"试试搜索“铁哥们儿”或“李青 神龙”…");
  hClear=control(window,L"BUTTON",L"清空",WS_TABSTOP,ID_CLEAR,0);
  hUpdate=control(window,L"BUTTON",L"更新设置",WS_TABSTOP,ID_UPDATE,0);
  hResultsLabel=control(window,L"STATIC",L"查询结果",SS_LEFT,0,0);SendMessageW(hResultsLabel,WM_SETFONT,(WPARAM)sectionFont,TRUE);
  hList=control(window,WC_LISTVIEWW,L"",WS_TABSTOP|LVS_REPORT|LVS_SINGLESEL|LVS_SHOWSELALWAYS,ID_RESULTS,0);
  ListView_SetExtendedListViewStyle(hList,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER|LVS_EX_LABELTIP);
  ListView_SetBkColor(hList,RGB(255,255,255));ListView_SetTextBkColor(hList,RGB(255,255,255));
  rowSpacing=ImageList_Create(1,px(38),ILC_COLOR32,1,1);ListView_SetImageList(hList,rowSpacing,LVSIL_SMALL);
  const wchar_t *names[]={L"皮肤ID",L"皮肤中文名",L"英雄",L"品质",L"国服售价"};
  for(int c=0;c<5;c++){LVCOLUMNW col={0};col.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;col.pszText=(LPWSTR)names[c];col.cx=px(120);col.iSubItem=c;ListView_InsertColumn(hList,c,&col);}
  SetWindowSubclass(ListView_GetHeader(hList),headerSubclass,1,0);
  hDetailLabel=control(window,L"STATIC",L"皮肤详情",SS_LEFT|SS_ENDELLIPSIS,0,0);SendMessageW(hDetailLabel,WM_SETFONT,(WPARAM)sectionFont,TRUE);
  hCopy=control(window,L"BUTTON",L"复制表格",WS_TABSTOP,ID_COPY,0);
  hDetail=control(window,L"LoLSkinDetailsTable",L"",WS_TABSTOP|WS_VSCROLL,0,0);
  HWND buttons[]={hClear,hCopy,hUpdate};for(int i=0;i<3;i++)SetWindowSubclass(buttons[i],buttonSubclass,1,0);
  loadUpdateSettings();layout(window);search();SetFocus(hQuery);SetTimer(window,4,700,NULL);return 0;
 }
 case WM_SIZE:if(hList)layout(window);return 0;
 case WM_GETMINMAXINFO:{MINMAXINFO *mm=(MINMAXINFO*)lp;RECT work;SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);int mw=px(1140),mh=px(720);mm->ptMinTrackSize.x=mw>work.right-work.left?work.right-work.left:mw;mm->ptMinTrackSize.y=mh>work.bottom-work.top?work.bottom-work.top:mh;return 0;}
 case WM_COMMAND:
  if(LOWORD(wp)==ID_QUERY&&(HIWORD(wp)==EN_SETFOCUS||HIWORD(wp)==EN_KILLFOCUS)){InvalidateRect(window,NULL,FALSE);return 0;}
  if(LOWORD(wp)==ID_QUERY&&HIWORD(wp)==EN_CHANGE){SetTimer(window,1,160,NULL);return 0;}
  if(LOWORD(wp)==ID_CLEAR){SetWindowTextW(hQuery,L"");KillTimer(window,1);search();SetFocus(hQuery);return 0;}
  if(LOWORD(wp)==ID_COPY){copyDetail(window);return 0;}
  if(LOWORD(wp)==ID_UPDATE){showUpdateSettings(window);return 0;}
  break;
 case WM_NOTIFY:{NMHDR *hdr=(NMHDR*)lp;if(hdr->idFrom==ID_RESULTS&&hdr->code==NM_CUSTOMDRAW)return drawResult((NMLVCUSTOMDRAW*)lp);if(hdr->idFrom==ID_RESULTS&&hdr->code==LVN_ITEMCHANGED)showSelected();break;}
 case WM_TIMER:if(wp==4){KillTimer(window,4);startUpdateCheck(window,0);return 0;}if(wp==3){pollUpdateCheck(window);return 0;}if(wp==2){KillTimer(window,2);SetWindowTextW(hCopy,L"复制表格");return 0;}if(wp==1){KillTimer(window,1);search();}return 0;
 case WM_DESTROY:if(updateThread)CloseHandle(updateThread);DeleteObject(font);DeleteObject(titleFont);DeleteObject(boldFont);DeleteObject(smallFont);DeleteObject(sectionFont);DeleteObject(pageBrush);DeleteObject(navBrush);DeleteObject(whiteBrush);if(rowSpacing)ImageList_Destroy(rowSpacing);PostQuitMessage(0);return 0;
 }
 return DefWindowProcW(window,msg,wp,lp);
}
int WINAPI wWinMain(HINSTANCE app,HINSTANCE previous,LPWSTR cmd,int show){
 (void)previous;(void)cmd;instance=app;SetProcessDPIAware();
 INITCOMMONCONTROLSEX controls={sizeof(controls),ICC_LISTVIEW_CLASSES};InitCommonControlsEx(&controls);
 WNDCLASSEXW cls={0};cls.cbSize=sizeof(cls);cls.lpfnWndProc=procedure;cls.hInstance=app;cls.lpszClassName=L"LoLSkinQueryOfflineV1";cls.hCursor=LoadCursorW(NULL,IDC_ARROW);cls.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);cls.hIcon=LoadIconW(app,MAKEINTRESOURCEW(1));cls.hIconSm=cls.hIcon;RegisterClassExW(&cls);
 WNDCLASSEXW details={0};details.cbSize=sizeof(details);details.lpfnWndProc=detailProcedure;details.hInstance=app;details.lpszClassName=L"LoLSkinDetailsTable";details.hCursor=LoadCursorW(NULL,IDC_ARROW);RegisterClassExW(&details);
 HDC dc=GetDC(NULL);scale=GetDeviceCaps(dc,LOGPIXELSY);ReleaseDC(NULL,dc);
 RECT work;SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);int width=px(1440),height=px(920);if(width>work.right-work.left)width=work.right-work.left;if(height>work.bottom-work.top)height=work.bottom-work.top;
 HWND window=CreateWindowExW(0,cls.lpszClassName,L"英雄联盟 · 皮肤档案 v" APP_VERSION_W,WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,work.left+(work.right-work.left-width)/2,work.top+(work.bottom-work.top-height)/2,width,height,NULL,NULL,app,NULL);
 if(!window)return 1;ShowWindow(window,show);UpdateWindow(window);
 MSG msg;while(GetMessageW(&msg,NULL,0,0)>0){if(msg.message==WM_KEYDOWN&&msg.wParam=='F'&&(GetKeyState(VK_CONTROL)&0x8000)){SetFocus(hQuery);SendMessageW(hQuery,EM_SETSEL,0,-1);continue;}if(msg.message==WM_KEYDOWN&&msg.wParam==VK_ESCAPE){SendMessageW(window,WM_COMMAND,ID_CLEAR,0);continue;}if(!IsDialogMessageW(window,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}return (int)msg.wParam;
}
