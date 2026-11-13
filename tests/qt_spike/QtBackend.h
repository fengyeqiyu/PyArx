// QtBackend.h —— 暴露给 QML 的后端对象：QML 里写 backend.addNumbers(1234, 5678)
// 就会调到 C++，C++ 再转到 Python 回调。
#pragma once
#include <QObject>
#include <QString>

class QtBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString result READ result NOTIFY resultChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    explicit QtBackend(QObject* parent = nullptr);

    QString result() const { return m_result; }
    QString status() const { return m_status; }

    // QML 可调用：算两个数（实际计算发生在 Python 侧）
    Q_INVOKABLE void addNumbers(int a, int b);
    // QML 可调用：把页面上的动作回报给 Python（验证反向通道）
    Q_INVOKABLE void reportClick(const QString& what);

    void setResult(const QString& r);
    void setStatus(const QString& s);

Q_SIGNALS:
    void resultChanged();
    void statusChanged();

private:
    QString m_result = QStringLiteral("等待 Python");
    QString m_status = QStringLiteral("尚未点击");
};
