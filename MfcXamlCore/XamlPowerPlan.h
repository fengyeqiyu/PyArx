#pragma once
// 配电规划图（Power Plan）：数据结构 + 自动分层布局 + XAML/JSON/CSV/SVG 出口。
// 布局与 HTML 版 power-plan.html 同构：按"上游"建成树，子树沿主轴累加，
// 父卡片居中于首末子中心之间，同层卡片对齐，因此回路曲线（S 形）不会交叉。
// 两种走向：纵向（层向下、兄弟横向排开，默认）/ 横向（层向右、兄弟纵向排开）。
#include <string>
#include <vector>
#include "XamlCore.h"

namespace xamlplan
{
    // 一个配电节点。上游为空 = 起点（变压器/进线）。
    struct PowerNode
    {
        std::wstring code;      // 编号（唯一，如 AT1 / WL3）
        std::wstring name;      // 名称（如 "1号变压器"，可空）
        std::wstring upstream;  // 上游编号（空 = 起点）
        std::wstring cable;     // 进入本节点的电缆型号（如 YJV22-4×35）
        double loadKw = 0.0;    // 本节点负载（kW），0 表示未填
        std::wstring voltage = L"380";  // 归一化电压档：380 / 220 / 10kV / 110kV …
        double length = 0.0;    // 电缆长度（m），0 表示未填
        // 以下由 buildPlan 或调用方填：
        std::wstring role;      // root / branch / terminal
        int collapsedCount = 0; // 折叠后其下被隐藏的下游数量（0 = 未折叠）
        int dual = 0;           // 1 = 双电源柜（一个卡片两个上游），卡片画红色
    };

    // 布局后的卡片（画布绝对坐标，左上原点）
    struct PowerBox
    {
        std::wstring code;
        int x = 0, y = 0, w = 0, h = 0;
        int depth = 0;
    };

    struct PowerLink
    {
        std::wstring from, to;   // 上游编号 → 本节点编号
        std::wstring cable;      // 下游节点的电缆型号（连线标注用）
        int x1 = 0, y1 = 0;      // 起端（纵向=父底边中点，横向=父右边中点）
        int x2 = 0, y2 = 0;
        int vertical = 1;        // 1 = 竖直 S，0 = 水平 S
        int dashed = 0;          // 1 = 备用电源进线（双电源的第二条上游），画虚线
        int magenta = 0;         // 1 = 双电源柜的回路，线画洋红色
        double bez[8] = {0};     // 三次贝塞尔控制点：(x1,y1, c1x,c1y, c2x,c2y, x2,y2)
    };

    struct PowerPlan
    {
        std::vector<PowerNode> nodes;   // 与 boxes 一一对应
        std::vector<PowerBox> boxes;
        std::vector<PowerLink> links;
        int width = 0, height = 0;              // 生成的画布尺寸
        std::wstring dir = L"v";                // v = 纵向层流，h = 横向列流
        std::vector<std::wstring> warnings;     // 孤立节点、环路、编号重复等
    };

    // 布局尺寸参数（与 HTML 版 CFG 一致）
    struct PowerPlanStyle
    {
        int boxW = 132;       // 电柜/起点基准宽
        int boxH = 48;
        int termW = 120;      // 末端负载基准宽
        int termH = 44;
        int nameH = 8;        // 有名称时增加的高度
        int gapX = 30;        // 同级兄弟间距
        int gapY = 56;        // 层间距（纵向）；横向的列间距用 colGap
        int margin = 24;      // 画布外边距
        int rootGap = 64;     // 多棵树之间的间距
        int colGap = 100;     // 横向布局的列间距（留出 S 线空间）
    };

    // 电压角标样式
    struct VoltageStyle
    {
        std::wstring text;    // 380V / 220V / 10kV
        unsigned color = 0;   // 0xRRGGBB
        unsigned bg = 0;
    };

    // 电压归一化：空 → "380"；"10kV"/"10千伏" → "10kV"；含 380/三相 → "380"；含 220/单相 → "220"；其余 → "380"
    XAML_CORE_API std::wstring normVoltage(const std::wstring& raw);
    // 0 = 380V 低压, 1 = 220V 单相, 2 = 中压, 3 = 高压
    XAML_CORE_API int voltClass(const std::wstring& normalized);
    XAML_CORE_API VoltageStyle voltStyle(const std::wstring& normalized);
    // 卡片第三行：电缆型号 · 负载 kW · 长度 m
    XAML_CORE_API std::wstring cableLine(const PowerNode& n);

    // 建树 + 布局。horizontal=false 为默认的纵向层流。失败时 err 说明原因
    XAML_CORE_API bool buildPlan(const std::vector<PowerNode>& nodes, PowerPlan& out,
                                 const PowerPlanStyle& style, std::wstring& err,
                                 bool horizontal = false);

    // XAML（设计器文档）：Window + 一张 PowerCanvas（Data = 布局 JSON）
    XAML_CORE_API std::wstring toXaml(const PowerPlan& plan, const std::wstring& title);

    // JSON：nodes（原始数据）+ layout（坐标），便于外部工具复现同一张图
    XAML_CORE_API std::wstring toJson(const PowerPlan& plan);

    // CSV：Excel 直接双击可开（调用方负责写 UTF-8 BOM）
    XAML_CORE_API std::wstring toCsv(const std::vector<PowerNode>& nodes);

    // SVG：与屏幕同一张图（含图名；withTitleBlock=true 时底部加图框标题栏）
    XAML_CORE_API std::wstring toSvg(const PowerPlan& plan, const std::wstring& title,
                                     bool withTitleBlock);

    // 读 CSV / JSON。title 输出标题（可空）
    XAML_CORE_API bool parseCsv(const std::wstring& text, std::vector<PowerNode>& out,
                                std::wstring& title, std::wstring& err);
    XAML_CORE_API bool parseJson(const std::wstring& text, std::vector<PowerNode>& out,
                                 std::wstring& title, std::wstring& err,
                                 bool* horizontal = nullptr);

    // 直接吃"已算好坐标"的场景 JSON（toJson 同构：nodes + layout），不做任何布局。
    // 让坐标由 Python/JSON 引擎算，C++ 只按坐标画 —— 控件库不该复制一份排版算法。
    XAML_CORE_API bool parseScene(const std::wstring& text, PowerPlan& out, std::wstring& err);

    // 自检：统计回路曲线之间的交叉对数（0 = 满足"不许交叉"）
    XAML_CORE_API int countCrossings(const PowerPlan& plan);
}
