#include "stdafx.h"
#include "DesignerPaneView.h"
#include "DesignerPaneInternal.h"
#include <algorithm>
#include <set>
#include <unordered_map>

using namespace uidesign;
using namespace uidesign::detail;

// 粘贴/复制的临时诊断：PYARX_DESIGN_DEBUG=1 时写 log\paste_diag.log
static void pasteDiag(const char* fmt, ...)
{
    static const bool on = []() {
        char buf[8] = { 0 };
        return ::GetEnvironmentVariableA("PYARX_DESIGN_DEBUG", buf, sizeof(buf)) > 0;
    }();
    if (!on) return;
    std::FILE* f = nullptr;
    if (fopen_s(&f, "G:\\PyArx\\log\\paste_diag.log", "a") != 0 || f == nullptr) return;
    va_list ap; va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fclose(f);
}

void DesignerPaneView::onNew()
{
    pushUndo();
    m_doc.reset();
    // 新建的文档要有一个根 Window，否则根节点为空时画布、属性面板、保存都无从下手
    std::wstring err;
    if (!m_doc.loadString(
            L"<Window Name=\"MainWindow\" Width=\"640\" Height=\"420\" Title=\"未命名\"/>", err))
    {
        AfxMessageBox(CString(err.c_str()), MB_ICONERROR);
    }
    m_selected.clear();
    m_docPath.Empty();
    XamlElement* root = m_doc.root();
    if (root != nullptr)
        m_doc.layout(0, 0, root->w, root->h);
    m_canvas->setDocument(&m_doc);
    rebuildTree();
    fillProperties(nullptr);
    SetWindowText(_T("PyMfc UI Designer - 未命名"));
    updateStatus();
}

// a 是否为 b 的严格前缀（a 是 b 的某个祖先）
static bool pathIsPrefix(const Path& a, const Path& b)
{
    if (a.empty() || a.size() >= b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (a[i] != b[i]) return false;
    return true;
}

void DesignerPaneView::onDelete()
{
    XamlElement* root = m_doc.root();
    if (root == nullptr) return;

    // 待删路径：多选集合（>=2 个）走批量，否则就是单个主选中
    std::vector<Path> victims;
    if (m_selSet.size() >= 2) victims = m_selSet;
    else if (!m_selected.empty()) victims.push_back(m_selected);
    if (victims.empty())
    {
        notImpl(L"无法删除根节点");
        return;
    }

    // 祖先也选中时，它下面的子孙路径跳过：删祖先已经连带删掉它们，
    // 再按旧下标去删会删错节点或越界。
    std::vector<Path> real;
    for (const auto& p : victims)
    {
        if (p.empty()) continue;                     // 根窗体不能删
        bool covered = false;
        for (const auto& q : victims)
            if (pathIsPrefix(q, p)) { covered = true; break; }
        if (!covered) real.push_back(p);
    }
    if (real.empty())
    {
        notImpl(L"无法删除根节点");
        return;
    }

    // 兄弟按下标从大到小删，前面的下标才不会因为删除而错位
    struct Ent { XamlElement* par; int idx; };
    std::vector<Ent> ents;
    for (const auto& p : real)
    {
        int idx = 0;
        XamlElement* par = ParentOf(root, p, idx);
        if (par != nullptr && idx >= 0 && idx < (int)par->children.size())
            ents.push_back({ par, idx });
    }
    if (ents.empty()) return;

    pushUndo();
    std::sort(ents.begin(), ents.end(), [](const Ent& a, const Ent& b) {
        if (a.par != b.par) return a.par < b.par;
        return a.idx > b.idx;
    });
    const size_t removed = ents.size();
    for (const auto& e : ents)
        RemoveChild(e.par, e.idx);

    m_selected.clear();
    m_selSet.clear();
    if (m_canvas != nullptr)
    {
        m_canvas->setSelected(Path());
        m_canvas->clearSelection();
    }
    m_doc.layout(0, 0, root->w, root->h);
    m_canvas->setDocument(&m_doc);
    rebuildTree();
    fillProperties(nullptr);
    updateStatus();
    Invalidate(FALSE);
    if (removed > 1)
    {
        CString msg;
        msg.Format(_T("已删除 %d 个节点"), (int)removed);
        notImpl(msg);
    }
}

// ---- 节点级剪贴板 / 对齐 / 缩放 / 标尺 ----

// 子树序列化：属性按文档顺序原样输出，写进系统剪贴板后可直接粘到别的 XAML 文件
static void elementToXmlImpl(const XamlElement& el, int depth, std::wstring& out)
{
    const std::wstring pad((size_t)(depth * 2), L' ');
    out += pad + L"<" + el.tag;
    for (const auto& kv : el.attrs)
        out += L" " + kv.first + L"=\"" + kv.second + L"\"";
    if (!el.name.empty() && !el.has(L"Name"))
        out += L" Name=\"" + el.name + L"\"";
    if (el.text.empty() && el.children.empty())
    {
        out += L" />\r\n";
        return;
    }
    if (!el.text.empty())
    {
        out += L">" + el.text + L"</" + el.tag + L">\r\n";
        return;
    }
    out += L">\r\n";
    for (const auto& c : el.children)
        elementToXmlImpl(c, depth + 1, out);
    out += pad + L"</" + el.tag + L">\r\n";
}

static bool childNameTaken(const XamlElement* parent, const std::wstring& nm)
{
    if (parent == nullptr) return false;
    for (const auto& c : parent->children)
        if (c.name == nm) return true;
    return false;
}

// 文档结构变化后的统一刷新：重排 → 重设画布 → 属性表 → 状态栏 → 重画
// 对象树的折叠状态按节点指针保存，文档整体重解析 / undo 之后旧指针全部失效，
// 留着会让新树里复用同一地址的节点莫名其妙地"自带收起"，所以每次文档变化后修剪一次。
void DesignerPaneView::pruneCollapsedNodes()
{
    if (m_collapsedNodes.empty()) return;
    std::set<XamlElement*> live;
    XamlElement* root = m_doc.root();
    if (root != nullptr)
    {
        live.insert(root);
        uidesign::Walk(root, [&](const auto&, XamlElement& el) { live.insert(&el); });
    }
    for (auto it = m_collapsedNodes.begin(); it != m_collapsedNodes.end(); )
    {
        if (live.find(*it) == live.end())
            it = m_collapsedNodes.erase(it);
        else
            ++it;
    }
}

void DesignerPaneView::refreshAfterDocChange()
{
    pruneCollapsedNodes();
    XamlElement* root = m_doc.root();
    if (root != nullptr)
        m_doc.layout(0, 0, root->w, root->h);
    if (m_canvas != nullptr) m_canvas->setDocument(&m_doc);
    fillProperties(Resolve(m_doc.root(), m_selected));
    updateStatus();
    Invalidate(FALSE);
}

bool DesignerPaneView::copySelectedToClip()
{
    if (m_selected.empty()) return false;
    XamlElement* node = Resolve(m_doc.root(), m_selected);
    if (node == nullptr) return false;
    m_clipXml.clear();
    elementToXmlImpl(*node, 0, m_clipXml);
    // 同时放进系统剪贴板：粘到外部编辑器 / 另一个设计器实例都行
    if (::OpenClipboard(m_hWnd))
    {
        ::EmptyClipboard();
        const size_t units = m_clipXml.size() + 1;
        HGLOBAL h = ::GlobalAlloc(GMEM_MOVEABLE, units * sizeof(wchar_t));
        if (h != nullptr)
        {
            void* p = ::GlobalLock(h);
            if (p != nullptr)
            {
                wchar_t* dst = static_cast<wchar_t*>(p);
                std::copy(m_clipXml.begin(), m_clipXml.end(), dst);
                dst[m_clipXml.size()] = 0;
                ::GlobalUnlock(h);
                ::SetClipboardData(CF_UNICODETEXT, h);
            }
        }
        ::CloseClipboard();
    }
    return true;
}

void DesignerPaneView::onCopy()
{
    if (!copySelectedToClip()) { notImpl(L"未选中节点，无法复制"); return; }
    notImpl(L"已复制节点");
}

void DesignerPaneView::onCut()
{
    if (!copySelectedToClip()) { notImpl(L"未选中节点，无法剪切"); return; }
    if (m_selected.empty()) { notImpl(L"根节点不能剪切"); return; }
    onDelete();
    notImpl(L"已剪切节点");
}

void DesignerPaneView::onPaste()
{
    XamlElement* root = m_doc.root();
    pasteDiag("[paste] enter clipXml=%d hasClipNode=%d selLen=%d root=%p\n",
              (int)m_clipXml.size(), m_hasClipNode ? 1 : 0, (int)m_selected.size(), (void*)root);
    if (root == nullptr) return;

    // 片段来源双通道：代码区/系统剪贴板走 m_clipXml，对象检查器"复制节点"走 m_clipNode。
    // 两个都要认，否则从对象检查器复制完按 Ctrl+V 会提示"内部剪贴板为空"。
    XamlElement frag;
    bool hasFrag = false;
    if (!m_clipXml.empty())
    {
        XamlRuntime tmp;
        std::wstring err;
        if (tmp.loadString(m_clipXml.c_str(), err) && tmp.root() != nullptr)
        {
            frag = *tmp.root();      // XamlElement 是值语义树，整体深拷贝
            hasFrag = true;
        }
    }
    if (!hasFrag && m_hasClipNode)
    {
        frag = m_clipNode;
        hasFrag = true;
    }
    if (!hasFrag) { notImpl(L"剪贴板为空，先复制一个节点"); return; }

    // 目标容器：选中容器就粘进去；选中叶子粘到它的父容器；
    // 什么都没选（空画布 / 刚启动 / 刚按 Esc）就粘到根窗体
    Path targetPath = m_selected;
    XamlElement* target = nullptr;
    XamlElement* sel = Resolve(root, m_selected);
    if (sel != nullptr && sel != root && uidesign::IsContainer(sel->tag))
    {
        target = sel;
    }
    else if (sel != nullptr && sel != root)
    {
        int idx = 0;
        target = uidesign::ParentOf(root, m_selected, idx);
        if (!targetPath.empty()) targetPath.pop_back();
    }
    if (target == nullptr) { target = root; targetPath.clear(); }

    // 名字：尾部数字 +1（Button3 → Button4），文档内唯一；用户之后自己双击改名
    CString base = frag.name.empty() ? CString(frag.tag.c_str()) : CString(frag.name.c_str());
    CString nm = makeUniqueName(base);

    // 粘出来的节点要肉眼可见：绝对定位的控件和源节点错开若干格，
    // 否则副本正好压在原件上，看起来像"根本没粘上去"（这是最容易误判成"Ctrl+V 没反应"的地方）。
    if (frag.hasX && frag.hasY)
    {
        const int ox = _wtoi(frag.get(L"Left").c_str());
        const int oy = _wtoi(frag.get(L"Top").c_str());
        int dx = 18, dy = 18;
        for (int step = 0; step < 40; ++step)
        {
            bool clash = false;
            for (const XamlElement& c : target->children)
            {
                if (!c.hasX || !c.hasY) continue;
                if (_wtoi(c.get(L"Left").c_str()) == ox + dx && _wtoi(c.get(L"Top").c_str()) == oy + dy)
                { clash = true; break; }
            }
            if (!clash) break;
            dx += 18; dy += 18;
        }
        frag.set(L"Left", std::to_wstring(ox + dx));
        frag.set(L"Top", std::to_wstring(oy + dy));
    }

    pushUndo();
    frag.name = nm.GetString();
    frag.set(L"Name", frag.name);
    target->children.push_back(frag);

    m_selected = targetPath;
    m_selected.push_back((int)target->children.size() - 1);
    pasteDiag("[paste] target tag=%s kids=%d newPathLen=%d name=%s\n",
              CStringA(CString(target->tag.c_str())).GetString(), (int)target->children.size(),
              (int)m_selected.size(), CStringA(nm).GetString());
    if (m_canvas != nullptr) m_canvas->setSelected(m_selected);
    refreshAfterDocChange();
    notImpl(L"已粘贴 <" + CString(frag.tag.c_str()) + L"> " + CString(nm.GetString()));
}

// Ctrl+拖动复制：整棵拷贝到同一父容器、末尾数字 +1，选中副本并让它接着跟手。
// 撤销快照在这里压（必须早于副本入树），画布那边用 beginMoveDragNoUndo 跳过二次压栈。
void DesignerPaneView::beginCopyDrag(const Path& p, CPoint pressPt)
{
    XamlElement* root = m_doc.root();
    if (root == nullptr || p.empty()) return;
    XamlElement* node = Resolve(root, p);
    if (node == nullptr || node == root) return;

    Path parentPath = p;
    parentPath.pop_back();
    XamlElement* par = Resolve(root, parentPath);
    if (par == nullptr) return;

    pushUndo();
    XamlElement dup = *node;                  // 值语义整棵拷贝
    CString base = dup.name.empty() ? CString(dup.tag.c_str()) : CString(dup.name.c_str());
    CString nm = makeUniqueName(base);
    dup.name = nm.GetString();
    dup.set(L"Name", dup.name);
    par->children.push_back(dup);

    m_selected = parentPath;
    m_selected.push_back((int)par->children.size() - 1);
    refreshAfterDocChange();                  // 重布局 + 重建对象树 + 刷新 XAML 代码区
    if (m_canvas != nullptr) m_canvas->setSelected(m_selected);
    if (m_canvas != nullptr) m_canvas->beginMoveDragNoUndo(pressPt);
    notImpl(L"已复制 <" + CString(dup.tag.c_str()) + L"> " + CString(nm.GetString()));
}

// 复制 / 粘贴时给副本起一个文档内不重名的名字：Tag1, Tag2, ...
CString DesignerPaneView::makeUniqueName(const CString& base) const
{
    std::set<std::wstring> used;
    XamlElement* root = const_cast<DesignerPaneView*>(this)->m_doc.root();
    if (root != nullptr)
        uidesign::Walk(root, [&](const Path&, XamlElement& el) { used.insert(el.name); });

    // 规则：尾部是数字就自动 +1（Button3 → Button4），没有数字才补 1、2、3…
    // 复制出来的名字只保证唯一，叫什么最后由用户自己双击改名。
    int i = base.GetLength();
    while (i > 0 && iswdigit((unsigned short)base[i - 1])) --i;
    CString prefix = base.Left(i);
    if (prefix.IsEmpty()) prefix = L"Item";          // 名字是纯数字时给个前缀
    CString digits = base.Mid(i);
    long start = 1;
    if (!digits.IsEmpty())
    {
        const long v = _wtol((LPCTSTR)digits);
        start = (v > 0 && v < 99999) ? v + 1 : 1;    // 溢出/异常时退回从 1 开始
    }

    for (long k = start; k < start + 9999; ++k)
    {
        CString nm;
        nm.Format(L"%s%ld", (LPCTSTR)prefix, k);
        if (used.find(nm.GetString()) == used.end()) return nm;
    }
    return base;
}

void DesignerPaneView::onAlign(const CString& mode)
{
    XamlElement* root = m_doc.root();
    if (root == nullptr || m_selected.empty()) { notImpl(L"未选中节点"); return; }

    // 多选（>=2）按选区外接框对齐（Qt Designer 语义）；单选按父容器内容区对齐
    std::vector<XamlElement*> targets;
    for (auto& p : m_selSet)
    {
        XamlElement* n = Resolve(root, p);
        if (n != nullptr) targets.push_back(n);
    }
    if (targets.empty())
    {
        XamlElement* node = Resolve(root, m_selected);
        if (node == nullptr) return;
        targets.push_back(node);
    }

    int px = 0, py = 0, pw = 0, ph = 0;
    if (targets.size() >= 2)
    {
        int l = 0x7fffffff, t = 0x7fffffff, r = -0x7fffffff, b = -0x7fffffff;
        for (auto* n : targets)
        {
            l = (std::min)(l, (int)n->previewX);
            t = (std::min)(t, (int)n->previewY);
            r = (std::max)(r, (int)n->previewX + (int)n->previewW);
            b = (std::max)(b, (int)n->previewY + (int)n->previewH);
        }
        px = l; py = t; pw = r - l; ph = b - t;
    }
    else
    {
        int idx = 0;
        XamlElement* parent = uidesign::ParentOf(root, m_selected, idx);
        if (parent == nullptr) { notImpl(L"根节点没有父容器，无法对齐"); return; }
        px = parent->previewX + parent->padL;
        py = parent->previewY + parent->padT;
        pw = parent->previewW - parent->padL - parent->padR;
        ph = parent->previewH - parent->padT - parent->padB;
    }

    pushUndo();
    bool moved = false;
    for (auto* n : targets)
    {
        const int nx0 = (int)n->previewX, ny0 = (int)n->previewY;
        const int nw = (int)n->previewW, nh = (int)n->previewH;
        int nx = nx0, ny = ny0;
        if (mode == L"left") nx = px;
        else if (mode == L"right") nx = px + pw - nw;
        else if (mode == L"hcenter") nx = px + (pw - nw) / 2;
        else if (mode == L"top") ny = py;
        else if (mode == L"bottom") ny = py + ph - nh;
        else if (mode == L"vcenter") ny = py + (ph - nh) / 2;
        if (nx == nx0 && ny == ny0) continue;

        // 每个节点自己的父偏移 = preview - x，逐节点换算，不依赖主选中的父容器
        applyGeomAttrs(n, nx - (int)(n->previewX - n->x), ny - (int)(n->previewY - n->y));
        moved = true;
    }
    if (!moved) return;
    refreshAfterDocChange();
    notImpl(L"已对齐");
}

// 只改 Left/Top（对齐 / 分布共用），宽高保持不动
void DesignerPaneView::applyGeomAttrs(XamlElement* n, int x, int y)
{
    if (n == nullptr) return;
    n->hasX = n->hasY = true;
    n->x = x; n->y = y;
    n->set(L"Left", std::to_wstring(x));
    n->set(L"Top", std::to_wstring(y));
}

// 均匀分布：首尾不动，中间控件等间隔（Qt Designer 的 distribute 语义）
void DesignerPaneView::onDistribute(bool horizontal)
{
    XamlElement* root = m_doc.root();
    if (root == nullptr || m_selSet.size() < 3)
    {
        notImpl(L"均匀分布需要先框选 / Ctrl 点选 3 个以上控件");
        return;
    }

    std::vector<XamlElement*> v;
    for (auto& p : m_selSet)
    {
        XamlElement* n = Resolve(root, p);
        if (n != nullptr) v.push_back(n);
    }
    if (v.size() < 3) { notImpl(L"均匀分布需要 3 个以上控件"); return; }

    std::sort(v.begin(), v.end(), [horizontal](XamlElement* a, XamlElement* b) {
        return horizontal ? (a->previewX < b->previewX) : (a->previewY < b->previewY);
    });

    int span = 0, used = 0;
    for (auto* n : v)
    {
        if (horizontal)
        {
            span = (std::max)(span, (int)(n->previewX + n->previewW) - (int)v.front()->previewX);
            used += (int)n->previewW;
        }
        else
        {
            span = (std::max)(span, (int)(n->previewY + n->previewH) - (int)v.front()->previewY);
            used += (int)n->previewH;
        }
    }
    const int gap = (span - used) / (int)(v.size() - 1);

    pushUndo();
    int cursor = horizontal
        ? (int)v.front()->previewX + (int)v.front()->previewW + gap
        : (int)v.front()->previewY + (int)v.front()->previewH + gap;
    for (size_t i = 1; i + 1 < v.size(); ++i)
    {
        XamlElement* n = v[i];
        const int offX = (int)(n->previewX - n->x);
        const int offY = (int)(n->previewY - n->y);
        if (horizontal)
        {
            applyGeomAttrs(n, cursor - offX, (int)n->previewY - offY);
            cursor += (int)n->previewW + gap;
        }
        else
        {
            applyGeomAttrs(n, (int)n->previewX - offX, cursor - offY);
            cursor += (int)n->previewH + gap;
        }
    }
    refreshAfterDocChange();
    notImpl(horizontal ? L"已横向均匀分布" : L"已纵向均匀分布");
}

void DesignerPaneView::toggleRuler()
{
    m_showRuler = !m_showRuler;
    if (m_canvas != nullptr)
    {
        m_canvas->setShowRuler(m_showRuler);
        m_canvas->Invalidate(FALSE);
    }
    notImpl(m_showRuler ? L"标尺已开启" : L"标尺已关闭");
}

void DesignerPaneView::zoomBy(float factor)
{
    if (m_canvas == nullptr) return;
    float z = m_canvas->zoom() * factor;
    if (z < 0.25f) z = 0.25f;
    if (z > 4.0f) z = 4.0f;
    m_canvas->setZoom(z);
    updateStatus();
    Invalidate(FALSE);
}

void DesignerPaneView::zoomFit()
{
    if (m_canvas == nullptr) return;
    XamlElement* root = m_doc.root();
    if (root == nullptr || root->w <= 0 || root->h <= 0) { notImpl(L"文档尺寸为 0，无法适应窗口"); return; }
    CRect rc;
    m_canvas->GetClientRect(&rc);
    if (rc.Width() <= 0 || rc.Height() <= 0) return;
    float zx = (float)rc.Width() / (float)root->w;
    float zy = (float)rc.Height() / (float)root->h;
    float z = (zx < zy) ? zx : zy;
    if (z > 1.0f) z = 1.0f;                 // 只缩小看全，不放大
    if (z < 0.25f) z = 0.25f;
    m_canvas->setZoom(z);
    updateStatus();
    Invalidate(FALSE);
    notImpl(L"已适应窗口");
}

void DesignerPaneView::toggleGrid()
{
    m_showGrid = !m_showGrid;
    if (m_canvas != nullptr)
    {
        m_canvas->setShowGrid(m_showGrid);
        m_canvas->Invalidate(FALSE);
    }
    notImpl(m_showGrid ? L"网格已开启" : L"网格已关闭");
}

void DesignerPaneView::notImpl(const CString& what)
{
    // 没有 CStatic 状态栏控件，提示写进自绘的状态栏文本并重绘
    m_surfStatus[0] = what;
    Invalidate(FALSE);
}

void DesignerPaneView::pushUndo()
{
    if (m_doc.root() == nullptr) return;
    m_undoStack.push_back(*m_doc.root());   // XamlElement 深拷贝（值语义树）
    m_redoStack.clear();
    if (m_undoStack.size() > 64) m_undoStack.erase(m_undoStack.begin());
}

void DesignerPaneView::restoreSnapshot(XamlElement& snap)
{
    XamlElement* root = m_doc.root();
    if (root != nullptr) *root = snap;       // 深拷贝恢复文档树
    m_selected.clear();
    m_doc.layout(0, 0, root ? root->w : 0, root ? root->h : 0);
    if (m_canvas != nullptr) m_canvas->setDocument(&m_doc);
    rebuildTree();
    fillProperties(nullptr);
    updateStatus();
}

void DesignerPaneView::undo()
{
    if (m_undoStack.empty() || m_doc.root() == nullptr) return;
    m_redoStack.push_back(*m_doc.root());
    XamlElement snap = m_undoStack.back();
    m_undoStack.pop_back();
    restoreSnapshot(snap);
    m_surfStatus[0] = L"已撤销";
}

void DesignerPaneView::redo()
{
    if (m_redoStack.empty() || m_doc.root() == nullptr) return;
    m_undoStack.push_back(*m_doc.root());
    XamlElement snap = m_redoStack.back();
    m_redoStack.pop_back();
    restoreSnapshot(snap);
    m_surfStatus[0] = L"已重做";
}

// ===========================================================================
// XAML 源码区编辑
// 代码区现在是控件库里的 CodeEditor（自带窗口 + D2D 自绘 + 智能提示），
// 不再"点进去贴一个 CEdit、走的时候再收掉"：编辑器常驻，焦点进/出就是编辑的开始与落定。
// ===========================================================================

void DesignerPaneView::beginCodeEdit()
{
    if (!m_codeEditor) return;
    if (m_surfCodeRc.right <= m_surfCodeRc.left) return;   // 代码区还没画过

    syncCodeEditor();
    m_codeEditor->focusEditor();
    m_codeEditing = true;
    m_surfStatus[0] = L"源码编辑中（离开代码区即应用）";
    Invalidate(FALSE);
}

void DesignerPaneView::commitCodeEdit()
{
    if (!m_codeEditing || !m_codeEditor)
        return;
    m_codeEditing = false;

    CString xml(m_codeEditor->text().c_str());
    xml.Replace(L"\r\n", L"\n");
    if (xml.IsEmpty())
    {
        Invalidate(FALSE);
        return;
    }
    if (xml == CString(m_doc.toXml().c_str()))
    {
        Invalidate(FALSE);          // 没改过就不动文档，也不压一次撤销
        return;
    }
    applyCodeText(std::wstring(xml));
    Invalidate(FALSE);
}

void DesignerPaneView::cancelCodeEdit()
{
    m_codeEditing = false;
    syncCodeEditor();               // 回到文档里的内容
    m_surfStatus[0] = L"已取消源码编辑";
    Invalidate(FALSE);
}
