"""Regenerate the embedded catalog from data/skins.json; Python 3 standard library."""
from pathlib import Path
import json
import re

ROOT = Path(__file__).resolve().parent
LABELS = ['皮肤ID', '皮肤中文名', '英雄', '品质', '国服售价（原价）', '发行日期', '获取方式', '是否限定', '备注']

def generate():
    rows = json.loads((ROOT / 'data/skins.json').read_text(encoding='utf-8'))
    assert len(rows) == len({r['values'][0] for r in rows}) == 1970
    lit = lambda s: 'L' + json.dumps(s, ensure_ascii=False)
    lines = ['/* 1970 CN skin IDs; evidence and unresolved items: data/skins.json. */',
             'static const int DETAIL_COUNT = 9;',
             'static const wchar_t *const detailLabels[] = {' + ','.join(map(lit, LABELS)) + '};']
    for i, row in enumerate(rows):
        values = row['values']
        assert len(values) == 9 and re.fullmatch(r'T[0-5]', values[3])
        assert all(isinstance(v, str) and v for v in values)
        lines.append('static const wchar_t *info_' + str(i) + '[] = {' + ','.join(map(lit, values)) + '};')
    lines.append('static Skin skins[] = {')
    for i, row in enumerate(rows):
        values = row['values']
        search = re.sub(r't[0-5]$', '', row['search']) + values[3].lower()
        detail = '\r\n'.join(k + '\t' + v for k, v in zip(LABELS, values))
        lines.append('{' + ','.join(map(lit, values + [search, detail, row['source']])) + ',info_' + str(i) + '},')
    lines.extend(['};', 'static const int SKIN_COUNT = 1970;'])
    (ROOT / 'skins_data.h').write_text('\n'.join(lines) + '\n', encoding='utf-8')

if __name__ == '__main__':
    generate()
    print('Generated skins_data.h: 1970 records')
