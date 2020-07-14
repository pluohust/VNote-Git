#ifndef VALLTAGSPANEL_H
#define VALLTAGSPANEL_H

#include <QWidget>

class QListWidget;
class QListWidgetItem;
class VTagLabel;

class VAllTagsPanel : public QWidget
{
    Q_OBJECT
public:
    explicit VAllTagsPanel(QWidget *p_parent = nullptr);

    void clear();

    VTagLabel *addTag(const QString &p_text);

signals:
    void tagRemoved(const QString &p_text);

protected:
    void showEvent(QShowEvent *p_event) Q_DECL_OVERRIDE;

    void keyPressEvent(QKeyEvent *p_event) Q_DECL_OVERRIDE;

private:
    void removeItem(QListWidgetItem *p_item);

    QListWidget *m_list;
};

#endif // VALLTAGSPANEL_H
