// PowerPlan.qml —— 配电干线图（HTML 版 power-plan.html 的 QML 移植）
// 逻辑内核在 PowerPlanCore.js，本文件只管界面与交互。
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import PyArx.PowerPlan 1.0
import "PowerPlanCore.js" as C

Rectangle {
    id: root
    color: "#F3F5F8"

    // ================= 对外（Python 侧）可读写 =================
    property string docTitle: "三级配电干线图"
    property string payloadText: ""              // 与 Python 交换文本（导入内容 / 导出内容 / PNG 目标路径）
    property var meta: ({ project: "", drawingNo: "", unit: "", date: "" })

    // ================= 内部状态 =================
    property var nodeRows: []
    property var fullPlan: null
    property var plan: null
    property var layout: null
    property var collapsed: ({})
    property var checks: ({ crossings: 0, overlaps: 0, warnings: [] })
    property string dir: "h"
    property real tx: 0
    property real ty: 0
    property real k: 1
    property string selected: ""
    property string pulse: ""
    property bool showCable: true
    property var selEdges: ({})
    property string graphSvg: ""
    property string graphOps: ""
    property var editRows: []
    property bool editorOpen: false
    property string hoverCode: ""
    property string fCode: ""
    property bool formVisible: false
    property string bannerText: ""
    property string bannerKind: "error"
    property bool confirmVisible: false
    property string confirmText: ""
    property string confirmCode: ""
    property bool pasteVisible: false

    width: 1100
    height: 760

    // ============================================================
    // 数据装载流水线
    // ============================================================
    function loadRecords(records, opts) {
        try {
            var full = C.buildPlan(records);
            nodeRows = records.map(function (r) {
                return {
                    code: String(r.code).trim(),
                    name: String(r.name || "").trim(),
                    upstream: String(r.upstream || "").trim(),
                    cable: String(r.cable || "").trim(),
                    loadKw: isFinite(parseFloat(r.loadKw)) ? parseFloat(r.loadKw) : 0,
                    length: isFinite(parseFloat(r.length)) ? parseFloat(r.length) : 0,
                    voltage: C.normVoltage(r.voltage),
                    __row: r.__row
                };
            });
            fullPlan = full;
            // 用原始 upstream 为准（buildPlan 可能把无效上游清空）
            nodeRows.forEach(function (n) {
                var pn = full.map[n.code];
                if (pn) n.upstream = pn.upstream;
            });
            if (!opts || !opts.keepCollapse) collapsed = ({});
            selected = "";
            pulse = "";
            formVisible = false;
            rebuild();
            if (editorOpen) fillEditor();
            if (full.warnings.length) showBanner(full.warnings.join("；"), "warn");
            else hideBanner();
            if (!opts || opts.fit !== false) fitView();
            return "ok";
        } catch (err) {
            showBanner(String(err.message), "error");   // 整表拒绝，保留上一次的图纸
            return "err:" + err.message;
        }
    }

    function computeHidden() {
        var hidden = ({});
        if (!fullPlan) return hidden;
        for (var code in collapsed) {
            if (!collapsed[code]) continue;
            var node = fullPlan.map[code];
            if (!node) continue;
            var stack = node.children.slice();
            while (stack.length) {
                var n = stack.pop();
                if (hidden[n.code]) continue;
                hidden[n.code] = 1;
                for (var i = 0; i < n.children.length; i++) stack.push(n.children[i]);
            }
        }
        return hidden;
    }

    function rebuild() {
        var hidden = computeHidden();
        var visible = nodeRows.filter(function (n) { return !hidden[n.code]; });
        var p;
        try {
            p = C.buildPlan(visible);
        } catch (e) {
            showBanner(String(e.message), "error");
            return false;
        }
        // 折叠标记：自身还有被隐藏的下游
        for (var i = 0; i < p.nodes.length; i++) {
            var n = p.nodes[i];
            var fn2 = fullPlan.map[n.code];
            n.collapsedCount = (fn2 && n.children.length === 0) ? fn2.children.length : 0;
        }
        var L = C.layoutPlan(p, dir);
        var crossings = C.countCrossings(L.links, C.CFG.curveSteps);
        var overlaps = C.countOverlaps(L.nodes);

        plan = p;
        layout = L;
        checks = { crossings: crossings, overlaps: overlaps, warnings: p.warnings.slice() };
        applySelection();
        refreshSvg();
        drawMinimap();
        updateStatus();
        return true;
    }

    // ============================================================
    // 绘制
    // ============================================================
    function setFont(ctx, size, weight) {
        ctx.font = (weight >= 600 ? "600 " : "") + size + "px \"Microsoft YaHei\"";
    }

    function paintGraph(ctx, ss) {
        if (!ctx) return;
        ss = ss || 1;
        ctx.setTransform(ss, 0, 0, ss, 0, 0);
        ctx.clearRect(0, 0, graph.width, graph.height);
        ctx.fillStyle = "#F3F5F8";
        ctx.fillRect(0, 0, graph.width, graph.height);
        if (!layout) return;

        ctx.save();
        ctx.translate(tx, ty);
        ctx.scale(k, k);

        // 图纸底
        ctx.fillStyle = "#FFFFFF";
        ctx.fillRect(0, 0, layout.width, layout.height);
        ctx.strokeStyle = "#CFD6DF";
        ctx.lineWidth = 1;
        ctx.strokeRect(0.5, 0.5, layout.width - 1, layout.height - 1);

        // 连线
        for (var i = 0; i < layout.links.length; i++) {
            var l = layout.links[i];
            var key = l.from + ">" + l.to;
            var hl = !!selEdges[key];
            ctx.beginPath();
            ctx.moveTo(l.x1, l.y1);
            ctx.bezierCurveTo(l.bezier[2], l.bezier[3], l.bezier[4], l.bezier[5], l.x2, l.y2);
            ctx.strokeStyle = hl ? "#E15759" : "#2F6FBA";
            ctx.lineWidth = hl ? 2.6 : 2;
            ctx.lineCap = "round";
            ctx.stroke();

            if (showCable && l.cable) {
                ctx.fillStyle = "#1F64B0";
                setFont(ctx, 10, 400);
                if (layout.dir === "h") {
                    ctx.textAlign = "center";
                    ctx.fillText(l.cable, (l.x1 + l.x2) / 2, (l.y1 + l.y2) / 2 - 5);
                } else {
                    ctx.textAlign = "left";
                    ctx.fillText(l.cable, (l.x1 + l.x2) / 2 + 6, (l.y1 + l.y2) / 2 + 3);
                }
            }
        }

        // 节点
        for (var ni = 0; ni < layout.nodes.length; ni++) {
            var n = layout.nodes[ni];
            var pn = plan.map[n.code];
            var paint = C.nodePaint(n, pn);
            var isSel = (selected === n.code);
            var isPulse = (pulse === n.code);

            ctx.fillStyle = paint.fill;
            ctx.fillRect(n.x, n.y, n.w, n.h);
            ctx.strokeStyle = isPulse ? "#E15759" : (isSel ? "#E15759" : paint.stroke);
            ctx.lineWidth = isPulse ? 5 : (isSel ? 2.2 : paint.lineWidth);
            ctx.strokeRect(n.x, n.y, n.w, n.h);

            // 电压角标
            if (pn && pn.voltage) {
                var vs = paint.voltageStyle;
                var bw = 8 + C.textUnits(vs.text) * 4.6;
                var bh = 11;
                var bx = n.x + n.w - bw - 4, by = n.y + 3;
                ctx.fillStyle = vs.bg;
                ctx.fillRect(bx, by, bw, bh);
                ctx.strokeStyle = vs.color;
                ctx.lineWidth = 0.7;
                ctx.strokeRect(bx, by, bw, bh);
                ctx.fillStyle = vs.color;
                setFont(ctx, 8.5, 600);
                ctx.textAlign = "center";
                ctx.fillText(vs.text, bx + bw / 2, by + 8.6);
            }

            // 文本行
            var lines = paint.lines;
            var blockH = 16 + 13 * (lines.length - 1);
            var ty2 = n.y + (n.h - blockH) / 2 + 12;
            var cx = n.x + n.w / 2;
            ctx.textAlign = "center";
            for (var li = 0; li < lines.length; li++) {
                var ln = lines[li];
                ctx.fillStyle = ln.cls === "t-code" ? "#1F2A37" : (ln.cls === "t-name" ? "#5B6675" : "#1F64B0");
                setFont(ctx, ln.size, ln.weight);
                ctx.fillText(ln.t, cx, ty2);
                ty2 += 13;
            }

            if (paint.collapsedCount) {
                ctx.fillStyle = "#8A94A6";
                setFont(ctx, 9, 400);
                ctx.textAlign = "right";
                ctx.fillText("+ " + paint.collapsedCount, n.x + n.w - 6, n.y + 12);
            }
        }

        ctx.restore();
    }

    // 图元是矢量 SVG（SvgGraph），平移/缩放由 offset/zoom 属性驱动，这里只需在
    // 布局、选中态、连线标注变化时重建一次 SVG 文本。
    // 只用绘制指令（C++ 侧 QPainter 直绘）；SVG 文本仅在导出时由 buildExportSvg 生成
    function refreshSvg() {
        if (!layout || !plan) { graphOps = ""; graphSvg = ""; return; }
        var colorOf = function (key) { return selEdges[key] ? "#E15759" : "#2F6FBA"; };
        graphOps = C.buildDrawOps(layout, plan, showCable, colorOf, hoverCode);
        graphSvg = "";
    }

    function repaint() { }   // 交给 SvgGraph 的 offset / zoom 绑定自动重绘

    function drawMinimap() {
        mini.requestPaint();
    }

    function paintMinimap(ctx, ss) {
        if (!ctx) return;
        ss = ss || 1;
        var W = mini.width, H = mini.height;
        ctx.setTransform(ss, 0, 0, ss, 0, 0);
        ctx.clearRect(0, 0, W, H);
        ctx.fillStyle = "#FBFCFE";
        ctx.fillRect(0, 0, W, H);
        ctx.strokeStyle = "#E4E7EC";
        ctx.lineWidth = 1;
        ctx.strokeRect(0.5, 0.5, W - 1, H - 1);
        if (!layout) return;
        var pad = 4;
        var s = Math.min((W - pad * 2) / layout.width, (H - pad * 2) / layout.height);
        var ox = (W - layout.width * s) / 2, oy = (H - layout.height * s) / 2;

        ctx.fillStyle = "#CFE0F3";
        for (var i = 0; i < layout.nodes.length; i++) {
            var n = layout.nodes[i];
            ctx.fillRect(ox + n.x * s, oy + n.y * s, Math.max(1, n.w * s), Math.max(1, n.h * s));
        }
        var vx = -tx / k, vy = -ty / k, vw = graph.width / k, vh = graph.height / k;
        var rx0 = Math.max(1, ox + vx * s), ry0 = Math.max(1, oy + vy * s);
        var rx1 = Math.min(W - 1, ox + (vx + vw) * s), ry1 = Math.min(H - 1, oy + (vy + vh) * s);
        if (rx1 > rx0 && ry1 > ry0) {
            ctx.strokeStyle = "#E15759";
            ctx.lineWidth = 1;
            ctx.strokeRect(rx0, ry0, rx1 - rx0, ry1 - ry0);
        }
    }

    // ============================================================
    // 选择与折叠
    // ============================================================
    function applySelection() {
        var edges = ({});
        if (selected && plan && plan.map[selected]) {
            var map = plan.map;
            var start = map[selected];
            var upChain = {};
            upChain[selected] = 1;
            var cur = start;
            while (cur.parent) { upChain[cur.parent.code] = 1; cur = cur.parent; }

            var subTree = {};
            var stack = [start];
            while (stack.length) {
                var n = stack.pop();
                subTree[n.code] = 1;
                for (var i = 0; i < n.children.length; i++) stack.push(n.children[i]);
            }
            for (var code in upChain) {
                var un = map[code];
                if (un && un.parent) edges[un.parent.code + ">" + un.code] = 1;
            }
            for (var code2 in subTree) {
                var sn = map[code2];
                if (!sn) continue;
                for (var j = 0; j < sn.children.length; j++)
                    edges[sn.code + ">" + sn.children[j].code] = 1;
            }
        }
        selEdges = edges;
        refreshSvg();
    }

    function nodeAt(px, py) {
        if (!layout) return "";
        var wx = (px - tx) / k, wy = (py - ty) / k;
        for (var i = 0; i < layout.nodes.length; i++) {
            var n = layout.nodes[i];
            if (wx >= n.x && wx <= n.x + n.w && wy >= n.y && wy <= n.y + n.h) return n.code;
        }
        return "";
    }

    function selectNode(code) {
        if (!plan || !plan.map[code]) return;
        selected = code;
        pulse = "";
        applySelection();
        fillForm(code);
    }

    function clearSelection() {
        if (!selected) return;
        selected = "";
        formVisible = false;
        applySelection();
    }

    function toggleCollapse(code) {
        var c = ({});
        for (var key in collapsed) c[key] = collapsed[key];
        if (c[code]) delete c[code];
        else c[code] = 1;
        collapsed = c;
        if (selected === code) { selected = ""; formVisible = false; }
        rebuild();
    }

    function centerOn(code) {
        if (!layout) return;
        var n = null;
        for (var i = 0; i < layout.nodes.length; i++)
            if (layout.nodes[i].code === code) n = layout.nodes[i];
        if (!n) return;
        var area = viewArea();
        tx = area.w / 2 - (n.x + n.w / 2) * k;
        ty = area.h / 2 - (n.y + n.h / 2) * k;
        repaint();
        drawMinimap();
        updateStatus();
    }

    // ============================================================
    // 视图变换
    // ============================================================
    function viewArea() {
        var used = editorOpen ? 640 : 0;
        return { w: Math.max(200, graph.width - used), h: graph.height };
    }

    function clampK(kk) { return Math.min(4, Math.max(0.2, kk)); }

    function zoomAt(px, py, factor) {
        var kk = clampK(k * factor);
        if (kk === k) return;
        tx = px - (px - tx) * (kk / k);
        ty = py - (py - ty) * (kk / k);
        k = kk;
        repaint();
        drawMinimap();
        updateStatus();
    }

    function fitView() {
        if (!layout) return;
        var area = viewArea();
        var pad = 0.92;
        k = clampK(Math.min(area.w / layout.width, area.h / layout.height) * pad);
        tx = (area.w - layout.width * k) / 2;
        ty = (area.h - layout.height * k) / 2;
        repaint();
        drawMinimap();
        updateStatus();
    }

    function statusLine() {
        var L = layout;
        var v380 = 0, v220 = 0, vMv = 0, vHv = 0;
        for (var i = 0; i < nodeRows.length; i++) {
            var cl = C.voltClass(C.normVoltage(nodeRows[i].voltage));
            if (cl === "lv220") v220++;
            else if (cl === "mv") vMv++;
            else if (cl === "hv") vHv++;
            else v380++;
        }
        var zoomTxt = Math.round(k * 100) + "%";
        var hint = k < 0.55 ? " · 图较宽，滚轮放大看细节，双击电柜可折叠" : "";
        if (!L) return "拖动平移 · 滚轮缩放";
        return "节点 " + L.nodes.length + " · 连线 " + L.links.length +
            " · 交叉 " + checks.crossings + " · 重叠 " + checks.overlaps +
            " · 380V " + v380 + " / 220V " + v220 +
            (vMv ? " / 中压 " + vMv : "") + (vHv ? " / 高压 " + vHv : "") +
            " · " + zoomTxt + hint + "  |  拖动平移 · 滚轮缩放 · 双击折叠";
    }

    function updateStatus() { statusText.text = statusLine(); }

    // ============================================================
    // 提示条
    // ============================================================
    function showBanner(msg, kind) {
        bannerText = msg;
        bannerKind = (kind === "warn" ? "warn" : "error");
        bannerTimer.interval = (kind === "warn" ? 6000 : 5000);
        bannerTimer.restart();
    }
    function hideBanner() { bannerTimer.stop(); bannerText = ""; }

    Timer {
        id: bannerTimer
        repeat: false
        onTriggered: root.bannerText = ""
    }

    // ============================================================
    // 搜索
    // ============================================================
    function doSearch(kw) {
        kw = String(kw || "").trim().toLowerCase();
        if (!plan) return;
        if (!kw) { pulse = ""; applySelection(); return; }
        var hit = null;
        for (var i = 0; i < nodeRows.length; i++) {
            var n = nodeRows[i];
            if (n.code.toLowerCase().indexOf(kw) >= 0 ||
                    (n.name && n.name.toLowerCase().indexOf(kw) >= 0)) { hit = n; break; }
        }
        if (!hit) { pulse = ""; applySelection(); return; }
        var visibleHit = false;
        for (var j = 0; j < layout.nodes.length; j++)
            if (layout.nodes[j].code === hit.code) visibleHit = true;
        if (!visibleHit) {
            // 被折叠隐藏 → 先展开其祖先的折叠
            var c = ({});
            for (var key in collapsed) c[key] = collapsed[key];
            for (var code in collapsed) {
                var fn2 = fullPlan.map[code];
                if (!fn2) continue;
                var stack = fn2.children.slice();
                var found = false;
                while (stack.length) {
                    var q = stack.pop();
                    if (q.code === hit.code) { found = true; break; }
                    for (var m = 0; m < q.children.length; m++) stack.push(q.children[m]);
                }
                if (found) delete c[code];
            }
            collapsed = c;
            rebuild();
        }
        pulse = hit.code;
        applySelection();
        repaint();
        centerOn(hit.code);
    }

    // ============================================================
    // 节点表单（右侧详情）
    // ============================================================
    function fillForm(code) {
        var n = plan.map[code];
        if (!n) return;
        fCode = n.code;
        neCode.text = n.code;
        neName.text = n.name || "";
        neUp.text = n.upstream || "";
        neCable.text = n.cable || "";
        neLen.text = (n.length > 0 ? String(n.length) : "");
        neLoad.text = (n.loadKw > 0 ? String(n.loadKw) : "");
        neVolt.options = C.voltageOptions(C.normVoltage(n.voltage));
        neVolt.value = C.normVoltage(n.voltage);
        var downCount = n.collapsedCount || n.children.length;
        neTip.text = "第 " + (n.depth + 1) + " 层 · 下游 " + downCount + " 个" +
            (n.collapsedCount ? "（已折叠）" : "");
        formVisible = true;
    }

    function saveNodeEdit(oldCode) {
        var code = neCode.text.trim();
        var name = neName.text.trim();
        var upstream = neUp.text.trim();
        var cable = neCable.text.trim();
        var voltage = C.normVoltage(neVolt.value);
        var rawLen = neLen.text.trim();
        var length = rawLen ? parseFloat(rawLen) : 0;
        var rawLoad = neLoad.text.trim();
        var loadKw = rawLoad ? parseFloat(rawLoad) : 0;

        if (!code) { showBanner("编号不能为空", "error"); return; }
        if (rawLoad && (!isFinite(loadKw) || loadKw < 0)) { showBanner("负载必须是不小于 0 的数字", "error"); return; }
        if (rawLen && (!isFinite(length) || length < 0)) { showBanner("长度必须是不小于 0 的数字", "error"); return; }
        if (upstream && upstream === code) { showBanner("上游不能是节点自己", "error"); return; }

        var rows = nodeRows.map(function (r) {
            return { code: r.code, name: r.name, upstream: r.upstream, cable: r.cable,
                loadKw: r.loadKw, voltage: r.voltage, length: r.length };
        });
        var idx = -1;
        for (var i = 0; i < rows.length; i++) if (rows[i].code === oldCode) idx = i;
        if (idx < 0) return;
        if (code !== oldCode) {
            var dup = false;
            for (var j = 0; j < rows.length; j++) if (rows[j].code === code) dup = true;
            if (dup) { showBanner("编号重复：" + code, "error"); return; }
            rows.forEach(function (r) { if (r.upstream === oldCode) r.upstream = code; });
        }
        rows[idx] = { code: code, name: name, upstream: upstream, cable: cable,
            loadKw: loadKw, voltage: voltage, length: length };

        loadRecords(rows, { fit: false, keepCollapse: true });
        if (plan && plan.map[code]) selectNode(code);
    }

    function nextChildCode(parentCode) {
        var codes = ({});
        for (var i = 0; i < nodeRows.length; i++) codes[nodeRows[i].code] = 1;
        var i2 = 1;
        while (codes[parentCode + "-" + i2]) i2++;
        return parentCode + "-" + i2;
    }

    function addChildNode(parentCode) {
        var code = nextChildCode(parentCode);
        var parent = plan ? plan.map[parentCode] : null;
        var rows = nodeRows.map(function (r) {
            return { code: r.code, name: r.name, upstream: r.upstream, cable: r.cable,
                loadKw: r.loadKw, voltage: r.voltage, length: r.length };
        });
        rows.push({ code: code, name: "", upstream: parentCode, cable: "",
            loadKw: 0, voltage: parent ? parent.voltage : "380", length: 0 });
        loadRecords(rows, { fit: false, keepCollapse: true });
        if (plan && plan.map[code]) selectNode(code);
    }

    function askDelete(code) {
        if (!plan || !plan.map[code]) return;
        var n = plan.map[code];
        var kids = n.children.length;
        confirmCode = code;
        confirmText = kids
            ? ("删除节点 " + code + "？它的 " + kids + " 个下游会挂到它的上游。")
            : ("删除节点 " + code + "？");
        confirmVisible = true;
    }

    function doDelete(code) {
        var n = plan.map[code];
        if (!n) return;
        var rows = nodeRows.filter(function (r) { return r.code !== code; })
            .map(function (r) {
                var up = r.upstream;
                if (up === code) up = n.upstream;
                return { code: r.code, name: r.name, upstream: up, cable: r.cable,
                    loadKw: r.loadKw, voltage: r.voltage, length: r.length };
            });
        var c = ({});
        for (var key in collapsed) if (key !== code) c[key] = collapsed[key];
        collapsed = c;
        loadRecords(rows, { fit: false, keepCollapse: true });
        if (n.upstream && plan && plan.map[n.upstream]) selectNode(n.upstream);
        else clearSelection();
    }

    // ============================================================
    // 节点表格（右侧抽屉）
    // ============================================================
    function setCell(rowIndex, key, value) {
        if (rowIndex < 0 || rowIndex >= editRows.length) return;
        var row = editRows[rowIndex];
        row[key] = value;
    }

    function fillEditor() {
        editRows = nodeRows.map(function (n) {
            return { code: n.code, name: n.name, upstream: n.upstream, cable: n.cable,
                loadKw: (n.loadKw ? String(n.loadKw) : ""), voltage: C.normVoltage(n.voltage),
                length: (n.length ? String(n.length) : "") };
        });
    }

    function suggestCode() {
        var used = ({});
        for (var i = 0; i < editRows.length; i++) used[editRows[i].code] = 1;
        var i2 = 1;
        while (used["N" + i2]) i2++;
        return "N" + i2;
    }

    function editorAdd() {
        var rows = editRows.slice();
        rows.push({ code: suggestCode(), name: "", upstream: "", cable: "", loadKw: "",
            voltage: "380", length: "" });
        editRows = rows;
    }

    function editorRemove(rowIndex) {
        var rows = [];
        for (var i = 0; i < editRows.length; i++) if (i !== rowIndex) rows.push(editRows[i]);
        editRows = rows;
    }

    function editorApply() {
        var rows = [];
        for (var i = 0; i < editRows.length; i++) {
            var r = editRows[i];
            if (!r.code && !r.name && !r.upstream && !r.cable && !r.loadKw && !r.length) continue;
            rows.push({ code: String(r.code || ""), name: String(r.name || ""),
                upstream: String(r.upstream || ""), cable: String(r.cable || ""),
                loadKw: String(r.loadKw || ""), voltage: String(r.voltage || "380"),
                length: String(r.length || "") });
        }
        if (!rows.length) { showBanner("表格里还没有节点", "error"); return "err:表格里还没有节点"; }
        var wasOpen = editorOpen;
        var ret = loadRecords(rows, { fit: false });
        if (wasOpen) fillEditor();
        return ret;
    }

    // ============================================================
    // 导入 / 导出（文件读写交给 Python，文本经 payloadText 传递）
    // ============================================================
    function loadCsvText(text) {
        try {
            return loadRecords(C.parseCSV(text));
        } catch (e) {
            showBanner(String(e.message), "error");
            return "err:" + e.message;
        }
    }

    function loadJsonText(text) {
        try {
            var obj = JSON.parse(text);
            var arr = (obj instanceof Array) ? obj : (obj.nodes || []);
            return loadRecords(arr);
        } catch (e) {
            showBanner("JSON 解析失败：" + e.message, "error");
            return "err:" + e.message;
        }
    }

    function loadSample() {
        docTitleField.text = "1号配电室 配电干线图";
        docTitle = docTitleField.text;
        return loadCsvText(C.SAMPLE_CSV);
    }

    function loadThreeLevel() {
        docTitleField.text = "三级配电干线图";
        docTitle = docTitleField.text;
        return loadCsvText(C.buildThreeLevelCsv());
    }

    function exportCsvText() {
        if (!nodeRows.length) return "";
        return C.toCSV(nodeRows);
    }

    function exportJsonText() {
        if (!layout) return "";
        var nodes = nodeRows.map(function (n) {
            return { code: n.code, name: n.name, upstream: n.upstream, cable: n.cable,
                loadKw: n.loadKw, voltage: n.voltage, length: n.length };
        });
        var ln = [];
        for (var i = 0; i < layout.nodes.length; i++) {
            var n2 = layout.nodes[i];
            ln.push({ code: n2.code, x: Math.round(n2.x * 100) / 100, y: Math.round(n2.y * 100) / 100,
                w: n2.w, h: n2.h, depth: n2.depth });
        }
        var lk = [];
        for (var j = 0; j < layout.links.length; j++) {
            var l = layout.links[j];
            lk.push({ from: l.from, to: l.to, x1: l.x1, y1: l.y1, x2: l.x2, y2: l.y2 });
        }
        return JSON.stringify({
            title: docTitle,
            nodes: nodes,
            layout: {
                width: Math.round(layout.width * 100) / 100,
                height: Math.round(layout.height * 100) / 100,
                nodes: ln, links: lk
            },
            checks: { crossings: checks.crossings, overlaps: checks.overlaps,
                warnings: checks.warnings }
        }, null, 2);
    }

    function exportSvgText() {
        if (!layout) return "";
        var colorOf = function (key) { return selEdges[key] ? "#E15759" : "#2F6FBA"; };
        var today = Qt.formatDate(new Date(), "yyyy/M/d");
        return C.buildExportSvg(layout, plan, showCable, docTitle, meta, colorOf, today);
    }

    function exportPngTo(path) {
        payloadText = path;
        graph.grabToImage(function (result) {
            var ok = result.saveToFile(path);
            showBanner(ok ? ("已导出 PNG：" + path) : ("PNG 保存失败：" + path),
                ok ? "warn" : "error");
        }, Qt.size(Math.round(graph.width * 2), Math.round(graph.height * 2)));
    }

    // 供 Python 判断能力
    function info() {
        return JSON.stringify({
            nodes: nodeRows.length,
            visible: layout ? layout.nodes.length : 0,
            links: layout ? layout.links.length : 0,
            crossings: checks.crossings,
            overlaps: checks.overlaps,
            dir: dir,
            zoom: Math.round(k * 100)
        });
    }

    // ============================================================
    // 界面
    // ============================================================
    // ---- 画布 ----
    Item {
        id: graphArea
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: toolbar.bottom
        anchors.bottom: statusBar.top
        clip: true

        // 图纸底（跟随平移缩放）
        Rectangle {
            id: paper
            x: root.tx
            y: root.ty
            width: root.layout ? root.layout.width * root.k : 0
            height: root.layout ? root.layout.height * root.k : 0
            color: "#FFFFFF"
            border.width: 1
            border.color: "#CFD6DF"
            visible: root.layout !== null
        }

        SvgGraph {
            id: graph
            objectName: "graph"
            anchors.fill: parent
            ops: root.graphOps
            offset: Qt.point(root.tx, root.ty)
            zoom: root.k
        }

        MouseArea {
            id: area
            anchors.fill: parent
            hoverEnabled: true
            property real pressX: 0
            property real pressY: 0
            property real startTx: 0
            property real startTy: 0
            property bool moved: false
            property bool dragging: false

            onPressed: function (m) {
                pressX = m.x; pressY = m.y;
                startTx = root.tx; startTy = root.ty;
                moved = false;
                dragging = (root.nodeAt(m.x, m.y) === "");
            }
            onPositionChanged: function (m) {
                if (!pressed) {                       // 未拖动时更新悬停高亮
                    var hc = root.nodeAt(m.x, m.y);
                    if (hc !== root.hoverCode) { root.hoverCode = hc; root.refreshSvg(); }
                }
                if (!pressed || !dragging) return;
                var dx = m.x - pressX, dy = m.y - pressY;
                if (Math.abs(dx) > 3 || Math.abs(dy) > 3) moved = true;
                if (moved) {
                    root.tx = startTx + dx;
                    root.ty = startTy + dy;
                    root.repaint();
                    root.drawMinimap();
                    root.updateStatus();
                }
            }
            onReleased: {
                if (!dragging && !moved) {
                    var code = root.nodeAt(pressX, pressY);
                    if (code) root.selectNode(code);
                    else root.clearSelection();
                }
                dragging = false;
                moved = false;
            }
            onDoubleClicked: function (m) {
                var code = root.nodeAt(m.x, m.y);
                if (code) root.toggleCollapse(code);
            }
            onWheel: function (w) {
                root.zoomAt(w.x, w.y, w.angleDelta.y > 0 ? 1.12 : 1 / 1.12);
            }
            onExited: {
                if (root.hoverCode !== "") { root.hoverCode = ""; root.refreshSvg(); }
            }
            cursorShape: root.hoverCode !== "" ? Qt.PointingHandCursor
                         : (dragging ? Qt.ClosedHandCursor : Qt.OpenHandCursor)
        }
    }

    // ---- 工具栏 ----
    Rectangle {
        id: toolbar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: toolbarFlow.y + toolbarFlow.height + 8
        color: "#FFFFFF"
        border.width: 0

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: "#D0D5DD"
        }

        Flow {
            id: toolbarFlow
            x: 8
            y: 8
            width: parent.width - 16
            spacing: 8

            PPText {
                id: docTitleField
            objectName: "docTitleField"
                width: 150
                text: root.docTitle
                font.pixelSize: 15
                font.bold: true
                onTextEdited: root.docTitle = text.trim() || "配电干线图"
                background: Rectangle {
                    radius: 3
                    border.width: 1
                    border.color: docTitleField.activeFocus ? "#A9C4E4" : "transparent"
                    color: docTitleField.activeFocus ? "#F8FBFF" : "transparent"
                }
            }

            PPButton { text: "文档示例"; primary: true; onClicked: root.loadSample() }
            PPButton { text: "三级配电示例"; primary: true; onClicked: root.loadThreeLevel() }
            PPButton {
                text: "节点编辑"
                onClicked: {
                    root.editorOpen = !root.editorOpen;
                    if (root.editorOpen) root.fillEditor();
                }
            }
            PPButton { text: "导入 CSV"; onClicked: io("importCsv") }
            PPButton { text: "导入 XLSX"; onClicked: io("importXlsx") }
            PPButton { text: "导入 JSON"; onClicked: io("importJson") }
            PPButton { text: "粘贴 JSON"; onClicked: root.pasteVisible = true }

            PPButton { text: "导出 JSON"; onClicked: save("saveJson") }
            PPButton { text: "导出 CSV"; onClicked: save("saveCsv") }
            PPButton { text: "导出 SVG"; onClicked: save("saveSvg") }
            PPButton {
                text: "导出 PNG"
                onClicked: {
                    var r = backendCall("io:savePng");
                    if (r === "ok") root.exportPngTo(root.payloadText);
                }
            }

            PPButton { text: "－"; onClicked: root.zoomAt(graphArea.width / 2, graphArea.height / 2, 1 / 1.25) }
            PPButton {
                text: Math.round(root.k * 100) + "%"
                onClicked: root.zoomAt(graphArea.width / 2, graphArea.height / 2, 1 / root.k)
            }
            PPButton { text: "＋"; onClicked: root.zoomAt(graphArea.width / 2, graphArea.height / 2, 1.25) }
            PPButton { text: "适应窗口"; onClicked: root.fitView() }

            Row {
                spacing: 4
                height: 28
                CheckBox {
                    id: cableChk
                    checked: root.showCable
                    text: "连线标注电缆"
                    font.pixelSize: 13
                    font.family: "Microsoft YaHei"
                    padding: 0
                    topPadding: 4
                    bottomPadding: 4
                    onCheckedChanged: {
                        root.showCable = checked;
                        root.repaint();
                    }
                }
            }

            Row {
                spacing: 4
                height: 28
                Text {
                    text: "布局"
                    font.pixelSize: 13
                    font.family: "Microsoft YaHei"
                    color: "#475467"
                    anchors.verticalCenter: parent.verticalCenter
                }
                PPCombo {
                    id: dirCombo
                    width: 104
                    options: [{ value: "h", label: "横向 左→右" }, { value: "v", label: "纵向 上→下" }]
                    value: root.dir
                    onValueChanged: {
                        if (root.dir !== value) {
                            root.dir = value;
                            root.rebuild();
                            root.fitView();
                        }
                    }
                }
            }

            PPText {
                id: searchField
                objectName: "searchField"
                width: 170
                hint: "搜索编号 / 名称"
                onTextEdited: root.doSearch(text)
            }
        }
    }

    // ---- 状态条 ----
    Rectangle {
        id: statusBar
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        width: Math.min(parent.width - 210, statusText.implicitWidth + 24)
        height: 24
        color: "#FFFFFFEE"
        border.width: 1
        border.color: "#E4E7EC"
        radius: 4
        anchors.margins: 10

        Text {
            id: statusText
            objectName: "statusText"
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: 12
            font.family: "Microsoft YaHei"
            color: "#475467"
            elide: Text.ElideRight
            text: "拖动平移 · 滚轮缩放"
        }
    }

    // ---- 小地图 ----
    Rectangle {
        id: miniBox
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 10
        anchors.bottomMargin: 10
        width: 188
        height: 134
        color: "#FFFFFFF2"
        border.width: 1
        border.color: "#D0D5DD"
        radius: 4

        Canvas {
            id: mini
            width: 180
            height: 126
            anchors.centerIn: parent
            antialiasing: true
            property real ss: Screen.devicePixelRatio
            canvasSize: Qt.size(Math.round(width * ss), Math.round(height * ss))
            onPaint: root.paintMinimap(getContext("2d"), ss)

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.CrossCursor
                onClicked: {
                    if (!root.layout) return;
                    var L = root.layout, pad = 4;
                    var s = Math.min((mini.width - pad * 2) / L.width, (mini.height - pad * 2) / L.height);
                    var ox = (mini.width - L.width * s) / 2, oy = (mini.height - L.height * s) / 2;
                    var wx = (mouse.x - ox) / s, wy = (mouse.y - oy) / s;
                    root.tx = root.viewArea().w / 2 - wx * root.k;
                    root.ty = graphArea.height / 2 - wy * root.k;
                    root.repaint();
                    root.drawMinimap();
                }
            }
        }
    }

    // ---- 提示条 ----
    Rectangle {
        id: banner
        visible: root.bannerText !== ""
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: toolbar.bottom
        anchors.topMargin: 10
        width: Math.min(parent.width * 0.7, bannerLabel.implicitWidth + 28)
        height: bannerLabel.implicitHeight + 16
        radius: 5
        color: root.bannerKind === "warn" ? "#FFFAEB" : "#FEF3F2"
        border.width: 1
        border.color: root.bannerKind === "warn" ? "#FEC84B" : "#FDA29B"
        z: 30

        Text {
            id: bannerLabel
            anchors.centerIn: parent
            width: parent.width - 28
            wrapMode: Text.WordWrap
            font.pixelSize: 13
            font.family: "Microsoft YaHei"
            color: root.bannerKind === "warn" ? "#7A4F01" : "#912018"
            text: root.bannerText
        }
    }

    // ---- 节点表单 ----
    Rectangle {
        id: formPanel
        visible: root.formVisible
        width: 272
        anchors.top: toolbar.bottom
        anchors.topMargin: 12
        anchors.right: parent.right
        anchors.rightMargin: root.editorOpen ? 652 : 12
        height: formCol.height + 24
        color: "#FFFFFF"
        border.width: 1
        border.color: "#D0D5DD"
        radius: 6
        z: 20

        Column {
            id: formCol
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 12
            spacing: 6

            Text {
                text: "节点 " + root.fCode
                font.pixelSize: 14
                font.bold: true
                font.family: "Microsoft YaHei"
                color: "#1F2A37"
            }

            Text { text: "编号"; font.pixelSize: 12; color: "#667085" }
            PPText { id: neCode; objectName: "neCode"; width: parent.width }
            Text { text: "名称"; font.pixelSize: 12; color: "#667085" }
            PPText { id: neName; objectName: "neName"; width: parent.width; hint: "可空" }
            Text { text: "上游"; font.pixelSize: 12; color: "#667085" }
            PPText { id: neUp; objectName: "neUp"; width: parent.width; hint: "留空=起点" }
            Text { text: "电缆型号"; font.pixelSize: 12; color: "#667085" }
            PPText { id: neCable; objectName: "neCable"; width: parent.width; hint: "进入本节点的电缆" }
            Text { text: "电压"; font.pixelSize: 12; color: "#667085" }
            PPCombo { id: neVolt; objectName: "neVolt"; width: parent.width }
            Row {
                spacing: 8
                width: parent.width
                Column {
                    width: (parent.width - 8) / 2
                    spacing: 2
                    Text { text: "长度 m"; font.pixelSize: 12; color: "#667085" }
                    PPText { id: neLen; width: parent.width; hint: "0" }
                }
                Column {
                    width: (parent.width - 8) / 2
                    spacing: 2
                    Text { text: "负载 kW"; font.pixelSize: 12; color: "#667085" }
                    PPText { id: neLoad; width: parent.width; hint: "0" }
                }
            }
            Row {
                spacing: 4
                PPButton { text: "保存并重绘"; primary: true; onClicked: root.saveNodeEdit(root.fCode) }
                PPButton { text: "＋下游节点"; onClicked: root.addChildNode(root.fCode) }
                PPButton { text: "删除"; onClicked: root.askDelete(root.fCode) }
            }
            Text {
                id: neTip
                width: parent.width
                wrapMode: Text.WordWrap
                font.pixelSize: 11
                color: "#8A94A6"
            }
        }
    }

    // ---- 节点表格（右侧抽屉）----
    Rectangle {
        id: editor
        objectName: "editor"
        width: 640
        anchors.top: toolbar.bottom
        anchors.bottom: statusBar.top
        x: root.editorOpen ? (parent.width - width) : parent.width
        color: "#FFFFFF"
        border.width: 1
        border.color: "#D0D5DD"
        z: 40
        Behavior on x { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }

        Column {
            anchors.fill: parent
            spacing: 0

            Rectangle {
                id: edHead
                width: parent.width
                height: 42
                color: "#FFFFFF"

                Row {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 6
                    Text {
                        text: "节点数据  共 " + root.editRows.length + " 行"
                        font.pixelSize: 13
                        font.bold: true
                        font.family: "Microsoft YaHei"
                        color: "#1F2A37"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Item { width: 12; height: 1 }
                    PPButton { text: "＋ 新增节点"; onClicked: root.editorAdd() }
                    PPButton { text: "应用并重绘"; primary: true; onClicked: root.editorApply() }
                    PPButton { text: "关闭"; onClicked: root.editorOpen = false }
                }
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: 1
                    color: "#E4E7EC"
                }
            }

            // 表头
            Row {
                width: parent.width
                height: 26
                anchors.left: parent.left
                anchors.leftMargin: 6
                spacing: 2
                Repeater {
                    model: [
                        { t: "编号", w: 78 }, { t: "名称", w: 100 }, { t: "上游", w: 70 },
                        { t: "电缆型号", w: 130 }, { t: "负载kW", w: 52 }, { t: "电压", w: 100 },
                        { t: "长度m", w: 56 }, { t: "", w: 26 }
                    ]
                    Text {
                        width: modelData.w
                        height: 26
                        text: modelData.t
                        font.pixelSize: 12
                        font.bold: true
                        font.family: "Microsoft YaHei"
                        color: "#475467"
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
            Rectangle { width: parent.width; height: 1; color: "#E4E7EC" }

            Flickable {
                width: parent.width
                height: parent.height - edHead.height - 27 - edFoot.height
                clip: true
                contentHeight: rowsCol.height
                boundsBehavior: Flickable.StopAtBounds

                Column {
                    id: rowsCol
                    width: parent.width
                    anchors.left: parent.left
                    anchors.leftMargin: 6

                    Repeater {
                        model: root.editRows.length

                        Row {
                            id: edRow
                            property int rowIndex: index
                            property var rowData: root.editRows[rowIndex] ? root.editRows[rowIndex] : ({})
                            width: rowsCol.width
                            height: 28
                            spacing: 2

                            function sync() {
                                fCode.text = (rowData.code !== undefined) ? String(rowData.code) : "";
                                fName.text = (rowData.name !== undefined) ? String(rowData.name) : "";
                                fUp.text = (rowData.upstream !== undefined) ? String(rowData.upstream) : "";
                                fCable.text = (rowData.cable !== undefined) ? String(rowData.cable) : "";
                                fLoad.text = (rowData.loadKw !== undefined && rowData.loadKw !== "") ? String(rowData.loadKw) : "";
                                fVolt.options = C.voltageOptions(rowData.voltage !== undefined ? rowData.voltage : "380");
                                fVolt.value = (rowData.voltage !== undefined && rowData.voltage !== "") ? rowData.voltage : "380";
                                fLen.text = (rowData.length !== undefined && rowData.length !== "") ? String(rowData.length) : "";
                            }

                            Component.onCompleted: sync()

                            Connections {
                                target: root
                                function onEditRowsChanged() { edRow.sync(); }
                            }

                            PPText {
                                id: fCode
                                width: 78; hint: "编号"
                                onTextEdited: root.setCell(edRow.rowIndex, "code", text)
                            }
                            PPText {
                                id: fName
                                width: 100; hint: "名称"
                                onTextEdited: root.setCell(edRow.rowIndex, "name", text)
                            }
                            PPText {
                                id: fUp
                                width: 70; hint: "上游"
                                onTextEdited: root.setCell(edRow.rowIndex, "upstream", text)
                            }
                            PPText {
                                id: fCable
                                width: 130; hint: "电缆型号"
                                onTextEdited: root.setCell(edRow.rowIndex, "cable", text)
                            }
                            PPText {
                                id: fLoad
                                width: 52; hint: "0"
                                onTextEdited: root.setCell(edRow.rowIndex, "loadKw", text)
                            }
                            PPCombo {
                                id: fVolt
                                width: 100
                                onValueChanged: root.setCell(edRow.rowIndex, "voltage", value)
                            }
                            PPText {
                                id: fLen
                                width: 56; hint: "0"
                                onTextEdited: root.setCell(edRow.rowIndex, "length", text)
                            }
                            Item {
                                width: 26; height: 28
                                Text {
                                    anchors.centerIn: parent
                                    text: "✕"
                                    font.pixelSize: 14
                                    color: hover.hovered ? "#B42318" : "#98A2B3"
                                }
                                HoverHandler { id: hover }
                                TapHandler { onTapped: root.editorRemove(edRow.rowIndex) }
                            }
                        }
                    }
                }
            }

            Rectangle {
                id: edFoot
                width: parent.width
                height: 26
                color: "#FFFFFF"
                Rectangle {
                    anchors.top: parent.top
                    width: parent.width
                    height: 1
                    color: "#E4E7EC"
                }
                Text {
                    anchors.centerIn: parent
                    text: "上游填现有编号即挂到该节点下；留空为起点。改完点「应用并重绘」。"
                    font.pixelSize: 12
                    font.family: "Microsoft YaHei"
                    color: "#667085"
                }
            }
        }
    }

    // ---- 粘贴 JSON ----
    Rectangle {
        visible: root.pasteVisible
        anchors.fill: parent
        color: "#00000055"
        z: 50

        Rectangle {
            width: Math.min(parent.width - 80, 620)
            height: Math.min(parent.height - 80, 420)
            anchors.centerIn: parent
            color: "#FFFFFF"
            radius: 6
            border.width: 1
            border.color: "#D0D5DD"

            Column {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8
                Text {
                    text: "粘贴 JSON（支持 {\"nodes\":[...]} 或节点数组）"
                    font.pixelSize: 13
                    font.bold: true
                    font.family: "Microsoft YaHei"
                    color: "#1F2A37"
                }
                ScrollView {
                    width: parent.width
                    height: parent.height - 76
                    TextArea {
                        id: pasteArea
                        wrapMode: TextArea.Wrap
                        font.pixelSize: 12
                        font.family: "Consolas"
                        selectByMouse: true
                        background: Rectangle {
                            border.width: 1
                            border.color: "#D0D5DD"
                            radius: 3
                        }
                    }
                }
                Row {
                    spacing: 6
                    PPButton {
                        text: "确定"
                        primary: true
                        onClicked: {
                            var r = root.loadJsonText(pasteArea.text);
                            if (r === "ok") {
                                root.pasteVisible = false;
                                pasteArea.text = "";
                            }
                        }
                    }
                    PPButton {
                        text: "取消"
                        onClicked: { root.pasteVisible = false; pasteArea.text = ""; }
                    }
                }
            }
        }
    }

    // ---- 删除确认 ----
    Rectangle {
        visible: root.confirmVisible
        anchors.fill: parent
        color: "#00000055"
        z: 55

        Rectangle {
            width: 340
            height: 132
            anchors.centerIn: parent
            color: "#FFFFFF"
            radius: 6
            border.width: 1
            border.color: "#D0D5DD"

            Column {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10
                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    text: root.confirmText
                    font.pixelSize: 13
                    font.family: "Microsoft YaHei"
                    color: "#1F2A37"
                }
                Row {
                    spacing: 8
                    PPButton {
                        text: "删除"
                        primary: true
                        onClicked: {
                            root.confirmVisible = false;
                            root.doDelete(root.confirmCode);
                        }
                    }
                    PPButton { text: "取消"; onClicked: root.confirmVisible = false }
                }
            }
        }
    }

    // ============================================================
    // Python 桥（文件对话框 / 读写都在 Python 侧）
    // ============================================================
    function backendCall(m) {
        try {
            if (typeof backend === "undefined" || !backend) return "err:没有 backend 对象";
            return String(backend.call(m));
        } catch (e) {
            return "err:" + e.message;
        }
    }

    function io(kind) {
        var r = backendCall("io:" + kind);
        if (r === "ok") {
            var res = (kind === "importJson") ? loadJsonText(payloadText) : loadCsvText(payloadText);
            if (res !== "ok") showBanner(String(res).replace(/^err:/, ""), "error");
        } else if (r !== "cancel" && r !== "") {
            showBanner(String(r).replace(/^err:/, ""), "error");
        }
    }

    function save(kind) {
        var text = "";
        if (kind === "saveJson") text = exportJsonText();
        else if (kind === "saveCsv") text = exportCsvText();
        else if (kind === "saveSvg") text = exportSvgText();
        if (!text) { showBanner("当前没有图纸可导出", "error"); return; }
        payloadText = text;
        var r = backendCall("io:" + kind);
        if (r === "ok") showBanner("已导出：" + payloadText, "warn");
        else if (r !== "cancel" && r !== "") showBanner(String(r).replace(/^err:/, ""), "error");
    }

    // 启动默认载入三级配电示例
    Component.onCompleted: {
        loadThreeLevel();
        editorOpen = false;          // 与 HTML 一致：抽屉默认收起，点"节点编辑"才滑出
        fillEditor();
        fitView();
    }
}
