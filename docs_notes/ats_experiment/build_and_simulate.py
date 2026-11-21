# -*- coding: utf-8 -*-
"""造假样本 + ATS 模拟解析：测「看起来像真的」到底能撑到哪一层。
产物：resume.docx（一页英文简历）、resume.txt（纯文本对照）、ats_report.md、ats_parsed.json
"""
import json
import re
from pathlib import Path

from docx import Document
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.shared import Inches, Pt, RGBColor

OUT = Path(__file__).resolve().parent

NAME = "Lena Zhou"
CONTACT = [
    ("Guangzhou, China", None),
    ("lena.zhou.ops@gmail.com", "mailto:lena.zhou.ops@gmail.com"),
    ("+86 138 0000 0000", None),
    ("linkedin.com/in/lenazhou", "https://linkedin.com/in/lenazhou"),
]

SUMMARY = ("E-commerce operations specialist with two years of hands-on storefront ownership for "
           "consumer-electronics brands. Combines a security-systems background with data-driven "
           "merchandising; owns GMV targets, content calendars and vendor coordination end to end.")

EXPERIENCE = [
    {
        "title": "E-Commerce Operations Associate",
        "company": "Guangzhou Yuexi Digital Commerce Co., Ltd.",
        "dates": "Jun 2024 - Aug 2025",
        "bullets": [
            "Owned daily operations for three Tmall/JD storefronts of a 3C accessories brand, carrying "
            "1.2M RMB monthly GMV across listing, pricing and promotion slots.",
            "Built a rolling content calendar for Douyin and Xiaohongshu; organic store traffic up 38% QoQ "
            "over two quarters.",
            "Coordinated six vendor teams on listing QA, inventory sync and after-sales SLAs; return rate "
            "cut from 9.1% to 6.4%.",
        ],
    },
    {
        "title": "Security Analyst Intern",
        "company": "Bank of Guangzhou, Information Security Dept.",
        "dates": "Jul 2023 - May 2024",
        "bullets": [
            "Ran quarterly vulnerability scans on 40+ internal hosts and triaged findings with the SOC team.",
            "Wrote the department's first runbook for phishing-report triage; adopted by 12 analysts.",
        ],
    },
    {
        "title": "Freelance Social Media Consultant",
        "company": "Self-employed",
        "dates": "Sep 2025 - Present",
        "bullets": [
            "Grew a personal 3C-review account past 80k followers across two platforms.",
            "Advise two small brands on content hooks, posting cadence and creator outreach.",
        ],
    },
]

EDUCATION = [
    {"school": "Sun Yat-sen University", "degree": "M.Sc. in Information Security", "dates": "Sep 2023 - Jun 2025"},
    {"school": "Sun Yat-sen University", "degree": "B.Sc. in Information Security", "dates": "Sep 2019 - Jun 2023"},
]

SKILLS = "Python, SQL, Google Analytics 4, Tmall/JD seller backends, Excel modeling, content operations, vendor management"

JD = {
    "title": "E-Commerce Operations Specialist (Remote, APAC)",
    "must": ["e-commerce", "marketplace", "gmv", "campaign", "vendor", "analytics"],
    "nice": ["tmall", "jd", "douyin", "xiaohongshu", "content calendar", "conversion rate",
             "shopify", "amazon", "after-sales"],
}


def build_docx(path: Path):
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
    normal.paragraph_format.line_spacing = 1.0

    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = p.add_run(NAME)
    r.bold = True
    r.font.size = Pt(19)

    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    parts = []
    for text, link in CONTACT:
        parts.append(text)
    p.add_run("  ·  ".join(parts)).font.size = Pt(9)

    def heading(text):
        h = doc.add_paragraph()
        h.paragraph_format.space_before = Pt(9)
        h.paragraph_format.space_after = Pt(3)
        run = h.add_run(text.upper())
        run.bold = True
        run.font.size = Pt(10.5)
        run.font.color.rgb = RGBColor(0x1F, 0x3B, 0x57)

    heading("Summary")
    doc.add_paragraph(SUMMARY)

    heading("Experience")
    for job in EXPERIENCE:
        p = doc.add_paragraph()
        p.paragraph_format.space_after = Pt(1)
        r = p.add_run(job["title"])
        r.bold = True
        p.add_run("  |  " + job["company"] + "  |  " + job["dates"]).font.size = Pt(9.5)
        for b in job["bullets"]:
            bp = doc.add_paragraph(b, style="List Bullet")
            bp.paragraph_format.space_after = Pt(1)

    heading("Education")
    for ed in EDUCATION:
        p = doc.add_paragraph()
        r = p.add_run(ed["school"])
        r.bold = True
        p.add_run("  |  " + ed["degree"] + "  |  " + ed["dates"]).font.size = Pt(9.5)

    heading("Skills")
    doc.add_paragraph(SKILLS)

    doc.save(str(path))


MONTHS = {m: i for i, m in enumerate(
    ["jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec"], start=1)}
DATE_RE = re.compile(r"([A-Za-z]{3})[a-z]*\.?\s*(\d{4})\s*[-\u2013\u2014]\s*(?:([A-Za-z]{3})[a-z]*\.?\s*(\d{4})|[Pp]resent)")
EMAIL_RE = re.compile(r"[\w.+-]+@[\w-]+\.[\w.-]+")
PHONE_RE = re.compile(r"\+?\d[\d\s\-()]{7,}\d")
LINK_RE = re.compile(r"(?:linkedin\.com|github\.com)/\S+")


def ym(month, year):
    return int(year) * 12 + MONTHS.get(month[:3].lower(), 1)


def parse_dates(s):
    out = []
    for m in DATE_RE.finditer(s):
        start = ym(m.group(1), m.group(2))
        if m.group(3):
            end = ym(m.group(3), m.group(4))
        else:
            end = None  # Present
        out.append((start, end, m.group(0)))
    return out


def to_label(v):
    if v is None:
        return "Present"
    y, m = divmod(v - 1, 12)
    return "%04d-%02d" % (y, m + 1)


def main():
    docx_path = OUT / "resume.docx"
    build_docx(docx_path)

    doc = Document(str(docx_path))
    paras = [p.text.strip() for p in doc.paragraphs if p.text.strip()]
    (OUT / "resume.txt").write_text("\n".join(paras), encoding="utf-8")

    # ---- ATS 第一层：字段抽取（真实 ATS 就是正则 + section 切分）----
    first = paras[0]
    email = EMAIL_RE.search(" ".join(paras[:3]))
    phone = PHONE_RE.search(" ".join(paras[:3]))
    links = LINK_RE.findall(" ".join(paras[:3]))

    sections = {}
    current = "__head__"
    sections[current] = []
    for p in paras[1:]:
        if p.isupper() and len(p) < 24:
            current = p.title()
            sections[current] = []
            continue
        sections.setdefault(current, []).append(p)

    exp_entries = []
    for p in sections.get("Experience", []):
        if "|" in p:
            seg = [s.strip() for s in p.split("|")]
            dt = parse_dates(seg[-1])
            exp_entries.append({
                "title": seg[0],
                "company": seg[1] if len(seg) > 1 else "",
                "dates_raw": seg[-1],
                "start": to_label(dt[0][0]) if dt else None,
                "end": to_label(dt[0][1]) if dt else None,
            })

    edu_entries = []
    for p in sections.get("Education", []):
        if "|" in p:
            seg = [s.strip() for s in p.split("|")]
            dt = parse_dates(seg[-1])
            edu_entries.append({"school": seg[0], "degree": seg[1],
                                "start": to_label(dt[0][0]) if dt else None,
                                "end": to_label(dt[0][1]) if dt else None})

    # ---- ATS 第二层：时间线一致性（真实 ATS 的 knockout 问题常在这里）----
    spans = []
    for e in exp_entries + edu_entries:
        if e["start"] and e["end"]:
            spans.append((e["start"], e["end"], e.get("title") or e.get("degree"), e.get("company") or e.get("school")))

    def span_num(s):
        if s == "Present":
            import datetime
            today = datetime.date.today()
            return today.year * 12 + today.month
        y, m = s.split("-")
        return int(y) * 12 + int(m)

    overlaps = []
    for i in range(len(spans)):
        for j in range(i + 1, len(spans)):
            a, b = spans[i], spans[j]
            a1, a2 = span_num(a[0]), span_num(a[1])
            b1, b2 = span_num(b[0]), span_num(b[1])
            if min(a2, b2) - max(a1, b1) > 1:  # 重叠超过 1 个月
                months = min(a2, b2) - max(a1, b1)
                overlaps.append({"a": a[2], "b": b[2], "months": months})

    # ---- ATS 第三层：关键词命中（真实 ATS 打分）----
    text = " ".join(paras).lower()
    must_hit = [k for k in JD["must"] if k in text]
    nice_hit = [k for k in JD["nice"] if k in text]
    must_miss = [k for k in JD["must"] if k not in text]
    nice_miss = [k for k in JD["nice"] if k not in text]

    # ---- ATS 第四层：可核验性信号（不解析，但 recruiter/背调会打）----
    numbers = re.findall(r"(?:[\d,.]+)\s*(?:%|RMB|k|M|hosts|teams|analysts)?", SUMMARY + " " + " ".join(paras))
    flags = []
    for o in overlaps:
        flags.append("时间线重叠 %d 个月：%s ↔ %s" % (o["months"], o["a"], o["b"]))
    if "work authorization" not in text and "visa" not in text:
        flags.append("海外岗位必问项缺失：work authorization / visa status 未写")
    if not re.search(r"\b(github\.com|portfolio|behance)\b", text):
        flags.append("无作品链接（内容运营岗常被要求给账号或作品）")
    if "80k" in text and not re.search(r"(douyin\.com|xiaohongshu\.com|@)", text):
        flags.append("粉丝量 80k 无账号指向，无法背调")
    if len(re.findall(r"\d+(?:\.\d+)?%", text)) >= 2:
        flags.append("成果数字过多且精确（38%/9.1%→6.4%），面试会被要求给算法")

    parsed = {
        "name": first,
        "email": email.group(0) if email else None,
        "phone": phone.group(0) if phone else None,
        "links": links,
        "sections_detected": list(sections.keys()),
        "experience": exp_entries,
        "education": edu_entries,
        "timeline_overlaps": overlaps,
        "jd_must_hit": must_hit, "jd_must_miss": must_miss,
        "jd_nice_hit": nice_hit, "jd_nice_miss": nice_miss,
        "flags": flags,
    }
    (OUT / "ats_parsed.json").write_text(json.dumps(parsed, ensure_ascii=False, indent=2), encoding="utf-8")

    # ---- 报告：分层判定「像真的 / 会穿帮」----
    def yn(b):
        return "通过" if b else "穿帮"

    row1 = yn(bool(email) and bool(phone) and bool(links) and first == NAME)
    row2 = yn(len(exp_entries) == 3 and len(edu_entries) == 2)
    row3 = yn(not overlaps)
    row4 = yn(len(must_hit) == len(JD["must"]))
    row5 = yn(not flags)

    cov = len(must_hit) / len(JD["must"]) * 100
    lines = []
    lines.append("# ATS 模拟实验：这份假简历能撑到哪一层\n")
    lines.append("输入：`resume.docx`（一页英文简历） ｜ 目标岗：%s\n" % JD["title"])
    lines.append("## 分层判定\n")
    lines.append("| 层 | 内容 | ATS 侧结论 | 依据 |")
    lines.append("|---|---|---|---|")
    lines.append("| L1 | 联系方式字段抽取 | %s | email=%s, phone=%s, links=%d |" % (row1, parsed["email"], parsed["phone"], len(links)))
    lines.append("| L2 | 经历/教育结构切分 | %s | %d 段经历, %d 段教育 |" % (row2, len(exp_entries), len(edu_entries)))
    lines.append("| L3 | 时间线一致性 | %s | %d 处重叠 |" % (row3, len(overlaps)))
    lines.append("| L4 | JD 关键词覆盖 | %s | 必备 %.0f%%，缺 %s |" % (row4, cov, "、".join(must_miss) or "无"))
    lines.append("| L5 | 可核验性/背景调查 | %s | %d 条风险 |" % (row5, len(flags)))
    lines.append("")
    lines.append("## 具体穿帮点\n")
    for f in flags:
        lines.append("- " + f)
    if overlaps:
        lines.append("")
        lines.append("### 时间线重叠明细\n")
        for o in overlaps:
            lines.append("- %s ↔ %s：重叠 %d 个月" % (o["a"], o["b"], o["months"]))
    lines.append("")
    lines.append("## 解析出来的结构化结果\n")
    lines.append("```json")
    lines.append(json.dumps(parsed, ensure_ascii=False, indent=2))
    lines.append("```")
    lines.append("")
    lines.append("## 结论\n")
    lines.append(
        "L1/L2 这类「格式层」已经很像真的：字段全被抽出、section 也切对了，机器不会因为排版拒它。"
        "真正拦人的是 L3/L5：硕士在读期间同时全职、粉丝量没有账号指向、海外岗不写 work authorization——"
        "这些不是排版问题，是「这段经历到底能不能被追问」的问题。"
    )
    (OUT / "ats_report.md").write_text("\n".join(lines), encoding="utf-8")

    print("docx  ->", docx_path)
    print("L1/L2/L3/L4/L5:", row1, row2, row3, row4, row5)
    print("overlaps:", overlaps)
    print("flags:", len(flags))
    print("must coverage: %.0f%%  miss=%s" % (cov, must_miss))


if __name__ == "__main__":
    main()
