#ifndef VLINEEDIT_H
#define VLINEEDIT_H

#include <QLineEdit>


class VLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit VLineEdit(QWidget *p_parent = nullptr);

    VLineEdit(const QString &p_contents, QWidget *p_parent = nullptr);

    void setCtrlKEnabled(bool p_enabled);

    void setCtrlEEnabled(bool p_enabled);

protected:
    void keyPressEvent(QKeyEvent *p_event) Q_DECL_OVERRIDE;

private:
    // Enable Ctrl+K shortcut.
    // In QLineEdit, Ctrl+K will delete till the end.
    bool m_ctrlKEnabled;

    // Enable Ctrl+E shortcut.
    // In QLineEdit, Ctrl+E will move the cursor to the end.
    bool m_ctrlEEnabled;
};

inline void VLineEdit::setCtrlKEnabled(bool p_enabled)
{
    m_ctrlKEnabled = p_enabled;
}

inline void VLineEdit::setCtrlEEnabled(bool p_enabled)
{
    m_ctrlEEnabled = p_enabled;
}
#endif // VLINEEDIT_H
