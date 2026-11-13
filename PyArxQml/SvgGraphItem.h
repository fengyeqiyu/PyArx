// SvgGraphItem.h —— 用 QSvgRenderer 在 QQuickPaintedItem 上矢量绘制配电干线图
// 目的：像 HTML 的 SVG 那样按当前缩放重新栅格化，任何倍率下都锐利。
// 性能策略：交互（平移/滚轮缩放）阶段只把上一次的栅格按偏移/比例贴出去（约 1ms/帧），
// 停手 90ms 后再按当前缩放精确重绘一张（约 86ms/次），因此既不卡也不糊。
#pragma once

#include <QQuickPaintedItem>
#include <QSvgRenderer>
#include <QByteArray>
#include <QPointF>
#include <QImage>
#include <QTimer>
#include <QJsonArray>

class SvgGraphItem : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QString svg READ svg WRITE setSvg NOTIFY svgChanged)
    Q_PROPERTY(QString ops READ ops WRITE setOps NOTIFY opsChanged)
    Q_PROPERTY(QPointF offset READ offset WRITE setOffset NOTIFY offsetChanged)
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(bool valid READ valid NOTIFY svgChanged)

public:
    explicit SvgGraphItem(QQuickItem* parent = nullptr);

    void paint(QPainter* painter) override;

    QString svg() const { return m_svg; }
    void setSvg(const QString& s);

    // 绘制指令（JSON）：优先用它，QPainter 直接画，不做 SVG 解析
    QString ops() const { return m_ops; }
    void setOps(const QString& s);

    QPointF offset() const { return m_offset; }
    void setOffset(const QPointF& p);

    qreal zoom() const { return m_zoom; }
    void setZoom(qreal z);

    bool valid() const { return m_valid; }

Q_SIGNALS:
    void svgChanged();
    void opsChanged();
    void offsetChanged();
    void zoomChanged();

private:
    void renderCache();          // 按当前 zoom/offset 精确渲染一张带余量的栅格
    bool cacheCoversView() const;
    void onIdle();

    QString      m_svg;
    QByteArray   m_data;
    QSvgRenderer m_renderer;
    QPointF      m_offset;
    qreal        m_zoom = 1.0;
    bool         m_valid = false;

    QImage  m_cache;             // 上一次精确渲染的栅格
    QPointF m_cacheOrigin;       // 栅格左上角对应的图坐标（未缩放）
    qreal   m_cacheZoom = 0.0;
    QSize   m_cacheView;
    QTimer  m_idle;              // 停手后补一次精确重绘

    QString   m_ops;             // 绘制指令 JSON
    QJsonArray m_opsLinks;
    QJsonArray m_opsRects;
    QJsonArray m_opsTexts;
    bool      m_opsValid = false;
    void paintOps(QPainter* painter);
};
