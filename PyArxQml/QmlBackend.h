// QmlBackend.h —— 注入 QML context 的后端对象：QML 里写 backend.addNumbers(a, b)
#pragma once
#include <QObject>
#include <QString>

class QmlBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString result READ result NOTIFY resultChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    explicit QmlBackend(QObject* parent = nullptr);

    QString result() const { return m_result; }
    QString status() const { return m_status; }

    Q_INVOKABLE void addNumbers(int a, int b);          // 计算落到 Python
    Q_INVOKABLE void reportClick(const QString& what);  // QML 动作回报 C++
    Q_INVOKABLE QString call(const QString& method);    // 通用：调 Python(方法名)

    void setResult(const QString& r);
    void setStatus(const QString& s);

Q_SIGNALS:
    void resultChanged();
    void statusChanged();

private:
    QString m_result = QStringLiteral("等待 Python");
    QString m_status = QStringLiteral("尚未交互");
};
