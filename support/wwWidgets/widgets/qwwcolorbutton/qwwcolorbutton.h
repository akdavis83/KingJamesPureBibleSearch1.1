//
// C++ Interface: qcolorbutton
//
// Description:
//
//
// Author: Witold Wysota <wysota@wysota.eu.org>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef QWWCOLORBUTTON_H
#define QWWCOLORBUTTON_H

#ifndef WW_NO_COLORBUTTON

#include <QPushButton>
#include <QModelIndex>
#include <wwglobal.h>


class QwwColorButtonPrivate;
class Q_WW_EXPORT QwwColorButton : public QPushButton, public QwwPrivatable {
    Q_OBJECT
    Q_PROPERTY(QStringList colors READ colors WRITE setColors)
    Q_PROPERTY(QColor currentColor READ currentColor WRITE setCurrentColor)
    Q_PROPERTY(bool dragEnabled READ dragEnabled WRITE setDragEnabled)
    Q_PROPERTY(bool showName READ showName WRITE setShowName)
public:
    QwwColorButton(QWidget *parent = nullptr);
    void addColor(const QColor &c, const QString &n = QString());
    void clear();
    QStringList colors() const;
    QColor currentColor() const;
    bool dragEnabled() const;
    void setColors(const QStringList &);
    void setDragEnabled(bool v);
    virtual void setStandardColors();
    bool showName() const;

public slots:
    void setCurrentColor(const QColor &c);
    void setShowName(bool v);
signals:
    void colorPicked(const QColor &);
protected:
    virtual void mousePressEvent(QMouseEvent*) override;
    virtual void mouseMoveEvent(QMouseEvent*) override;
    virtual void dragEnterEvent(QDragEnterEvent*) override;
    virtual void dropEvent(QDropEvent*) override;
private:
    WW_DECLARE_PRIVATE(QwwColorButton);
    Q_PRIVATE_SLOT(d_func(), void _q_clicked());
    Q_PRIVATE_SLOT(d_func(), void _q_activated(const QModelIndex &));
    Q_PRIVATE_SLOT(d_func(), void _q_colorDialogRequested());
    Q_PRIVATE_SLOT(d_func(), void _q_setCurrentIndex(const QModelIndex &));
};

#endif // WW_NO_COLORBUTTON

#endif // QWWCOLORBUTTON_H
