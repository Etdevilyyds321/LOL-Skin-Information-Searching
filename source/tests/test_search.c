#include <stdio.h>
#include <locale.h>
#include "../core.h"
#include "../skins_data.h"
int main(void){
 setlocale(LC_ALL,"");
 for(int i=0;i<SKIN_COUNT;i++){
  if(!skinMatches(&skins[i],skins[i].id)||!skinMatches(&skins[i],skins[i].name)||!skinMatches(&skins[i],skins[i].hero))return 2;
  if(wcscmp(skins[i].cells[3],skins[i].quality)||DETAIL_COUNT!=9)return 3;
  for(int j=0;j<DETAIL_COUNT;j++)if(!skins[i].cells[j]||!skins[i].cells[j][0])return 4;
  if(wcscmp(skins[i].quality,L"T0")&&wcscmp(skins[i].quality,L"T1")&&wcscmp(skins[i].quality,L"T2")&&wcscmp(skins[i].quality,L"T3")&&wcscmp(skins[i].quality,L"T4")&&wcscmp(skins[i].quality,L"T5")&&wcscmp(skins[i].quality,L"T级待核实"))return 5;
 }
 const wchar_t *queries[]={L"安妮",L"黑暗之女",L"铁哥们儿",L"奥拉夫",L"K/DA",L"kda",L"1001",L"不存在的皮肤XYZ",L"安妮 冰霜",L"李青 神龙",L"Annie",L"   "};
 for(size_t q=0;q<sizeof(queries)/sizeof(queries[0]);q++){
  int count=0;for(int i=0;i<SKIN_COUNT;i++)count+=skinMatches(&skins[i],queries[q]);
  printf("%d\n",count);
 }
 return 0;
}
