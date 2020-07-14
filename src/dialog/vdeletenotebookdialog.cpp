#include <QtWidgets>
#include "vdeletenotebookdialog.h"
#include "vconfigmanager.h"

extern VConfigManager *g_config;

VDeleteNotebookDialog::VDeleteNotebookDialog(const QString &p_title,
                                             const VNotebook *p_notebook,
                                             QWidget *p_parent)
    : QDialog(p_parent), m_notebook(p_notebook)
{
    setupUI(p_title, m_notebook->getName());
}

void VDeleteNotebookDialog::setupUI(const QString &p_title, const QString &p_name)
{
    QLabel *infoLabel = new QLabel(tr("Are you sure to delete notebook <span style=\"%1\">%2</span>?")
                                     .arg(g_config->c_dataTextStyle).arg(p_name));
    m_warningLabel = new QLabel();
    m_warningLabel->setWordWrap(true);

    m_deleteCheck = new QCheckBox(tr("Delete files from disk"), this);
    m_deleteCheck->setChecked(false);
    m_deleteCheck->setToolTip(tr("When checked, VNote will delete all files (including Recycle Bin) of this notebook from disk"));
    connect(m_deleteCheck, &QCheckBox::stateChanged,
            this, &VDeleteNotebookDialog::deleteCheckChanged);

    deleteCheckChanged(false);

    // Ok is the default button.
    m_btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QPushButton *okBtn = m_btnBox->button(QDialogButtonBox::Ok);
    okBtn->setProperty("DangerBtn", true);

    // Standard Warning icon.
    QLabel *iconLabel = new QLabel();
    QPixmap pixmap = standardIcon(QMessageBox::Warning);
    if (pixmap.isNull()) {
        iconLabel->hide();
    } else {
        iconLabel->setPixmap(pixmap);
    }

    QVBoxLayout *iconLayout = new QVBoxLayout();
    iconLayout->addStretch();
    iconLayout->addWidget(iconLabel);
    iconLayout->addStretch();

    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->addWidget(infoLabel);
    infoLayout->addWidget(m_deleteCheck);
    infoLayout->addWidget(m_warningLabel);

    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addLayout(iconLayout);
    topLayout->addLayout(infoLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_btnBox);

    setLayout(mainLayout);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    setWindowTitle(p_title);
}

bool VDeleteNotebookDialog::getDeleteFiles() const
{
    return m_deleteCheck->isChecked();
}

QPixmap VDeleteNotebookDialog::standardIcon(QMessageBox::Icon p_icon)
{
    QStyle *style = this->style();
    int iconSize = style->pixelMetric(QStyle::PM_MessageBoxIconSize, 0, this);
    QIcon tmpIcon;
    switch (p_icon) {
    case QMessageBox::Information:
        tmpIcon = style->standardIcon(QStyle::SP_MessageBoxInformation, 0, this);
        break;
    case QMessageBox::Warning:
        tmpIcon = style->standardIcon(QStyle::SP_MessageBoxWarning, 0, this);
        break;
    case QMessageBox::Critical:
        tmpIcon = style->standardIcon(QStyle::SP_MessageBoxCritical, 0, this);
        break;
    case QMessageBox::Question:
        tmpIcon = style->standardIcon(QStyle::SP_MessageBoxQuestion, 0, this);
        break;
    default:
        break;
    }

    if (!tmpIcon.isNull()) {
        QWindow *window = this->windowHandle();
        if (!window) {
            if (const QWidget *nativeParent = this->nativeParentWidget()) {
                window = nativeParent->windowHandle();
            }
        }
        return tmpIcon.pixmap(window, QSize(iconSize, iconSize));
    }
    return QPixmap();
}

void VDeleteNotebookDialog::deleteCheckChanged(int p_state)
{
    if (!p_state) {
        m_warningLabel->setText(tr("VNote won't delete files in directory <span style=\"%1\">%2</span>.")
                                  .arg(g_config->c_dataTextStyle).arg(m_notebook->getPath()));
    } else {
        m_warningLabel->setText(tr("<span style=\"%1\">WARNING</span>: "
                                   "VNote may delete <b>ANY</b> files in directory <span style=\"%2\">%3</span> "
                                   "and directory <span style=\"%2\">%4</span>!<br>"
                                   "VNote will try to delete all the root folders within this notebook one by one.<br>"
                                   "It may be UNRECOVERABLE!")
                                  .arg(g_config->c_warningTextStyle)
                                  .arg(g_config->c_dataTextStyle)
                                  .arg(m_notebook->getPath())
                                  .arg(m_notebook->getRecycleBinFolderPath()));
    }
}
