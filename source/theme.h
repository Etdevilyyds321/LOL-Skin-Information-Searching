#ifndef LOL_SKIN_THEME_H
#define LOL_SKIN_THEME_H
#define C_NAVY RGB(19,35,56)
#define C_GOLD RGB(190,151,83)
#define C_PAGE RGB(244,246,250)
#define C_TEXT RGB(32,48,70)
#define C_MUTED RGB(96,111,133)
#define C_LINE RGB(225,231,239)
static void rounded(HDC dc,RECT r,COLORREF bg,COLORREF stroke,int radius){
 HBRUSH brush=CreateSolidBrush(bg);HPEN pen=CreatePen(PS_SOLID,1,stroke);HGDIOBJ ob=SelectObject(dc,brush),op=SelectObject(dc,pen);
 RoundRect(dc,r.left,r.top,r.right,r.bottom,px(radius),px(radius));SelectObject(dc,op);SelectObject(dc,ob);DeleteObject(pen);DeleteObject(brush);
}
static COLORREF tierColor(const wchar_t *tier){
 if(!wcscmp(tier,L"T0"))return RGB(116,54,147);
 if(!wcscmp(tier,L"T1"))return RGB(132,94,29);
 if(!wcscmp(tier,L"T2"))return RGB(120,83,174);
 if(!wcscmp(tier,L"T3"))return RGB(25,106,118);
 if(!wcscmp(tier,L"T4"))return RGB(78,111,149);
 if(!wcscmp(tier,L"T5"))return RGB(96,109,127);
 return RGB(143,94,20);
}
static void tierPill(HDC dc,const wchar_t *tier,RECT cell){
 HGDIOBJ old=SelectObject(dc,boldFont);SIZE sz;GetTextExtentPoint32W(dc,tier,(int)wcslen(tier),&sz);
 COLORREF col=tierColor(tier);COLORREF bg=RGB((GetRValue(col)+255*9)/10,(GetGValue(col)+255*9)/10,(GetBValue(col)+255*9)/10);
 RECT pill={cell.left,cell.top+(cell.bottom-cell.top-px(26))/2,cell.left+sz.cx+px(20),cell.top+(cell.bottom-cell.top-px(26))/2+px(26)};
 if(pill.right>cell.right)pill.right=cell.right;
 rounded(dc,pill,bg,bg,10);SetTextColor(dc,col);SetBkMode(dc,TRANSPARENT);DrawTextW(dc,tier,-1,&pill,DT_SINGLELINE|DT_CENTER|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX);SelectObject(dc,old);
}
static LRESULT CALLBACK buttonSubclass(HWND h,UINT m,WPARAM w,LPARAM l,UINT_PTR id,DWORD_PTR data){
 (void)id;(void)data;
 if(m==WM_MOUSEMOVE){TRACKMOUSEEVENT t={sizeof(t),TME_LEAVE,h,0};TrackMouseEvent(&t);InvalidateRect(h,NULL,FALSE);}
 if(m==WM_MOUSELEAVE||m==WM_SETFOCUS||m==WM_KILLFOCUS||m==WM_ENABLE)InvalidateRect(h,NULL,FALSE);
 return DefSubclassProc(h,m,w,l);
}
static void drawButton(DRAWITEMSTRUCT *di){
 HDC dc=di->hDC;RECT r=di->rcItem;wchar_t text[100];GetWindowTextW(di->hwndItem,text,100);
 POINT p;GetCursorPos(&p);ScreenToClient(di->hwndItem,&p);BOOL hover=PtInRect(&r,p),disabled=(di->itemState&ODS_DISABLED)!=0,pressed=(di->itemState&ODS_SELECTED)!=0;
 COLORREF bg=RGB(255,255,255),fg=C_TEXT,border=C_LINE;
 if(di->CtlID==ID_COPY){bg=pressed?RGB(165,127,62):(hover?RGB(207,171,108):C_GOLD);border=bg;fg=C_NAVY;}
 else if(di->CtlID==ID_ABOUT||di->CtlID==ID_UPDATE){bg=hover?RGB(43,61,84):C_NAVY;border=RGB(79,93,114);fg=RGB(231,235,241);}
 else if(hover){bg=RGB(235,240,247);border=RGB(192,204,220);}
 if(disabled){bg=RGB(237,240,245);fg=RGB(161,171,185);border=bg;}
 HBRUSH parent=CreateSolidBrush(di->CtlID==ID_ABOUT||di->CtlID==ID_UPDATE?C_NAVY:(di->CtlID==ID_CLEAR?RGB(255,255,255):C_PAGE));FillRect(dc,&r,parent);DeleteObject(parent);
 r.right--;r.bottom--;rounded(dc,r,bg,border,12);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,fg);HGDIOBJ old=SelectObject(dc,boldFont);
 DrawTextW(dc,text,-1,&r,DT_SINGLELINE|DT_CENTER|DT_VCENTER|DT_NOPREFIX);
 if(di->itemState&ODS_FOCUS){RECT f=r;InflateRect(&f,-px(4),-px(4));DrawFocusRect(dc,&f);}SelectObject(dc,old);
}
static LRESULT CALLBACK headerSubclass(HWND h,UINT m,WPARAM w,LPARAM l,UINT_PTR id,DWORD_PTR data){
 (void)id;(void)data;
 if(m==HDM_LAYOUT){LRESULT result=DefSubclassProc(h,m,w,l);HDLAYOUT *p=(HDLAYOUT*)l;int old=p->pwpos->cy;p->pwpos->cy=px(42);p->prc->top+=px(42)-old;return result;}
 if(m==WM_ERASEBKGND)return 1;
 if(m==WM_PAINT){
  PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);RECT rc;GetClientRect(h,&rc);HBRUSH brush=CreateSolidBrush(RGB(245,247,251));FillRect(dc,&rc,brush);DeleteObject(brush);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,C_MUTED);HGDIOBJ old=SelectObject(dc,smallFont);
  for(int i=0;i<Header_GetItemCount(h);i++){RECT r;Header_GetItemRect(h,i,&r);wchar_t text[100];HDITEMW item={0};item.mask=HDI_TEXT;item.pszText=text;item.cchTextMax=100;Header_GetItem(h,i,&item);r.left+=px(14);r.right-=px(6);DrawTextW(dc,text,-1,&r,DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS);}
  HPEN pen=CreatePen(PS_SOLID,1,C_LINE);HGDIOBJ op=SelectObject(dc,pen);MoveToEx(dc,0,rc.bottom-1,NULL);LineTo(dc,rc.right,rc.bottom-1);SelectObject(dc,op);DeleteObject(pen);SelectObject(dc,old);EndPaint(h,&ps);return 0;
 }
 return DefSubclassProc(h,m,w,l);
}
static LRESULT drawResult(NMLVCUSTOMDRAW *d){
 if(d->nmcd.dwDrawStage==CDDS_PREPAINT)return CDRF_NOTIFYITEMDRAW;
 if(d->nmcd.dwDrawStage==CDDS_ITEMPREPAINT)return CDRF_NOTIFYSUBITEMDRAW;
 if(d->nmcd.dwDrawStage==(CDDS_ITEMPREPAINT|CDDS_SUBITEM)){
  int row=(int)d->nmcd.dwItemSpec,col=d->iSubItem;RECT r;ListView_GetSubItemRect(hList,row,col,LVIR_BOUNDS,&r);if(col==0)r.right=r.left+ListView_GetColumnWidth(hList,0);
  BOOL chosen=(ListView_GetItemState(hList,row,LVIS_SELECTED)&LVIS_SELECTED)!=0;
  COLORREF bg=chosen?RGB(234,240,249):(row%2?RGB(251,252,254):RGB(255,255,255));HBRUSH brush=CreateSolidBrush(bg);FillRect(d->nmcd.hdc,&r,brush);DeleteObject(brush);
  if(chosen&&col==0){RECT mark={r.left,r.top,r.left+px(3),r.bottom};brush=CreateSolidBrush(C_GOLD);FillRect(d->nmcd.hdc,&mark,brush);DeleteObject(brush);}
  wchar_t text[1024];ListView_GetItemText(hList,row,col,text,1024);r.left+=px(14);r.right-=px(8);
  if(col==3)tierPill(d->nmcd.hdc,text,r);
  else {SetBkMode(d->nmcd.hdc,TRANSPARENT);SetTextColor(d->nmcd.hdc,col==0?C_MUTED:(wcsstr(text,L"待核实")?RGB(157,112,40):C_TEXT));HGDIOBJ old=SelectObject(d->nmcd.hdc,col==1?boldFont:font);DrawTextW(d->nmcd.hdc,text,-1,&r,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX);SelectObject(d->nmcd.hdc,old);}
  return CDRF_SKIPDEFAULT;
 }
 return CDRF_DODEFAULT;
}
static void paintWindow(HWND window){
 PAINTSTRUCT ps;HDC target=BeginPaint(window,&ps);RECT rc;GetClientRect(window,&rc);if(rc.right<=0||rc.bottom<=0){EndPaint(window,&ps);return;}
 HDC dc=CreateCompatibleDC(target);HBITMAP bitmap=CreateCompatibleBitmap(target,rc.right,rc.bottom);HGDIOBJ ob=SelectObject(dc,bitmap);HBRUSH brush=CreateSolidBrush(C_PAGE);FillRect(dc,&rc,brush);DeleteObject(brush);
 RECT nav={0,0,rc.right,px(104)};brush=CreateSolidBrush(C_NAVY);FillRect(dc,&nav,brush);DeleteObject(brush);
 RECT logo={px(28),px(27),px(76),px(75)};rounded(dc,logo,C_GOLD,C_GOLD,14);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,C_NAVY);HGDIOBJ old=SelectObject(dc,titleFont);DrawTextW(dc,L"L",-1,&logo,DT_CENTER|DT_VCENTER|DT_SINGLELINE);SelectObject(dc,old);
 int m=px(28),gap=px(20),left=(rc.right-2*m-gap)*55/100,rightx=m+left+gap;
 RECT searchBox={m,px(122),rc.right-m,px(208)};rounded(dc,searchBox,RGB(255,255,255),GetFocus()==hQuery?C_GOLD:C_LINE,16);
 RECT a={m,px(272),m+left,rc.bottom-px(28)},b={rightx,px(272),rc.right-m,rc.bottom-px(28)};rounded(dc,a,RGB(255,255,255),C_LINE,14);rounded(dc,b,RGB(255,255,255),C_LINE,14);
 BitBlt(target,0,0,rc.right,rc.bottom,dc,0,0,SRCCOPY);SelectObject(dc,ob);DeleteObject(bitmap);DeleteDC(dc);EndPaint(window,&ps);
}
#endif
