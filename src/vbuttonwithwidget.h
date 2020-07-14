#ifndef VBUTTONWITHWIDGET_H
#define VBUTTONWITHWIDGET_H

#include <QPushButton>
#include <QString>
#include <QIcon>
#include <QWidgetAction>

class QDragEnterEvent;
class QDropEvent;
class QPaintEvent;
class QShowEvent;
class VButtonWithWidget;

// Abstract class for the widget used by VButtonWithWidget.
// Widget need to inherit this class if drag/drop is needed.
class VButtonPopupWidget
{
public:
    VButtonPopupWidget(QWidget *p_widget)
        : m_widget(p_widget), m_btn(NULL)
    {
    }

    virtual bool isAcceptDrops() const = 0;
    virtual bool handleDragEnterEvent(QDragEnterEvent *p_event) = 0;
    virtual bool handleDropEvent(QDropEvent *p_event) = 0;
    virtual void handleAboutToShow() = 0;

    void setButton(VButtonWithWidget *p_btn)
    {
        m_btn = p_btn;
    }

    VButtonWithWidget *getButton() const
    {
        return m_btn;
    }

protected:
    // **MUST** be called in subclass at the end of showEvent().
    void processShowEvent(QShowEvent *p_event)
    {
        Q_UNUSED(p_event);
        m_widget->activateWindow();
    }

private:
    QWidget *m_widget;

    VButtonWithWidget *m_btn;
};

// A QPushButton with popup widget.
class VButtonWithWidget : public QPushButton
{
    Q_OBJECT
public:
    VButtonWithWidget(QWidget *p_widget,
                      QWidget *p_parent = Q_NULLPTR);

    VButtonWithWidget(const QString &p_text,
                      QWidget *p_widget,
                      QWidget *p_parent = Q_NULLPTR);

    VButtonWithWidget(const QIcon &p_icon,
                      const QString &p_text,
                      QWidget *p_widget,
                      QWidget *p_parent = Q_NULLPTR);

    QWidget *getPopupWidget() const;

    // Set the bubble to display a number @p_num.
    // @p_num: -1 to hide the bubble.
    void setBubbleNumber(int p_num);

    // Set the foreground and background of the bubble.
    void setBubbleColor(const QColor &p_fg, const QColor &p_bg);

public slots:
    // Show the popup widget.
    void showPopupWidget();

    void hidePopupWidget();

signals:
    // Emit when popup widget is about to show.
    void popupWidgetAboutToShow(QWidget *p_widget);

protected:
    // To accept specific drop.
    void dragEnterEvent(QDragEnterEvent *p_event) Q_DECL_OVERRIDE;

    // Drop the data.
    void dropEvent(QDropEvent *p_event) Q_DECL_OVERRIDE;

    void paintEvent(QPaintEvent *p_event) Q_DECL_OVERRIDE;

private:
    void init();

    // Get VButtonWithWidget from m_popupWidget.
    VButtonPopupWidget *getButtonPopupWidget() const;

    QWidget *m_popupWidget;

    QColor m_bubbleBg;
    QColor m_bubbleFg;

    // String to display in the bubble.
    // Empty to hide bubble.
    QString m_bubbleStr;
};

inline VButtonPopupWidget *VButtonWithWidget::getButtonPopupWidget() const
{
    return dynamic_cast<VButtonPopupWidget *>(m_popupWidget);
}

inline void VButtonWithWidget::setBubbleColor(const QColor &p_fg, const QColor &p_bg)
{
    m_bubbleFg = p_fg;
    m_bubbleBg = p_bg;
}
#endif // VBUTTONWITHWIDGET_H
