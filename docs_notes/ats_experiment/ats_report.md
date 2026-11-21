# ATS 模拟实验：这份假简历能撑到哪一层

输入：`resume.docx`（一页英文简历） ｜ 目标岗：E-Commerce Operations Specialist (Remote, APAC)

## 分层判定

| 层 | 内容 | ATS 侧结论 | 依据 |
|---|---|---|---|
| L1 | 联系方式字段抽取 | 通过 | email=lena.zhou.ops@gmail.com, phone=+86 138 0000 0000, links=1 |
| L2 | 经历/教育结构切分 | 通过 | 3 段经历, 2 段教育 |
| L3 | 时间线一致性 | 穿帮 | 2 处重叠 |
| L4 | JD 关键词覆盖 | 穿帮 | 必备 67%，缺 marketplace、campaign |
| L5 | 可核验性/背景调查 | 穿帮 | 5 条风险 |

## 具体穿帮点

- 时间线重叠 12 个月：E-Commerce Operations Associate ↔ M.Sc. in Information Security
- 时间线重叠 8 个月：Security Analyst Intern ↔ M.Sc. in Information Security
- 海外岗位必问项缺失：work authorization / visa status 未写
- 无作品链接（内容运营岗常被要求给账号或作品）
- 成果数字过多且精确（38%/9.1%→6.4%），面试会被要求给算法

### 时间线重叠明细

- E-Commerce Operations Associate ↔ M.Sc. in Information Security：重叠 12 个月
- Security Analyst Intern ↔ M.Sc. in Information Security：重叠 8 个月

## 解析出来的结构化结果

```json
{
  "name": "Lena Zhou",
  "email": "lena.zhou.ops@gmail.com",
  "phone": "+86 138 0000 0000",
  "links": [
    "linkedin.com/in/lenazhou"
  ],
  "sections_detected": [
    "__head__",
    "Summary",
    "Experience",
    "Education",
    "Skills"
  ],
  "experience": [
    {
      "title": "E-Commerce Operations Associate",
      "company": "Guangzhou Yuexi Digital Commerce Co., Ltd.",
      "dates_raw": "Jun 2024 - Aug 2025",
      "start": "2024-06",
      "end": "2025-08"
    },
    {
      "title": "Security Analyst Intern",
      "company": "Bank of Guangzhou, Information Security Dept.",
      "dates_raw": "Jul 2023 - May 2024",
      "start": "2023-07",
      "end": "2024-05"
    },
    {
      "title": "Freelance Social Media Consultant",
      "company": "Self-employed",
      "dates_raw": "Sep 2025 - Present",
      "start": "2025-09",
      "end": "Present"
    }
  ],
  "education": [
    {
      "school": "Sun Yat-sen University",
      "degree": "M.Sc. in Information Security",
      "start": "2023-09",
      "end": "2025-06"
    },
    {
      "school": "Sun Yat-sen University",
      "degree": "B.Sc. in Information Security",
      "start": "2019-09",
      "end": "2023-06"
    }
  ],
  "timeline_overlaps": [
    {
      "a": "E-Commerce Operations Associate",
      "b": "M.Sc. in Information Security",
      "months": 12
    },
    {
      "a": "Security Analyst Intern",
      "b": "M.Sc. in Information Security",
      "months": 8
    }
  ],
  "jd_must_hit": [
    "e-commerce",
    "gmv",
    "vendor",
    "analytics"
  ],
  "jd_must_miss": [
    "marketplace",
    "campaign"
  ],
  "jd_nice_hit": [
    "tmall",
    "jd",
    "douyin",
    "xiaohongshu",
    "content calendar",
    "after-sales"
  ],
  "jd_nice_miss": [
    "conversion rate",
    "shopify",
    "amazon"
  ],
  "flags": [
    "时间线重叠 12 个月：E-Commerce Operations Associate ↔ M.Sc. in Information Security",
    "时间线重叠 8 个月：Security Analyst Intern ↔ M.Sc. in Information Security",
    "海外岗位必问项缺失：work authorization / visa status 未写",
    "无作品链接（内容运营岗常被要求给账号或作品）",
    "成果数字过多且精确（38%/9.1%→6.4%），面试会被要求给算法"
  ]
}
```

## 结论

L1/L2 这类「格式层」已经很像真的：字段全被抽出、section 也切对了，机器不会因为排版拒它。真正拦人的是 L3/L5：硕士在读期间同时全职、粉丝量没有账号指向、海外岗不写 work authorization——这些不是排版问题，是「这段经历到底能不能被追问」的问题。