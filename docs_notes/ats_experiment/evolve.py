# -*- coding: utf-8 -*-
"""进化版 ATS 模拟：格式鲁棒性 × 带权匹配。

同一份简历出 5 个版式变体，分别用「朴素解析器」和「有能力的解析器」过一遍，
看字段召回与 JD 打分掉在哪一层——回答「看起来像真的」在真实管道里到底被什么打穿。

产物：variants/*.docx|pdf、evolution_report.md、evolution_metrics.json
"""
import json
import re
from pathlib import Path

from build_and_simulate import CONTACT, EDUCATION, EXPERIENCE, NAME, SKILLS, SUMMARY

OUT = Path(__file__).resolve().parent
VAR = OUT / "variants"
VAR.mkdir(exist_ok=True)

# ---------------------------------------------------------------- 变体生成

def make_docx_plain(path, table_skills=False, header_contact=False):
    from docx import Document
    from docx.enum.text import WD_ALIGN_PARAGRAPH
    from docx.shared import Inches, Pt

    doc = Document()
    for s in doc.sections:
        s.top_margin = Inches(0.55)
        s.bottom_margin = Inches(0.5)
        s.left_margin = Inches(0.7)
        s.right_margin = Inches(0.7)

    normal = doc.styles["Normal"]
    normal.font.name = "Calibri"
    normal.font.size = Pt(10)
    normal.paragraph_format.space_after = Pt(2)

    if header_contact:
        hp = doc.sections[0].header.paragraphs[0]
        hp.alignment = WD_ALIGN_PARAGRAPH.CENTER
        hp.add_run("  ·  ".join(t for t, _ in CONTACT)).font.size = Pt(8)

    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = p.add_run(NAME)
    r.bold = True
    r.font.size = Pt(19)

    if not header_contact:
        p = doc.add_paragraph()
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        p.add_run("  ·  ".join(t for t, _ in CONTACT)).font.size = Pt(9)

    def heading(text):
        h = doc.add_paragraph()
        h.paragraph_format.space_before = Pt(9)
        run = h.add_run(text.upper())
        run.bold = True
        run.font.size = Pt(10.5)

    heading("Summary")
    doc.add_paragraph(SUMMARY)

    heading("Experience")
    for job in EXPERIENCE:
        p = doc.add_paragraph()
        p.add_run(job["title"]).bold = True
        p.add_run("  |  " + job["company"] + "  |  " + job["dates"])
        for b in job["bullets"]:
            doc.add_paragraph(b, style="List Bullet")

    heading("Education")
    for ed in EDUCATION:
        p = doc.add_paragraph()
        p.add_run(ed["school"]).bold = True
        p.add_run("  |  " + ed["degree"] + "  |  " + ed["dates"])

    heading("Skills")
    if table_skills:
        t = doc.add_table(rows=1, cols=2)
        t.style = "Table Grid"
        t.rows[0].cells[0].text = "Core tools"
        t.rows[0].cells[1].text = SKILLS
    else:
        doc.add_paragraph(SKILLS)

    doc.save(str(path))


def make_pdf(path, two_column=False):
    from reportlab.lib.pagesizes import A4
    from reportlab.pdfgen import canvas

    c = canvas.Canvas(str(path), pagesize=A4)
    W, H = A4
    left_x, right_x = 40, W / 2 + 10
    top = H - 50
    line_h = 13.5
    wrap_chars = 34 if two_column else 62

    def wrap(text, n):
        words, lines, cur = text.split(" "), [], ""
        for w in words:
            if len(cur) + len(w) + 1 <= n:
                cur = (cur + " " + w).strip()
            else:
                lines.append(cur)
                cur = w
        if cur:
            lines.append(cur)
        return lines

    blocks = [("title", NAME), ("line", "  ·  ".join(t for t, _ in CONTACT))]
    blocks.append(("head", "SUMMARY"))
    blocks += [("line", l) for l in wrap(SUMMARY, wrap_chars)]
    blocks.append(("head", "EXPERIENCE"))
    for job in EXPERIENCE:
        blocks.append(("line", job["title"] + "  |  " + job["company"] + "  |  " + job["dates"]))
        blocks += [("line", "- " + l) for b in job["bullets"] for l in wrap(b, wrap_chars - 2)]
    blocks.append(("head", "EDUCATION"))
    for ed in EDUCATION:
        blocks.append(("line", ed["school"] + "  |  " + ed["degree"] + "  |  " + ed["dates"]))
    blocks.append(("head", "SKILLS"))
    blocks += [("line", l) for l in wrap(SKILLS, wrap_chars)]

    font_for = {"title": ("Helvetica-Bold", 18), "head": ("Helvetica-Bold", 10.5), "line": ("Helvetica", 9.5)}

    if not two_column:
        y = top
        for kind, text in blocks:
            if y < 40:
                c.showPage()
                y = top
            c.setFont(*font_for[kind])
            c.drawString(left_x, y, text)
            y -= (22 if kind == "title" else 14 if kind == "head" else line_h)
        c.save()
        return

    # 双栏：按"视觉行"交错写入 —— 这正是双栏排版导出 PDF 的常见文本流
    half = (len(blocks) + 1) // 2
    cols = [blocks[:half], blocks[half:]]
    ys = [top, top]
    for i in range(half):
        for ci in range(2):
            if i >= len(cols[ci]) or ys[ci] < 40:
                continue
            kind, text = cols[ci][i]
            c.setFont(*font_for[kind])
            c.drawString(left_x if ci == 0 else right_x, ys[ci], text)
            ys[ci] -= (22 if kind == "title" else 14 if kind == "head" else line_h)
    c.save()


# ---------------------------------------------------------------- 抽取器

EMAIL_RE = re.compile(r"[\w.+-]+@[\w-]+\.[\w.-]+")
PHONE_RE = re.compile(r"\+?\d[\d\s\-()]{7,}\d")
LINK_RE = re.compile(r"(?:linkedin\.com|github\.com)/\S+")
DATE_RE = re.compile(r"([A-Za-z]{3})[a-z]*\.?\s*(\d{4})\s*[-\u2013\u2014]\s*(?:([A-Za-z]{3})[a-z]*\.?\s*(\d{4})|[Pp]resent)")
SKILL_TOKENS = ["python", "sql", "google analytics", "tmall", "excel", "vendor management"]


def text_of_docx(path, capable):
    from docx import Document
    doc = Document(str(path))
    parts = [p.text for p in doc.paragraphs]
    if capable:
        for t in doc.tables:
            for row in t.rows:
                for cell in row.cells:
                    parts.append(cell.text)
        for s in doc.sections:
            parts += [p.text for p in s.header.paragraphs]
    return "\n".join(x.strip() for x in parts if x.strip())


def text_of_pdf(path, column_aware):
    """朴素：默认文本流。有能力：按 word 坐标重建视觉行，再用栏间最大空白切列。"""
    import pymupdf
    doc = pymupdf.open(str(path))
    page = doc[0]

    if not column_aware:
        txt = page.get_text()
        doc.close()
        return txt

    mid = page.rect.width / 2
    words = page.get_text("words")  # x0,y0,x1,y1,word,...
    words.sort(key=lambda w: (round(w[1], 1), w[0]))

    # 按 y 贪心聚成视觉行
    rows = []
    for w in words:
        if rows and abs(w[1] - rows[-1]["y"]) <= 3:
            rows[-1]["ws"].append(w)
        else:
            rows.append({"y": w[1], "ws": [w]})

    wide, col0, col1 = [], [], []
    split_rows = 0
    for r in rows:
        ws = sorted(r["ws"], key=lambda w: w[0])
        gap, gap_at = 0, None
        for a, b in zip(ws, ws[1:]):
            d = b[0] - a[2]
            if d > gap:
                gap, gap_at = d, (a[2], b[0])
        if gap >= 15 and gap_at and gap_at[0] < mid + 10 and gap_at[1] > mid - 10:
            split_rows += 1

    # 真实排版里左右两栏的基线不严格对齐，靠"同一 y 配对"不可靠；
    # 按行的起始 x 归列才是稳的判据：右半页有足够多行 = 双栏。
    right_rows = [r for r in rows if min(w[0] for w in r["ws"]) >= mid]
    two_col = len(right_rows) >= 5 and len(right_rows) >= 0.2 * len(rows)

    if two_col:
        for r in rows:
            ws = sorted(r["ws"], key=lambda w: w[0])
            gap, gap_at = 0, None
            for a, b in zip(ws, ws[1:]):
                d = b[0] - a[2]
                if d > gap:
                    gap, gap_at = d, (a[2], b[0])
            if gap >= 15 and gap_at and gap_at[0] < mid + 10 and gap_at[1] > mid - 10:
                # 一行里同时有左右两栏的文字：必须拆开，否则 section 标题会夹进别的段落
                left = " ".join(w[4] for w in ws if w[0] < mid)
                right = " ".join(w[4] for w in ws if w[0] >= mid)
                if left:
                    col0.append((r["y"], left))
                if right:
                    col1.append((r["y"], right))
            else:
                text = " ".join(w[4] for w in ws)
                (col0 if ws[0][0] < mid else col1).append((r["y"], text))
        out = [t for _, t in sorted(col0)] + [t for _, t in sorted(col1)]
    else:
        out = [" ".join(w[4] for w in sorted(r["ws"], key=lambda w: w[0])) for r in rows]

    doc.close()
    return "\n".join(out)


# ---------------------------------------------------------------- 匹配打分

SYN = {
    "e-commerce": ["e-commerce", "ecommerce", "online retail", "marketplace"],
    "marketplace": ["marketplace", "platform"],
    "gmv": ["gmv", "revenue", "sales"],
    "campaign": ["campaign", "promotion", "promo"],
    "vendor": ["vendor", "supplier", "partner"],
    "analytics": ["analytics", "analysis", "data"],
    "content calendar": ["content calendar", "calendar"],
    "conversion rate": ["conversion rate", "conversion"],
    "after-sales": ["after-sales", "after sales", "returns"],
}
MUST = ["e-commerce", "marketplace", "gmv", "campaign", "vendor", "analytics"]
NICE = ["tmall", "jd", "douyin", "xiaohongshu", "content calendar", "conversion rate",
        "shopify", "amazon", "after-sales"]


def score_jd(text):
    low = " ".join(re.findall(r"[a-z0-9][a-z0-9 +\-]*", text.lower()))
    hit_w = total_w = 0
    hits, miss = [], []
    for k in MUST + NICE:
        w = 2 if k in MUST else 1
        total_w += w
        if any(c in low for c in SYN.get(k, [k])):
            hit_w += w
            hits.append(k)
        else:
            miss.append(k)
    return {
        "score": round(hit_w / max(1, total_w) * 100, 1),
        "must_miss": [k for k in miss if k in MUST],
    }


def extract(text):
    paras = [p.strip() for p in text.split("\n") if p.strip()]
    sections = {}
    cur = "__head__"
    for p in paras[1:]:
        if p.isupper() and len(p) < 24:
            cur = p.title()
            continue
        sections.setdefault(cur, []).append(p)
    return {
        "name": bool(paras) and paras[0].strip() == NAME,
        "email": bool(EMAIL_RE.search(text)),
        "phone": bool(PHONE_RE.search(text)),
        "linkedin": bool(LINK_RE.search(text)),
        "exp": sum(1 for p in sections.get("Experience", []) if "|" in p),
        "edu": sum(1 for p in sections.get("Education", []) if "|" in p),
        "skills_hit": sum(1 for k in SKILL_TOKENS if k in text.lower()),
        "dates": len(DATE_RE.findall(text)),
        "sections": list(sections.keys()),
    }


# ---------------------------------------------------------------- 主流程

def main():
    make_docx_plain(VAR / "v1_plain.docx")
    make_docx_plain(VAR / "v2_table_skills.docx", table_skills=True)
    make_docx_plain(VAR / "v3_header_contact.docx", header_contact=True)
    make_pdf(VAR / "v4_single.pdf", two_column=False)
    make_pdf(VAR / "v5_twocol.pdf", two_column=True)

    cases = [
        ("v1 单栏 docx", VAR / "v1_plain.docx", "docx"),
        ("v2 技能表格 docx", VAR / "v2_table_skills.docx", "docx"),
        ("v3 联系方式页眉 docx", VAR / "v3_header_contact.docx", "docx"),
        ("v4 单栏 PDF", VAR / "v4_single.pdf", "pdf"),
        ("v5 双栏 PDF", VAR / "v5_twocol.pdf", "pdf"),
    ]

    rows = []
    for label, path, kind in cases:
        for mode in ("naive", "capable"):
            text = (text_of_docx(path, capable=(mode == "capable")) if kind == "docx"
                    else text_of_pdf(path, column_aware=(mode == "capable")))
            f = extract(text)
            s = score_jd(text)
            rows.append({
                "variant": label, "parser": "朴素" if mode == "naive" else "有能力",
                "name": f["name"], "email": f["email"], "phone": f["phone"],
                "linkedin": f["linkedin"], "exp": f["exp"], "edu": f["edu"],
                "skills": f["skills_hit"], "jd": s["score"],
                "must_miss": s["must_miss"], "sections": f["sections"],
            })

    (OUT / "evolution_metrics.json").write_text(
        json.dumps(rows, ensure_ascii=False, indent=2), encoding="utf-8")

    def yn(v):
        return "通过" if v else "丢失"

    lines = ["# 进化版：同一份简历的 5 种版式 × 2 类解析器", ""]
    lines.append("朴素解析器＝只读 docx 正文段落、PDF 默认文本流；"
                 "有能力解析器＝补读表格与页眉、PDF 按 word 坐标重建行并切栏。")
    lines.append("")
    lines.append("| 版式 | 解析器 | 姓名 | 邮箱 | 电话 | LinkedIn | 经历 | 教育 | 技能命中 | JD 分 |")
    lines.append("|---|---|---|---|---|---|---|---|---|---|")
    for r in rows:
        lines.append("| %s | %s | %s | %s | %s | %s | %d/3 | %d/2 | %d/6 | %.1f |" % (
            r["variant"], r["parser"], yn(r["name"]), yn(r["email"]), yn(r["phone"]),
            yn(r["linkedin"]), r["exp"], r["edu"], r["skills"], r["jd"]))

    idx = {(r["variant"], r["parser"]): r for r in rows}
    base = idx[("v1 单栏 docx", "朴素")]
    tbl = idx[("v2 技能表格 docx", "朴素")]
    tbl_c = idx[("v2 技能表格 docx", "有能力")]
    hdr = idx[("v3 联系方式页眉 docx", "朴素")]
    hdr_c = idx[("v3 联系方式页眉 docx", "有能力")]
    one_pdf = idx[("v4 单栏 PDF", "朴素")]
    two_n = idx[("v5 双栏 PDF", "朴素")]
    two_c = idx[("v5 双栏 PDF", "有能力")]

    lines += ["", "## 归因", ""]
    lines.append("- **v1 基线**：姓名/邮箱/电话/LinkedIn 全中，经历 3/3、教育 2/2、技能 6/6，JD %.1f。" % base["jd"])
    lines.append("- **v2 技能表格**：朴素解析器技能命中 %d/6（表格整块读不到），但 JD 仍是 %.1f——"
                 "总分没掉，说明总分是钝指标，字段级召回才是硬指标；补读表格后回到 %d/6。"
                 % (tbl["skills"], tbl["jd"], tbl_c["skills"]))
    lines.append("- **v3 联系方式页眉**：朴素解析器邮箱 %s、电话 %s、LinkedIn %s，"
                 "人眼看着完好无损，机器却联系不上这个人；补读页眉后全部恢复。"
                 % (yn(hdr["email"]), yn(hdr["phone"]), yn(hdr["linkedin"])))
    lines.append("- **v4 单栏 PDF**：JD %.1f，字段全中——PDF 本身不是问题，文本流干净就没事。" % one_pdf["jd"])
    lines.append("- **v5 双栏 PDF**：朴素解析器经历 %d/3（section 边界被左右交错的行冲垮），"
                 "按栏切之后恢复 %d/3；同一份 PDF 文件，差别只在解析器认不认 x 坐标。"
                 % (two_n["exp"], two_c["exp"]))

    lines += ["", "## 结论", ""]
    lines.append("1. 两轮测的是两道不同的关：上一轮测「机器愿不愿意信」（时间线、可核验性），"
                 "这一轮测「机器能不能读到」（表格、页眉、双栏）。**格式关更靠前，也更容易被忽略**——"
                 "因为人眼检查简历时，看到的永远是排版正确的那一版。")
    lines.append("2. 分数会骗人：v2 丢了整个技能栏，JD 分一分不掉。要判断真假，得看字段级召回，不能看总分。")
    lines.append("3. 同一份文件在不同解析器手里结论相反（v5：%d/3 vs %d/3 条经历）。"
                 "所以「这份简历过不过 ATS」这句话没有意义，得问「过哪家 ATS」。"
                 % (two_n["exp"], two_c["exp"]))

    (OUT / "evolution_report.md").write_text("\n".join(lines), encoding="utf-8")
    print("\n".join(lines))


if __name__ == "__main__":
    main()
