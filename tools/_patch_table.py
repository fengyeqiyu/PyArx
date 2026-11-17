import io,re
p=r"G:\PyArx\UIDesign\DesignCanvas.h"
s=io.open(p,encoding="utf-8").read()
start=s.find('            else if (tagIs(node, L"ListCtrl") || tagIs(node, L"GridLayout") ||')
end=s.find('            else if (tagIs(node, L"Rectangle")', start)
assert start>0 and end>start, (start,end)
new='''            else if (tagIs(node, L"ListCtrl") || tagIs(node, L"GridLayout") ||
                     tagIs(node, L"DataGrid") || tagIs(node, L"TableView"))
            {
                // 表格：画法与预览器/运行时同一份（MfcXmlCore/XmlTableDraw.h），
                // 这里只把 XML 颜色属性和字号转成 Style，不再各端各写一套行/表头逻辑。
                const COLORREF tbg = bgC == (COLORREF)-1 ? cvdoc::kField : bgC;
                const int lum = (GetRValue(tbg) * 30 + GetGValue(tbg) * 59 + GetBValue(tbg) * 11) / 100;
                const bool dark = lum < 128;
                xmltable::Style st;
                st.bg = tbg;
                st.ink = fgC == (COLORREF)-1 ? (dark ? RGB(226, 231, 238) : cvdoc::kInk) : fgC;
                st.headBg = dark ? RGB(52, 58, 70) : cvdoc::kHead;
                st.grid = dark ? RGB(72, 79, 92) : cvdoc::kGrid;
                st.border = bdC == (COLORREF)-1 ? cvdoc::kBorder : bdC;
                if (pt > 0) st.pt = pt;
                if (!fam.empty()) st.family = fam;
                drawRoundedCv(cv, rc, tbg, st.border, 3);
                if (!xmltable::drawTable(cv, rc, node->get(L"Columns"), node->get(L"Rows"),
                                         node->get(L"Sort"), st))
                {
                    // 没声明列就仍是"空表格"，但标出控件类型，避免看起来像坏掉的空框
                    drawRoundedCv(cv, rc, cvdoc::kSurface, cvdoc::kBorder, 4);
                    drawTextCv(cv, rc, std::wstring(L"[") + node->tag + std::wstring(L"]"),
                               cvdoc::kMuted, DT_CENTER | DT_VCENTER | DT_SINGLELINE, 0, std::wstring());
                }
            }
'''
s=s[:start]+new+s[end:]
s=s.replace('#include "XmlPowerPlanDraw.h"','#include "XmlPowerPlanDraw.h"\n#include "XmlTableDraw.h"',1)
io.open(p,"w",encoding="utf-8",newline="\n").write(s)
print("ok", len(s))