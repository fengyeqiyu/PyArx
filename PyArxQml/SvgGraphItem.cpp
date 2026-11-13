// SvgGraphItem.cpp
#include "SvgGraphItem.h"
#include "PyArxQmlLog.h"

#include <QPainter>
#include <QPainterPath>
#include <QSvgRenderer>
#include <QElapsedTimer>
#include <QQuickWindow>
#include <QPaintDevice>
#include <QFontMetricsF>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHash>

static const int kCacheMarginPx = 400;   // 缓存比视口每侧多留的像素，平移时少重绘
static const char* kFontFamily = "Microsoft YaHei";

SvgGraphItem::SvgGraphItem(QQuickItem* parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(true);
    setRenderTarget(QQuickPaintedItem::Image);   // Image 模式下 paint() 在 GUI 线程，可安全访问成员
    m_idle.setSingleShot(true);
    m_idle.setInterval(90);
    connect(&m_idle, &QTimer::timeout, this, &SvgGraphItem::onIdle);
}

void SvgGraphItem::setOps(const QString& s)
{
    if (s == m_ops) return;
    m_ops = s;
    m_opsValid = false;

    if (!s.isEmpty()) {
        QJsonParseError err;
        const QJsonDocument doc = QJsonDocument::fromJson(s.toUtf8(), &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            const QJsonObject o = doc.object();
            m_opsLinks = o.value(QStringLiteral("links")).toArray();
            m_opsRects = o.value(QStringLiteral("rects")).toArray();
            m_opsTexts = o.value(QStringLiteral("texts")).toArray();
            m_opsValid = true;
        } else {
            LogWrite("SvgGraph: 绘制指令解析失败（%d 字节，%s）",
                     (int)s.size(), err.errorString().toUtf8().constData());
        }
    }
    update();
    Q_EMIT opsChanged();
}

void SvgGraphItem::paintOps(QPainter* painter)
{
    QElapsedTimer clock;
    clock.start();
    // 视口反算到图坐标，只画可见范围（缩放小、图大时能省掉一半以上的图元）
    const QRectF view(-m_offset.x() / m_zoom, -m_offset.y() / m_zoom,
                      width() / m_zoom, height() / m_zoom);

    painter->save();
    painter->translate(m_offset);
    painter->scale(m_zoom, m_zoom);

    // 连线（三次贝塞尔）
    for (const QJsonValue& v : m_opsLinks) {
        const QJsonArray a = v.toArray();
        if (a.size() < 10) continue;
        {
            const QRectF lb(QPointF(a.at(0).toDouble(), a.at(1).toDouble()),
                            QPointF(a.at(6).toDouble(), a.at(7).toDouble()));
            if (!view.adjusted(-4, -4, 4, 4).intersects(lb.normalized())) continue;
        }
        QPainterPath path(QPointF(a.at(0).toDouble(), a.at(1).toDouble()));
        path.cubicTo(QPointF(a.at(2).toDouble(), a.at(3).toDouble()),
                     QPointF(a.at(4).toDouble(), a.at(5).toDouble()),
                     QPointF(a.at(6).toDouble(), a.at(7).toDouble()));
        QPen pen(QColor(a.at(8).toString()));
        // 低缩放下 2*0.2=0.4px 的线会被栅格断成虚线；保证至少 1 个设备像素宽
        pen.setWidthF(qMax(a.at(9).toDouble(), 1.0 / m_zoom));
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(path);
    }

    // 矩形（节点卡片、电压角标）
    for (const QJsonValue& v : m_opsRects) {
        const QJsonArray a = v.toArray();
        if (a.size() < 8) continue;
        const QRectF r(a.at(0).toDouble(), a.at(1).toDouble(),
                       a.at(2).toDouble(), a.at(3).toDouble());
        if (!view.adjusted(-4, -4, 4, 4).intersects(r)) continue;
        const qreal rad = a.at(4).toDouble();
        QPen pen(QColor(a.at(6).toString()));
        pen.setWidthF(qMax(a.at(7).toDouble(), 1.0 / m_zoom));
        painter->setPen(pen);
        painter->setBrush(QColor(a.at(5).toString()));
        if (rad > 0.01) painter->drawRoundedRect(r, rad, rad);
        else painter->drawRect(r);
    }

    painter->restore();


    // 文字放到设备像素空间绘制：字号按当前缩放量化成整数像素再栅格化。
    // 在缩放坐标系里给字号会把 10.5×0.4=4.2px 就近取整后再缩，字形发软——
    // 这正是「比 HTML 糊」的直接原因，浏览器是按设备像素精确排版字形的。
    static QHash<QString, QFont> s_fonts;
    static QHash<QString, qreal> s_widths;
    const qreal k = m_zoom;
    const qreal ox = m_offset.x();
    const qreal oy = m_offset.y();

    for (const QJsonValue& v : m_opsTexts) {
        const QJsonArray a = v.toArray();
        if (a.size() < 7) continue;
        const QString text = a.at(2).toString();
        if (text.isEmpty()) continue;

        const double px = a.at(3).toDouble() * k;      // 设备像素字号
        if (px < 2.0) continue;                        // 低于 2px 才跳过：再小也保留文字纹理，卡片不会变空条
        const int ipx = qRound(px);
        const int weight = a.at(4).toInt();
        const QString key = QString::number(ipx) + (weight >= 600 ? QLatin1Char('b') : QLatin1Char('n'));

        QFont f;
        if (s_fonts.contains(key)) {
            f = s_fonts.value(key);
        } else {
            f.setFamily(QString::fromLatin1(kFontFamily));
            f.setPixelSize(ipx);                       // 直接给像素字号，不经点位/DPI 折算
            f.setWeight(weight >= 600 ? QFont::DemiBold : QFont::Normal);
            f.setHintingPreference(QFont::PreferNoHinting);   // 与浏览器的字形渲染一致
            f.setStyleStrategy(QFont::NoSubpixelAntialias);   // 灰阶抗锯齿，去掉彩边
            s_fonts.insert(key, f);
        }


        const qreal x = ox + a.at(0).toDouble() * k;
        const qreal y = oy + a.at(1).toDouble() * k;
        if (y < -12 || y > height() + 12) continue;

        painter->setFont(f);
        painter->setPen(QColor(a.at(5).toString()));

        const int align = a.at(6).toInt();
        // 用字形轮廓填充而不是 drawText：Windows 字体引擎会走 ClearType，
        // 中文小字会出现明显彩边；轮廓填充是纯灰阶抗锯齿，与浏览器观感一致。
        static QHash<QString, QPainterPath> s_paths;
        const QString pk = key + QLatin1Char('|') + text;
        QPainterPath path = s_paths.value(pk);
        if (path.isEmpty()) {
            path.addText(0, 0, f, text);
            s_paths.insert(pk, path);
        }
        qreal anchor = x;
        if (align != 1) {
            const QString wk = key + QLatin1Char('|') + text;
            qreal w = s_widths.value(wk, -1.0);
            if (w < 0.0) {
                w = path.boundingRect().width();
                s_widths.insert(wk, w);
            }
            anchor = (align == 0) ? x - w / 2 : x - w;
            if (anchor > width() + 4 || anchor + w < -4) continue;
        } else if (anchor > width() + 4) {
            continue;
        }
        painter->save();
        painter->translate(anchor, y);
        painter->setBrush(QColor(a.at(5).toString()));
        painter->setPen(Qt::NoPen);
        painter->drawPath(path);
        painter->restore();
    }

    static int s_n = 0, s_slow = 0;
    static double s_sum = 0;
    const double ms = (double)clock.nsecsElapsed() / 1e6;
    ++s_n; s_sum += ms;
    if (ms > 6.0) {
        ++s_slow;
        LogWrite("SvgGraph: 直绘 %.1f ms（zoom %.2f，节点 %d，平均 %.1f ms）",
                 ms, (double)m_zoom, (int)m_opsRects.size(), s_sum / s_n);
    } else if (s_n % 60 == 0) {
        LogWrite("SvgGraph: 直绘平均 %.2f ms（第 %d 帧，慢帧 %d）",
                 s_sum / s_n, s_n, s_slow);
    }
}

void SvgGraphItem::setSvg(const QString& s)
{
    if (s == m_svg) return;
    m_svg = s;
    m_data = s.toUtf8();

    m_valid = m_renderer.load(m_data);
    if (!m_valid) {
        LogWrite("SvgGraph: QSvgRenderer 解析失败（%d 字节，首 80 字符：%s）",
                 (int)m_data.size(), s.left(80).toUtf8().constData());
    }
    m_cache = QImage();          // 内容变了，缓存作废，下一帧重绘
    m_cacheZoom = 0.0;
    update();
    Q_EMIT svgChanged();
}

void SvgGraphItem::setOffset(const QPointF& p)
{
    if (p == m_offset) return;
    m_offset = p;
    update();
    Q_EMIT offsetChanged();
}

void SvgGraphItem::setZoom(qreal z)
{
    if (qFuzzyCompare(z, m_zoom)) return;
    m_zoom = z;
    update();
    Q_EMIT zoomChanged();
}

void SvgGraphItem::onIdle()
{
    if (!m_valid) return;
    if (m_cache.isNull() || m_cacheZoom != m_zoom ||
        m_cacheView != QSize(int(width()), int(height())) || !cacheCoversView()) {
        renderCache();
        update();
    }
}

bool SvgGraphItem::cacheCoversView() const
{
    if (m_cache.isNull() || m_cacheZoom <= 0.0) return false;
    const QRectF vis(-m_offset.x() / m_zoom, -m_offset.y() / m_zoom,
                     width() / m_zoom, height() / m_zoom);
    const QRectF cached(m_cacheOrigin,
                        QSizeF(m_cache.width() / m_cacheZoom, m_cache.height() / m_cacheZoom));
    return cached.contains(vis);
}

void SvgGraphItem::renderCache()
{
    const QSizeF gs = m_renderer.defaultSize();
    if (gs.isEmpty()) return;

    const int vw = int(width());
    const int vh = int(height());
    if (vw <= 0 || vh <= 0 || m_zoom <= 0.0) return;

    // 视口对应的图坐标矩形，四周各多留 kCacheMarginPx 像素
    QRectF vis(-m_offset.x() / m_zoom, -m_offset.y() / m_zoom,
               vw / m_zoom, vh / m_zoom);
    vis = vis.adjusted(-kCacheMarginPx / m_zoom, -kCacheMarginPx / m_zoom,
                        kCacheMarginPx / m_zoom,  kCacheMarginPx / m_zoom);

    QElapsedTimer t;
    t.start();

    m_cache = QImage(QSize(vw + 2 * kCacheMarginPx, vh + 2 * kCacheMarginPx),
                     QImage::Format_ARGB32_Premultiplied);
    m_cache.fill(Qt::transparent);
    m_cacheOrigin = vis.topLeft();
    m_cacheZoom = m_zoom;
    m_cacheView = QSize(vw, vh);

    QPainter p(&m_cache);
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing |
                     QPainter::SmoothPixmapTransform | QPainter::LosslessImageRendering);
    p.scale(m_zoom, m_zoom);
    p.translate(-m_cacheOrigin.x(), -m_cacheOrigin.y());
    m_renderer.render(&p, QRectF(0, 0, gs.width(), gs.height()));
    p.end();

    static int s_logged = 0;
    if (s_logged < 12) {
        ++s_logged;
        LogWrite("SvgGraph: 精确重绘 %.0f ms（zoom %.2f，缓存 %dx%d）",
                 (double)t.elapsed(), double(m_zoom), m_cache.width(), m_cache.height());
    }
}

void SvgGraphItem::paint(QPainter* painter)
{
    // 有绘制指令时走 QPainter 直绘：每帧按当前缩放矢量重画，几十毫秒 → 几毫秒
    if (m_opsValid) {
        painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing |
                                QPainter::SmoothPixmapTransform);
        paintOps(painter);
        return;
    }

    if (!m_valid) return;

    const int vw = int(width());
    const int vh = int(height());
    if (vw <= 0 || vh <= 0) return;

    // 缓存不精确（首帧、缩放变了、视口跑出缓存范围）时立即精确重绘：
    // 实测本宿主的手动 pump 不触发 QTimer，所以不能用"停手后补绘"的定时器方案。
    // 代价是缩放每档约 90ms，平移在 400px 缓存余量内只需一次位图贴图（约 1ms）。
    if (m_cache.isNull() || m_cacheZoom != m_zoom ||
        m_cacheView != QSize(vw, vh) || !cacheCoversView()) {
        renderCache();
        if (m_cache.isNull()) return;
    }

    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing |
                            QPainter::SmoothPixmapTransform | QPainter::LosslessImageRendering);

    // 缓存像素 (px,py) 对应图坐标 cacheOrigin + (px,py)/cacheZoom；
    // 屏幕位置 = offset + 图坐标 * zoom  ⇒  dst = offset + cacheOrigin*zoom，缩放比 = zoom/cacheZoom
    const QPointF dst(m_offset.x() + m_cacheOrigin.x() * m_zoom,
                      m_offset.y() + m_cacheOrigin.y() * m_zoom);
    const qreal factor = m_zoom / m_cacheZoom;

    painter->save();
    painter->translate(dst);
    if (!qFuzzyCompare(factor, 1.0)) painter->scale(factor, factor);
    painter->drawImage(QPointF(0, 0), m_cache);
    painter->restore();


}
