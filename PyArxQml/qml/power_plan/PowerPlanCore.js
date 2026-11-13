.pragma library

// PowerPlanCore.js —— 配电干线图纯逻辑内核（从 G:\配电规划设计\power-plan.html 的 CORE 段原样移植）
// 只放不依赖 DOM 的算法：数据校验 / 建树 / 布局 / 交叉自检 / CSV / 示例数据 / SVG 导出。

var CFG = {
    boxW: 132, boxH: 48,      // 电柜/起点基准
    termW: 120, termH: 44,    // 末端负载基准
    nameH: 8,                 // 有名称时增加的高度
    gapX: 30, gapY: 56,
    margin: 24, rootGap: 64,
    curveSteps: 16
};

var CSV_HEADER = ['编号', '名称', '上游', '电缆型号', '负载kW', '电压', '长度m'];

var ALIAS = {
    '编号': 'code', 'code': 'code',
    '名称': 'name', 'name': 'name',
    '上游': 'upstream', 'upstream': 'upstream',
    '电缆型号': 'cable', 'cable': 'cable',
    '电压': 'voltage', 'voltage': 'voltage', '相数': 'voltage',
    '长度': 'length', '长度m': 'length', 'length': 'length', 'lengthm': 'length',
    '负载kw': 'loadKw', '负载': 'loadKw', 'loadkw': 'loadKw', 'loadKw': 'loadKw'
};

// 中文记 2 个半角单位，ASCII 记 1 个
function textUnits(s) {
    if (!s) return 0;
    s = String(s);
    var u = 0;
    for (var i = 0; i < s.length; i++) {
        u += (s.charCodeAt(i) >= 0x2E80) ? 2 : 1;
    }
    return u;
}

function cableLine(n) {
    var s = n.cable ? String(n.cable) : '';
    if (n.loadKw && n.loadKw > 0) s += (s ? ' · ' : '') + n.loadKw + ' kW';
    if (n.length && n.length > 0) s += (s ? ' · ' : '') + n.length + ' m';
    return s;
}

// 宽 = max(基准, 20 + 半角单位数 x 8)
function cardSize(role, hasName, lines) {
    var baseW = role === 'terminal' ? CFG.termW : CFG.boxW;
    var baseH = role === 'terminal' ? CFG.termH : CFG.boxH;
    var maxU = 0;
    for (var i = 0; i < lines.length; i++) {
        if (lines[i]) maxU = Math.max(maxU, textUnits(lines[i]));
    }
    return { w: Math.max(baseW, 20 + maxU * 8), h: baseH + (hasName ? CFG.nameH : 0) };
}

// 电压等级：380 = 三相低压，220 = 单相低压，其余按 kV 值
function normVoltage(v) {
    var s = String(v === undefined || v === null ? '' : v).trim();
    if (!s) return '380';
    var lower = s.toLowerCase().replace(/\s+/g, '');
    var kv = lower.match(/([0-9]+(?:\.[0-9]+)?)\s*(kv|千伏)/);
    if (kv) return kv[1].replace(/\.0+$/, '') + 'kV';
    if (lower.indexOf('380') >= 0 || lower.indexOf('三相') >= 0 || lower.indexOf('3ph') >= 0) return '380';
    if (lower.indexOf('220') >= 0 || lower.indexOf('单相') >= 0 || lower.indexOf('1ph') >= 0) return '220';
    return '380';
}

function voltClass(v) {
    if (v === '220' || v === '380') return v === '220' ? 'lv220' : 'lv380';
    var n = parseFloat(v);
    if (!isFinite(n)) return 'lv380';
    return n >= 66 ? 'hv' : 'mv';
}

function voltText(v) {
    if (v === '220') return '220V 单相';
    if (v === '380') return '380V 三相';
    var c = voltClass(v);
    return v + (c === 'hv' ? ' 高压' : ' 中压');
}

function voltStyle(v) {
    var c = voltClass(v);
    if (c === 'lv220') return { text: '220V', color: '#B54708', bg: '#FEF6E7' };
    if (c === 'mv') return { text: v, color: '#0F766E', bg: '#E6FFFA' };
    if (c === 'hv') return { text: v, color: '#A8071A', bg: '#FEF3F2' };
    return { text: '380V', color: '#1F64B0', bg: '#E8F1FB' };
}

// 电压下拉选项：常用档 + 数据里出现的非标准档（保证原值不被改掉）
function voltageOptions(cur) {
    var base = [
        ['380', '380V 三相'], ['220', '220V 单相'],
        ['10kV', '10kV 中压'], ['35kV', '35kV 中压'], ['110kV', '110kV 高压']
    ];
    var list = base.slice();
    var hit = false;
    for (var i = 0; i < list.length; i++) if (list[i][0] === cur) hit = true;
    if (cur && !hit) list.push([cur, voltText(cur)]);
    var out = [];
    for (var j = 0; j < list.length; j++) {
        out.push({ value: list[j][0], label: list[j][1], selected: (cur === list[j][0]) });
    }
    return out;
}

// ---- CSV：RFC4180 解析 / 编码（导出带 BOM）----
function parseCSV(text) {
    var src = String(text).replace(/\uFEFF/g, '');
    var rows = [];
    var row = [], cur = '', inQuote = false;
    for (var i = 0; i < src.length; i++) {
        var c = src.charAt(i);
        if (inQuote) {
            if (c === '"') {
                if (src.charAt(i + 1) === '"') { cur += '"'; i++; }
                else inQuote = false;
            } else cur += c;
        } else {
            if (c === '"') inQuote = true;
            else if (c === ',') { row.push(cur); cur = ''; }
            else if (c === '\r') { /* 忽略 */ }
            else if (c === '\n') { row.push(cur); rows.push(row); row = []; cur = ''; }
            else cur += c;
        }
    }
    row.push(cur); rows.push(row);
    while (rows.length && rows[rows.length - 1].every(function (v) { return String(v).trim() === ''; })) rows.pop();
    if (!rows.length) throw new Error('数据为空：没有可解析的表格行');

    var header = rows[0].map(function (h) { return String(h).trim(); });
    var cols = header.map(function (h) { return ALIAS[h.toLowerCase()] || ALIAS[h] || null; });
    var hasCode = false;
    for (var k = 0; k < cols.length; k++) if (cols[k] === 'code') hasCode = true;
    if (!hasCode) throw new Error('缺少必需列「编号」');

    var out = [];
    for (var r = 1; r < rows.length; r++) {
        var cells = rows[r];
        if (cells.length === 1 && String(cells[0]).trim() === '') continue;
        var rec = { __row: r };
        for (var cI = 0; cI < cols.length; cI++) {
            var key = cols[cI];
            if (!key) continue;
            rec[key] = String(cells[cI] === undefined ? '' : cells[cI]).trim();
        }
        out.push(rec);
    }
    return out;
}

function csvEscape(v) {
    var s = String(v === undefined || v === null ? '' : v);
    return (s.indexOf('"') >= 0 || s.indexOf(',') >= 0 || s.indexOf('\n') >= 0)
        ? '"' + s.replace(/"/g, '""') + '"' : s;
}

function toCSV(rows) {
    var out = '\uFEFF' + CSV_HEADER.join(',') + '\r\n';
    for (var i = 0; i < rows.length; i++) {
        var r = rows[i];
        out += [r.code, r.name, r.upstream, r.cable, (r.loadKw || 0), (r.voltage || '380'),
            (r.length || 0)].map(csvEscape).join(',') + '\r\n';
    }
    return out;
}

// ---- 建树 + 校验 ----
function buildPlan(records) {
    var warnings = [];
    var map = {};
    var order = [];

    for (var idx = 0; idx < records.length; idx++) {
        var r = records[idx];
        var code = String(r.code === undefined ? '' : r.code).trim();
        if (!code) throw new Error('第 ' + (idx + 1) + ' 行缺少编号');
        if (map[code]) throw new Error('编号重复：' + code);
        var loadKw = parseFloat(r.loadKw);
        if (!isFinite(loadKw) || loadKw < 0) loadKw = 0;
        var len = parseFloat(r.length);
        if (!isFinite(len) || len < 0) len = 0;
        var node = {
            code: code,
            name: String(r.name === undefined ? '' : r.name).trim(),
            upstream: String(r.upstream === undefined ? '' : r.upstream).trim(),
            cable: String(r.cable === undefined ? '' : r.cable).trim(),
            loadKw: loadKw,
            length: len,
            voltage: normVoltage(r.voltage),
            children: [],
            parent: null,
            depth: 0,
            collapsedCount: 0,
            _loadKwRaw: r.loadKw
        };
        map[code] = node;
        order.push(node);
    }

    var roots = [];
    for (var a = 0; a < order.length; a++) {
        var n = order[a];
        if (!n.upstream) { roots.push(n); continue; }
        var p = map[n.upstream];
        if (!p) {
            warnings.push('节点 ' + n.code + ' 的上游 ' + n.upstream + ' 不存在，已按起点处理');
            n.upstream = '';
            roots.push(n);
            continue;
        }
        if (p === n) throw new Error('上游关系成环，节点：' + n.code);
        n.parent = p;
        p.children.push(n);
    }

    // 兄弟按 code 稳定排序（不依赖 ICU，保证同一份数据出图一致）
    var cmp = function (x, y) { return x.code < y.code ? -1 : (x.code > y.code ? 1 : 0); };
    for (var b = 0; b < order.length; b++) order[b].children.sort(cmp);
    roots.sort(cmp);

    // 深度
    var stack = roots.map(function (rr) { return [rr, 0]; });
    var visited = {};
    while (stack.length) {
        var item = stack.pop();
        var nn = item[0], d = item[1];
        if (visited[nn.code]) continue;
        visited[nn.code] = 1;
        nn.depth = d;
        for (var ci = 0; ci < nn.children.length; ci++) stack.push([nn.children[ci], d + 1]);
    }
    var missed = order.filter(function (x) { return !visited[x.code]; });
    if (missed.length) throw new Error('上游关系成环，节点：' + missed[0].code);

    return { nodes: order, roots: roots, map: map, warnings: warnings };
}

// ---- 自动布局 ----
function layoutPlan(plan, dir) {
    var HORIZ = dir === 'h';
    var nodes = plan.nodes;

    for (var i = 0; i < nodes.length; i++) {
        var n = nodes[i];
        var role = !n.upstream ? 'root'
            : ((n.children.length || n.collapsedCount) ? 'branch' : 'terminal');
        n.role = role;
        var lines = [n.code];
        if (n.name) lines.push(n.name);
        var l3 = cableLine(n);
        if (l3) lines.push(l3);
        var s = cardSize(role, !!n.name, lines);
        n.w = s.w; n.h = s.h;
    }

    // 纵向布局按子树的"宽"累加；横向布局按子树的"高"累加
    var calcSub = function (n) {
        var own = HORIZ ? n.h : n.w;
        if (!n.children.length) { n.subW = own; return n.subW; }
        var sum = 0;
        for (var k = 0; k < n.children.length; k++) {
            sum += calcSub(n.children[k]) + (k ? CFG.gapX : 0);
        }
        n.subW = Math.max(own, sum);
        return n.subW;
    };
    for (var ri = 0; ri < plan.roots.length; ri++) calcSub(plan.roots[ri]);

    // 前序摆放：子树在主轴（纵向=横向 x，横向=纵向 y）上占 [start, start + subW)
    var place = function (n, start) {
        n.treeLeft = start;
        if (!n.children.length) {
            if (HORIZ) n.y = start + (n.subW - n.h) / 2;
            else n.x = start + (n.subW - n.w) / 2;
            return;
        }
        var kids = n.children;
        var kidsExt = 0;
        for (var k = 0; k < kids.length; k++) kidsExt += kids[k].subW + (k ? CFG.gapX : 0);
        var cur = start + (n.subW - kidsExt) / 2;
        var first = null, last = null;
        for (var j = 0; j < kids.length; j++) {
            place(kids[j], cur);
            if (!first) first = kids[j];
            last = kids[j];
            cur += kids[j].subW + CFG.gapX;
        }
        // 父卡片居中于「首个子中心 + 末个子中心」的中点
        if (HORIZ) {
            var midY = (first.y + first.h / 2 + last.y + last.h / 2) / 2;
            n.y = midY - n.h / 2;
        } else {
            var midX = (first.x + first.w / 2 + last.x + last.w / 2) / 2;
            n.x = midX - n.w / 2;
        }
    };

    var rootStart = CFG.margin;
    for (var r2 = 0; r2 < plan.roots.length; r2++) {
        place(plan.roots[r2], rootStart);
        rootStart += plan.roots[r2].subW + CFG.rootGap;
    }

    // 层级坐标：纵向 = 行，横向 = 列
    var byDepth = {};
    var maxDepth = 0;
    for (var di = 0; di < nodes.length; di++) {
        var dn = nodes[di];
        if (!byDepth[dn.depth]) byDepth[dn.depth] = [];
        byDepth[dn.depth].push(dn);
        if (dn.depth > maxDepth) maxDepth = dn.depth;
    }
    var cursor = CFG.margin;
    for (var d = 0; d <= maxDepth; d++) {
        var bucket = byDepth[d] || [];
        if (HORIZ) {
            var colW = bucket.length ? Math.max.apply(null, bucket.map(function (x) { return x.w; })) : CFG.boxW;
            // 横向：同列卡片左端对齐，保证同一父节点的各条出线 x 坐标完全一致，避免 S 线互相交叉
            for (var q1 = 0; q1 < bucket.length; q1++) bucket[q1].x = cursor;
            cursor += colW + 100;
        } else {
            var rowH = bucket.length ? Math.max.apply(null, bucket.map(function (x) { return x.h; })) : CFG.boxH;
            for (var q2 = 0; q2 < bucket.length; q2++) bucket[q2].y = cursor + (rowH - bucket[q2].h) / 2;
            cursor += rowH + CFG.gapY;
        }
    }

    // 输出节点（按 code 排序，保证导出稳定）
    var outNodes = nodes.slice().sort(function (x, y) {
        return x.code < y.code ? -1 : (x.code > y.code ? 1 : 0);
    }).map(function (x) {
        return { code: x.code, x: x.x, y: x.y, w: x.w, h: x.h, depth: x.depth };
    });

    // 连线：纵向 = 父底边中点 → 子顶边中点（竖直 S）；横向 = 父右边中点 → 子左边中点（水平 S）
    var links = [];
    for (var ni = 0; ni < nodes.length; ni++) {
        var pn = nodes[ni];
        for (var cj = 0; cj < pn.children.length; cj++) {
            var c2n = pn.children[cj];
            var x1, y1, x2, y2, bezier;
            if (HORIZ) {
                x1 = pn.x + pn.w; y1 = pn.y + pn.h / 2;
                x2 = c2n.x; y2 = c2n.y + c2n.h / 2;
                var mx = (x1 + x2) / 2;
                bezier = [x1, y1, mx, y1, mx, y2, x2, y2];
            } else {
                x1 = pn.x + pn.w / 2; y1 = pn.y + pn.h;
                x2 = c2n.x + c2n.w / 2; y2 = c2n.y;
                var my = (y1 + y2) / 2;
                bezier = [x1, y1, x1, my, x2, my, x2, y2];
            }
            links.push({
                from: pn.code, to: c2n.code, cable: c2n.cable,
                x1: x1, y1: y1, x2: x2, y2: y2, bezier: bezier
            });
        }
    }
    links.sort(function (x, y) {
        return x.from < y.from ? -1 : (x.from > y.from ? 1 :
            (x.to < y.to ? -1 : (x.to > y.to ? 1 : 0)));
    });

    var maxX = -Infinity, maxY = -Infinity;
    for (var oi = 0; oi < outNodes.length; oi++) {
        if (outNodes[oi].x + outNodes[oi].w > maxX) maxX = outNodes[oi].x + outNodes[oi].w;
        if (outNodes[oi].y + outNodes[oi].h > maxY) maxY = outNodes[oi].y + outNodes[oi].h;
    }
    var width = (outNodes.length ? maxX : 0) + CFG.margin;
    var height = (outNodes.length ? maxY : 0) + CFG.margin;

    return { width: width, height: height, nodes: outNodes, links: links, byDepth: byDepth, dir: HORIZ ? 'h' : 'v' };
}

// ---- 贝塞尔采样 + «真穿过» 线段求交 ----
function sampleBezier(b, steps) {
    var pts = [];
    var p0x = b[0], p0y = b[1], c1x = b[2], c1y = b[3];
    var c2x = b[4], c2y = b[5], p3x = b[6], p3y = b[7];
    for (var i = 0; i <= steps; i++) {
        var t = i / steps, mt = 1 - t;
        var k0 = mt * mt * mt, k1 = 3 * mt * mt * t, k2 = 3 * mt * t * t, k3 = t * t * t;
        pts.push({
            x: k0 * p0x + k1 * c1x + k2 * c2x + k3 * p3x,
            y: k0 * p0y + k1 * c1y + k2 * c2y + k3 * p3y
        });
    }
    return pts;
}

function segCross(o, a, b) {
    return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
}

function segProperIntersect(a, b, c, d) {
    var eps = 1e-9;
    var d1 = segCross(c, d, a), d2 = segCross(c, d, b);
    var d3 = segCross(a, b, c), d4 = segCross(a, b, d);
    var s12 = (d1 > eps && d2 < -eps) || (d1 < -eps && d2 > eps);
    var s34 = (d3 > eps && d4 < -eps) || (d3 < -eps && d4 > eps);
    return s12 && s34;
}

function countCrossings(links, steps) {
    var packs = links.map(function (l) {
        var pts = sampleBezier(l.bezier, steps);
        var x0 = Infinity, y0 = Infinity, x1 = -Infinity, y1 = -Infinity;
        for (var i = 0; i < pts.length; i++) {
            var p = pts[i];
            if (p.x < x0) x0 = p.x;
            if (p.x > x1) x1 = p.x;
            if (p.y < y0) y0 = p.y;
            if (p.y > y1) y1 = p.y;
        }
        return { pts: pts, x0: x0, y0: y0, x1: x1, y1: y1 };
    });
    var n = 0;
    for (var i2 = 0; i2 < packs.length; i2++) {
        for (var j = i2 + 1; j < packs.length; j++) {
            var A = packs[i2], B = packs[j];
            if (A.x1 < B.x0 || B.x1 < A.x0 || A.y1 < B.y0 || B.y1 < A.y0) continue;
            var pa = A.pts, pb = B.pts;
            for (var p1 = 0; p1 + 1 < pa.length; p1++) {
                for (var q1 = 0; q1 + 1 < pb.length; q1++) {
                    if (segProperIntersect(pa[p1], pa[p1 + 1], pb[q1], pb[q1 + 1])) n++;
                }
            }
        }
    }
    return n;
}

function countOverlaps(nodes) {
    var n = 0;
    for (var i = 0; i < nodes.length; i++) {
        for (var j = i + 1; j < nodes.length; j++) {
            var a = nodes[i], b = nodes[j];
            var apart = a.x + a.w <= b.x || b.x + b.w <= a.x || a.y + a.h <= b.y || b.y + b.h <= a.y;
            if (!apart) n++;
        }
    }
    return n;
}

// 同层横向最小净距
function minRowGap(nodes) {
    var byDepth = {};
    for (var i = 0; i < nodes.length; i++) {
        var n = nodes[i];
        if (!byDepth[n.depth]) byDepth[n.depth] = [];
        byDepth[n.depth].push(n);
    }
    var min = Infinity;
    for (var key in byDepth) {
        var list = byDepth[key].slice().sort(function (a, b) { return a.x - b.x; });
        for (var j = 1; j < list.length; j++) {
            var gap = list[j].x - (list[j - 1].x + list[j - 1].w);
            if (gap < min) min = gap;
        }
    }
    return min === Infinity ? null : min;
}

// ==================================================================
// 示例数据
// ==================================================================
var SAMPLE_CSV = [
    '编号,名称,上游,电缆型号,负载kW,电压,长度m',
    'TR1,1号变压器,,,0,10kV,0',
    'AT1,总配电柜,TR1,YJV22-4×95+1×50,0,380,12',
    'AP1,1号照明配电箱,AT1,YJV22-4×35+1×16,0,380,28',
    'AP2,2号动力配电箱,AT1,YJV22-4×50+1×25,0,380,35',
    'AP3,3号空调配电箱,AT1,YJV22-4×25+1×16,0,380,42',
    'WL1,照明回路一,AP1,BV-3×4,3,220,8',
    'WL2,照明回路二,AP1,BV-3×4,2.5,220,10',
    'WL3,插座回路,AP1,BV-3×6,5,220,12',
    'M1,生活水泵1,AP2,YJV-4×10,11,380,15',
    'M2,生活水泵2,AP2,YJV-4×10,11,380,18',
    'K1,空调机组1,AP3,YJV-4×6,7,380,22',
    'K2,空调机组2,AP3,YJV-4×6,7,380,25'
].join('\n');

// 三级配电示例：起点 -> 2 个电柜 -> 每柜 3 路二级回路 -> 每路 5 个三级回路
function buildThreeLevelCsv() {
    var rows = [['编号', '名称', '上游', '电缆型号', '负载kW', '电压', '长度m']];
    rows.push(['TR1', '1号变压器', '', '', '0', '10kV', '0']);
    for (var i = 1; i <= 2; i++) {
        var cab = 'AT' + i;
        rows.push([cab, i + '号配电柜', 'TR1', 'YJV22-4×95+1×50', '0', '380', i === 1 ? '15' : '25']);
        for (var j = 1; j <= 3; j++) {
            var two = cab + '-' + j;
            rows.push([two, i + '号柜' + j + '号回路', cab, 'YJV-4×35+1×16', '0', '380', String(10 * j)]);
            for (var k = 1; k <= 5; k++) {
                var three = two + '-' + k;
                var volt = k <= 3 ? '220' : '380';
                rows.push([three, i + '号柜' + j + '-' + k + '末端', two, 'BV-3×4', String(2 + k),
                    volt, String(3 + k * 2)]);
            }
        }
    }
    return rows.map(function (r) { return r.join(','); }).join('\n');
}

// ==================================================================
// 图元描述：供 Canvas 绘制与 SVG 导出共用同一份样式
// ==================================================================
function nodeLines(n, role) {
    var lines = [];
    lines.push({ t: n.code, size: role === 'root' ? 14 : 13, weight: 600, cls: 't-code' });
    if (n.name) lines.push({ t: n.name, size: 10.5, weight: 400, cls: 't-name' });
    var l3 = cableLine(n);
    if (l3) lines.push({ t: l3, size: 10.5, weight: 400, cls: 't-cable' });
    return lines;
}

function nodePaint(n, planNode) {
    var role = planNode ? planNode.role : 'branch';
    return {
        role: role,
        fill: role === 'terminal' ? '#EAF2FB' : '#FFFFFF',
        stroke: role === 'root' ? '#222222' : (role === 'terminal' ? '#74869C' : '#8A94A6'),
        lineWidth: role === 'root' ? 2 : 1,
        radius: 2,
        voltage: planNode ? planNode.voltage : '',
        voltageStyle: planNode ? voltStyle(planNode.voltage) : null,
        collapsedCount: planNode ? (planNode.collapsedCount || 0) : 0,
        lines: nodeLines(planNode || { code: n.code }, role)
    };
}

// SVG 导出：连线 + 节点（结构与 HTML 版 buildGraphString 等价）
function buildSvgGraph(layout, plan, labels, colorOf) {
    var s = '';
    for (var i = 0; i < layout.links.length; i++) {
        var l = layout.links[i];
        var d = 'M ' + l.x1 + ' ' + l.y1 + ' C ' + l.bezier[2] + ' ' + l.bezier[3] +
            ' ' + l.bezier[4] + ' ' + l.bezier[5] + ' ' + l.x2 + ' ' + l.y2;
        var stroke = colorOf(l.from + '>' + l.to);
        s += '<path class="link" data-edge="' + escXml(l.from + '>' + l.to) + '" d="' + d +
            '" fill="none" stroke="' + stroke + '" stroke-width="' + (stroke === '#E15759' ? 2.6 : 2) +
            '" stroke-linecap="round"/>';
        if (labels && l.cable) {
            if (layout.dir === 'h') {
                var mx = (l.x1 + l.x2) / 2;
                s += '<text class="cable-label" x="' + mx + '" y="' + ((l.y1 + l.y2) / 2 - 5) +
                    '" text-anchor="middle" font-size="10" fill="#1F64B0">' + escXml(l.cable) + '</text>';
            } else {
                var mx2 = (l.x1 + l.x2) / 2 + 6;
                var my2 = (l.y1 + l.y2) / 2 + 3;
                s += '<text class="cable-label" x="' + mx2 + '" y="' + my2 +
                    '" font-size="10" fill="#1F64B0">' + escXml(l.cable) + '</text>';
            }
        }
    }

    for (var ni = 0; ni < layout.nodes.length; ni++) {
        var n = layout.nodes[ni];
        var pn = plan.map[n.code];
        var paint = nodePaint(n, pn);
        s += '<g class="node" data-code="' + escXml(n.code) + '">';
        s += '<rect x="' + n.x + '" y="' + n.y + '" width="' + n.w + '" height="' + n.h +
            '" rx="' + paint.radius + '" fill="' + paint.fill + '" stroke="' + paint.stroke +
            '" stroke-width="' + paint.lineWidth + '"/>';

        if (pn && pn.voltage) {
            var vs = paint.voltageStyle;
            var bw = 8 + textUnits(vs.text) * 4.6;
            var bh = 11;
            var bx = n.x + n.w - bw - 4, by = n.y + 3;
            s += '<rect x="' + bx + '" y="' + by + '" width="' + bw + '" height="' + bh +
                '" rx="2" fill="' + vs.bg + '" stroke="' + vs.color + '" stroke-width="0.7"/>';
            s += '<text x="' + (bx + bw / 2) + '" y="' + (by + 8.6) +
                '" text-anchor="middle" font-size="8.5" fill="' + vs.color +
                '" font-weight="600">' + escXml(vs.text) + '</text>';
        }

        var lines = paint.lines;
        var blockH = 16 + 13 * (lines.length - 1);
        var ty = n.y + (n.h - blockH) / 2 + 12;
        var cx = n.x + n.w / 2;
        for (var li = 0; li < lines.length; li++) {
            var ln = lines[li];
            var fill = ln.cls === 't-code' ? '#1F2A37' : (ln.cls === 't-name' ? '#5B6675' : '#1F64B0');
            s += '<text x="' + cx + '" y="' + ty + '" text-anchor="middle" font-size="' + ln.size +
                '" font-weight="' + ln.weight + '" fill="' + fill + '">' + escXml(ln.t) + '</text>';
            ty += 13;
        }
        if (paint.collapsedCount) {
            s += '<text x="' + (n.x + n.w - 6) + '" y="' + (n.y + 12) +
                '" text-anchor="end" font-size="9" fill="#8A94A6">+ ' + paint.collapsedCount + '</text>';
        }
        s += '</g>';
    }
    return s;
}

function escXml(s) {
    return String(s === undefined || s === null ? '' : s)
        .replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;').replace(/'/g, '&#39;');
}

// 绘制指令：交给 C++ 侧 SvgGraph 图元用 QPainter 直接画（每帧矢量重绘，无 SVG 解析开销）
// links: [x1,y1,c1x,c1y,c2x,c2y,x2,y2,color,width]
// rects: [x,y,w,h,radius,fill,stroke,lineWidth]
// texts: [x,y,text,size,weight,color,align]   align 0=居中 1=左 2=右
function buildDrawOps(layout, plan, labels, colorOf, hoverCode) {
    var links = [], rects = [], texts = [];

    for (var i = 0; i < layout.links.length; i++) {
        var l = layout.links[i];
        var stroke = colorOf(l.from + '>' + l.to);
        links.push([l.x1, l.y1, l.bezier[2], l.bezier[3], l.bezier[4], l.bezier[5],
                    l.x2, l.y2, stroke, stroke === '#E15759' ? 2.6 : 2]);
        if (labels && l.cable) {
            if (layout.dir === 'h')
                texts.push([(l.x1 + l.x2) / 2, (l.y1 + l.y2) / 2 - 5, l.cable, 10, 400, '#1F64B0', 0]);
            else
                texts.push([(l.x1 + l.x2) / 2 + 6, (l.y1 + l.y2) / 2 + 3, l.cable, 10, 400, '#1F64B0', 1]);
        }
    }

    for (var ni = 0; ni < layout.nodes.length; ni++) {
        var n = layout.nodes[ni];
        var pn = plan.map[n.code];
        var paint = nodePaint(n, pn);
        rects.push([n.x, n.y, n.w, n.h, paint.radius, paint.fill, paint.stroke, paint.lineWidth]);

        if (pn && pn.voltage) {
            var vs = paint.voltageStyle;
            var bw = 8 + textUnits(vs.text) * 4.6, bh = 11;
            var bx = n.x + n.w - bw - 4, by = n.y + 3;
            rects.push([bx, by, bw, bh, 2, vs.bg, vs.color, 0.7]);
            texts.push([bx + bw / 2, by + 8.6, vs.text, 8.5, 600, vs.color, 0]);
        }

        var lines = paint.lines;
        var blockH = 16 + 13 * (lines.length - 1);
        var ty = n.y + (n.h - blockH) / 2 + 12;
        var cx = n.x + n.w / 2;
        for (var li = 0; li < lines.length; li++) {
            var ln = lines[li];
            var col = ln.cls === 't-code' ? '#1F2A37' : (ln.cls === 't-name' ? '#5B6675' : '#1F64B0');
            texts.push([cx, ty, ln.t, ln.size, ln.weight, col, 0]);
            ty += 13;
        }
        if (paint.collapsedCount)
            texts.push([n.x + n.w - 6, n.y + 12, '+ ' + paint.collapsedCount, 9, 400, '#8A94A6', 2]);
        if (hoverCode && n.code === hoverCode)      // 悬停高亮，对齐 HTML 的 hover
            rects.push([n.x - 1.5, n.y - 1.5, n.w + 3, n.h + 3, paint.radius + 1.5, 'transparent', '#2F6FBA', 2]);
    }

    return JSON.stringify({ w: layout.width, h: layout.height,
                            links: links, rects: rects, texts: texts });
}

function titleBlockSvg(x, y, w, h, info) {
    var c1 = w * 0.52, c2 = w * 0.24;
    var s = '';
    s += '<g stroke="#667085" stroke-width="1" fill="none">';
    s += '<rect x="' + x + '" y="' + y + '" width="' + w + '" height="' + h + '"/>';
    s += '<line x1="' + (x + c1) + '" y1="' + y + '" x2="' + (x + c1) + '" y2="' + (y + h) + '"/>';
    s += '<line x1="' + (x + c1 + c2) + '" y1="' + y + '" x2="' + (x + c1 + c2) + '" y2="' + (y + h) + '"/>';
    s += '<line x1="' + x + '" y1="' + (y + h / 2) + '" x2="' + (x + w) + '" y2="' + (y + h / 2) + '"/>';
    s += '</g>';
    s += '<g font-size="11" fill="#1F2A37" font-family="Microsoft YaHei, PingFang SC, sans-serif">';
    s += '<text x="' + (x + 6) + '" y="' + (y + h * 0.32) + '">图名：' + escXml(info.title) + '</text>';
    s += '<text x="' + (x + c1 + 6) + '" y="' + (y + h * 0.32) + '">工程：' + escXml(info.project || '-') + '</text>';
    s += '<text x="' + (x + c1 + c2 + 6) + '" y="' + (y + h * 0.32) + '">图号：' + escXml(info.drawingNo || '-') + '</text>';
    s += '<text x="' + (x + 6) + '" y="' + (y + h * 0.82) + '">日期：' + escXml(info.date || info.today) + '</text>';
    s += '<text x="' + (x + c1 + 6) + '" y="' + (y + h * 0.82) + '">单位：' + escXml(info.unit || '-') + '</text>';
    s += '<text x="' + (x + c1 + c2 + 6) + '" y="' + (y + h * 0.82) + '">比例：示意</text>';
    s += '</g>';
    return s;
}

function buildExportSvg(layout, plan, labels, title, meta, colorOf, today) {
    var titleH = 36;
    var footH = 64;
    var W = Math.max(layout.width, 720);
    var H = layout.height + titleH + footH;

    var s = '<?xml version="1.0" encoding="UTF-8"?>';
    s += '<svg xmlns="http://www.w3.org/2000/svg" width="' + W + '" height="' + H +
        '" viewBox="0 0 ' + W + ' ' + H + '">';
    s += '<rect x="0" y="0" width="' + W + '" height="' + H + '" fill="#FFFFFF"/>';
    s += '<rect x="' + CFG.margin + '" y="' + CFG.margin + '" width="' + layout.width + '" height="' + layout.height +
        '" fill="#FFFFFF" stroke="#CFD6DF"/>';
    s += '<text x="' + CFG.margin + '" y="22" font-size="14" font-weight="600" fill="#1F2A37" ' +
        'font-family="Microsoft YaHei, PingFang SC, sans-serif">' + escXml(title) + '</text>';
    s += '<g transform="translate(' + CFG.margin + ' ' + CFG.margin + ')">';
    s += buildSvgGraph(layout, plan, labels, colorOf);
    s += '</g>';
    s += titleBlockSvg(W - 520 - CFG.margin, H - footH + 4, 520, footH - 8, {
        title: title, project: meta.project, drawingNo: meta.drawingNo,
        unit: meta.unit, date: meta.date, today: today
    });
    s += '</svg>';
    return s;
}
