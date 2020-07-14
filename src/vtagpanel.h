#ifndef VTAGPANEL_H
#define VTAGPANEL_H

#include <QWidget>
#include "vnavigationmode.h"

#include <QVector>

class VTagLabel;
class VButtonWithWidget;
class VNoteFile;
class VAllTagsPanel;
class VLineEdit;
class QStringListModel;
class VNotebook;

class VTagPanel : public QWidget, public VNavigationMode
{
    Q_OBJECT
public:
    explicit VTagPanel(QWidget *parent = nullptr);

    void updateTags(VNoteFile *p_file);

    // Implementations for VNavigationMode.
    void showNavigation() Q_DECL_OVERRIDE;
    bool handleKeyNavigation(int p_key, bool &p_succeed) Q_DECL_OVERRIDE;

protected:
    bool eventFilter(QObject *p_obj, QEvent *p_event) Q_DECL_OVERRIDE;

private slots:
    void updateAllTagsPanel();

    void removeTag(const QString &p_text);

private:
    void setupUI();

    void clearLabels();

    void updateTags();

    bool addTag(const QString &p_text);

    void updateCompleter(const VNoteFile *p_file);

    void updateCompleter();

    bool isAllTagsPanelAvailable() const;

    QVector<VTagLabel *> m_labels;

    VButtonWithWidget *m_btn;

    VAllTagsPanel *m_tagsPanel;

    VLineEdit *m_tagEdit;

    // Used for auto completion.
    QStringListModel *m_tagsModel;

    VNoteFile *m_file;

    const VNotebook *m_notebookOfCompleter;
};
#endif // VTAGPANEL_H
