#include <QtWidgets>
#include "vmdedit.h"
#include "pegmarkdownhighlighter.h"
#include "vmdeditoperations.h"
#include "vnote.h"
#include "vconfigmanager.h"
#include "vtableofcontent.h"
#include "utils/vutils.h"
#include "utils/veditutils.h"
#include "utils/vpreviewutils.h"
#include "dialog/vselectdialog.h"
#include "dialog/vconfirmdeletiondialog.h"
#include "vtextblockdata.h"
#include "vorphanfile.h"

extern VConfigManager *g_config;
extern VNote *g_vnote;

const int VMdEdit::c_numberOfAysncJobs = 2;

VMdEdit::VMdEdit(VFile *p_file, VDocument *p_vdoc, MarkdownConverterType p_type,
                 QWidget *p_parent)
    : VEdit(p_file, p_parent), m_mdHighlighter(NULL), m_freshEdit(true),
      m_finishedAsyncJobs(c_numberOfAysncJobs)
{
    Q_UNUSED(p_type);
    Q_UNUSED(p_vdoc);

    V_ASSERT(p_file->getDocType() == DocType::Markdown);

    setAcceptRichText(false);

    /*
    m_mdHighlighter = new HGMarkdownHighlighter(g_config->getMdHighlightingStyles(),
                                                g_config->getCodeBlockStyles(),
                                                g_config->getMarkdownHighlightInterval(),
                                                document());

    connect(m_mdHighlighter, &HGMarkdownHighlighter::headersUpdated,
            this, &VMdEdit::updateHeaders);
    // After highlight, the cursor may trun into non-visible. We should make it visible
    // in this case.
    connect(m_mdHighlighter, &HGMarkdownHighlighter::highlightCompleted,
            this, [this]() {
            makeBlockVisible(textCursor().block());
    });
    */

    /*
    m_imagePreviewer = new VImagePreviewer(this, m_mdHighlighter);
    connect(m_mdHighlighter, &HGMarkdownHighlighter::imageLinksUpdated,
            m_imagePreviewer, &VImagePreviewer::imageLinksChanged);
    connect(m_imagePreviewer, &VImagePreviewer::requestUpdateImageLinks,
            m_mdHighlighter, &HGMarkdownHighlighter::updateHighlight);
    connect(m_imagePreviewer, &VImagePreviewer::previewFinished,
            this, [this](){
                if (m_freshEdit) {
                    finishOneAsyncJob(0);
                }
            });

    connect(m_imagePreviewer, &VImagePreviewer::previewWidthUpdated,
            this, [this](){
                if (m_freshEdit) {
                    finishOneAsyncJob(1);
                }
            });
    */

    // Comment out these lines since we use VMdEditor to replace VMdEdit.
    /*
    m_editOps = new VMdEditOperations(this, m_file);

    connect(m_editOps, &VEditOperations::statusMessage,
            this, &VEdit::statusMessage);
    connect(m_editOps, &VEditOperations::vimStatusUpdated,
            this, &VEdit::vimStatusUpdated);

    connect(this, &VMdEdit::cursorPositionChanged,
            this, &VMdEdit::updateCurrentHeader);

    connect(QApplication::clipboard(), &QClipboard::changed,
            this, &VMdEdit::handleClipboardChanged);
    */

    updateFontAndPalette();

    updateConfig();
}

void VMdEdit::updateFontAndPalette()
{
    setFont(g_config->getMdEditFont());
    setPalette(g_config->getMdEditPalette());
}

void VMdEdit::beginEdit()
{
    updateFontAndPalette();

    updateConfig();

    Q_ASSERT(m_file->getContent() == toPlainTextWithoutImg());

    initInitImages();

    setModified(false);

    if (m_freshEdit) {
        // Will set to false when all async jobs completed.
        setReadOnlyAndHighlight(true);
        // Disable and clear undo stacks temporary.
        setUndoRedoEnabled(false);
    } else {
        setReadOnlyAndHighlight(false);
    }

    updateHeaders(m_mdHighlighter->getHeaderRegions());
}

void VMdEdit::endEdit()
{
    setReadOnlyAndHighlight(true);
    clearUnusedImages();
}

void VMdEdit::saveFile()
{
    Q_ASSERT(m_file->isModifiable());

    if (!document()->isModified()) {
        return;
    }

    m_file->setContent(toPlainTextWithoutImg());
    setModified(false);
}

void VMdEdit::reloadFile()
{
    const QString &content = m_file->getContent();
    Q_ASSERT(content.indexOf(QChar::ObjectReplacementCharacter) == -1);

    setPlainText(content);

    setModified(false);
}

void VMdEdit::keyPressEvent(QKeyEvent *event)
{
    if (m_editOps->handleKeyPressEvent(event)) {
        return;
    }
    VEdit::keyPressEvent(event);
}

bool VMdEdit::canInsertFromMimeData(const QMimeData *source) const
{
    return source->hasImage() || source->hasUrls()
           || VEdit::canInsertFromMimeData(source);
}

void VMdEdit::insertFromMimeData(const QMimeData *source)
{
    VSelectDialog dialog(tr("Insert From Clipboard"), this);
    dialog.addSelection(tr("Insert As Image"), 0);
    dialog.addSelection(tr("Insert As Text"), 1);

    if (source->hasImage()) {
        // Image data in the clipboard
        if (source->hasText()) {
            if (dialog.exec() == QDialog::Accepted) {
                if (dialog.getSelection() == 1) {
                    // Insert as text.
                    Q_ASSERT(source->hasText() && source->hasImage());
                    VEdit::insertFromMimeData(source);
                    return;
                }
            } else {
                return;
            }
        }
        m_editOps->insertImageFromMimeData(source);
        return;
    } else if (source->hasUrls()) {
        QList<QUrl> urls = source->urls();
        if (urls.size() == 1 && VUtils::isImageURL(urls[0])) {
            if (dialog.exec() == QDialog::Accepted) {
                // FIXME: After calling dialog.exec(), source->hasUrl() returns false.
                if (dialog.getSelection() == 0) {
                    // Insert as image.
                    m_editOps->insertImageFromURL(urls[0]);
                    return;
                }
                QMimeData newSource;
                newSource.setUrls(urls);
                VEdit::insertFromMimeData(&newSource);
                return;
            } else {
                return;
            }
        }
    } else if (source->hasText()) {
        QString text = source->text();
        if (VUtils::isImageURLText(text)) {
            // The text is a URL to an image.
            if (dialog.exec() == QDialog::Accepted) {
                if (dialog.getSelection() == 0) {
                    // Insert as image.
                    QUrl url(text);
                    if (url.isValid()) {
                        m_editOps->insertImageFromURL(QUrl(text));
                    }
                    return;
                }
            } else {
                return;
            }
        }
        Q_ASSERT(source->hasText());
    }
    VEdit::insertFromMimeData(source);
}

void VMdEdit::imageInserted(const QString &p_path)
{
    ImageLink link;
    link.m_path = p_path;
    if (m_file->useRelativeImageFolder()) {
        link.m_type = ImageLink::LocalRelativeInternal;
    } else {
        link.m_type = ImageLink::LocalAbsolute;
    }

    m_insertedImages.append(link);
}

void VMdEdit::initInitImages()
{
    m_initImages = VUtils::fetchImagesFromMarkdownFile(m_file,
                                                       ImageLink::LocalRelativeInternal);
}

void VMdEdit::clearUnusedImages()
{
    QVector<ImageLink> images = VUtils::fetchImagesFromMarkdownFile(m_file,
                                                                    ImageLink::LocalRelativeInternal);

    QVector<QString> unusedImages;

    if (!m_insertedImages.isEmpty()) {
        for (int i = 0; i < m_insertedImages.size(); ++i) {
            const ImageLink &link = m_insertedImages[i];

            if (link.m_type != ImageLink::LocalRelativeInternal) {
                continue;
            }

            int j;
            for (j = 0; j < images.size(); ++j) {
                if (VUtils::equalPath(link.m_path, images[j].m_path)) {
                    break;
                }
            }

            // This inserted image is no longer in the file.
            if (j == images.size()) {
                unusedImages.push_back(link.m_path);
            }
        }

        m_insertedImages.clear();
    }

    for (int i = 0; i < m_initImages.size(); ++i) {
        const ImageLink &link = m_initImages[i];

        V_ASSERT(link.m_type == ImageLink::LocalRelativeInternal);

        int j;
        for (j = 0; j < images.size(); ++j) {
            if (VUtils::equalPath(link.m_path, images[j].m_path)) {
                break;
            }
        }

        // Original local relative image is no longer in the file.
        if (j == images.size()) {
            unusedImages.push_back(link.m_path);
        }
    }

    if (!unusedImages.isEmpty()) {
        if (g_config->getConfirmImagesCleanUp()) {
            QVector<ConfirmItemInfo> items;
            for (auto const & img : unusedImages) {
                items.push_back(ConfirmItemInfo(img,
                                                img,
                                                img,
                                                NULL));

            }

            QString text = tr("Following images seems not to be used in this note anymore. "
                              "Please confirm the deletion of these images.");

            QString info = tr("Deleted files could be found in the recycle "
                              "bin of this note.<br>"
                              "Click \"Cancel\" to leave them untouched.");

            VConfirmDeletionDialog dialog(tr("Confirm Cleaning Up Unused Images"),
                                          text,
                                          info,
                                          items,
                                          true,
                                          true,
                                          true,
                                          this);

            unusedImages.clear();
            if (dialog.exec()) {
                items = dialog.getConfirmedItems();
                g_config->setConfirmImagesCleanUp(dialog.getAskAgainEnabled());

                for (auto const & item : items) {
                    unusedImages.push_back(item.m_name);
                }
            }
        }

        for (int i = 0; i < unusedImages.size(); ++i) {
            bool ret = false;
            if (m_file->getType() == FileType::Note) {
                const VNoteFile *tmpFile = dynamic_cast<const VNoteFile *>((VFile *)m_file);
                ret = VUtils::deleteFile(tmpFile->getNotebook(), unusedImages[i], false);
            } else if (m_file->getType() == FileType::Orphan) {
                const VOrphanFile *tmpFile = dynamic_cast<const VOrphanFile *>((VFile *)m_file);
                ret = VUtils::deleteFile(tmpFile, unusedImages[i], false);
            } else {
                Q_ASSERT(false);
            }

            if (!ret) {
                qWarning() << "fail to delete unused original image" << unusedImages[i];
            } else {
                qDebug() << "delete unused image" << unusedImages[i];
            }
        }
    }

    m_initImages.clear();
}

void VMdEdit::updateCurrentHeader()
{
    emit currentHeaderChanged(textCursor().block().blockNumber());
}

static void addHeaderSequence(QVector<int> &p_sequence, int p_level, int p_baseLevel)
{
    Q_ASSERT(p_level >= 1 && p_level < p_sequence.size());
    if (p_level < p_baseLevel) {
        p_sequence.fill(0);
        return;
    }

    ++p_sequence[p_level];
    for (int i = p_level + 1; i < p_sequence.size(); ++i) {
        p_sequence[i] = 0;
    }
}

static QString headerSequenceStr(const QVector<int> &p_sequence)
{
    QString res;
    for (int i = 1; i < p_sequence.size(); ++i) {
        if (p_sequence[i] != 0) {
            res = res + QString::number(p_sequence[i]) + '.';
        } else if (res.isEmpty()) {
            continue;
        } else {
            break;
        }
    }

    return res;
}

static void insertSequenceToHeader(QTextBlock p_block,
                                   QRegExp &p_reg,
                                   QRegExp &p_preReg,
                                   const QString &p_seq)
{
    if (!p_block.isValid()) {
        return;
    }

    QString text = p_block.text();
    bool matched = p_reg.exactMatch(text);
    Q_ASSERT(matched);

    matched = p_preReg.exactMatch(text);
    Q_ASSERT(matched);

    int start = p_reg.cap(1).length() + 1;
    int end = p_preReg.cap(1).length();

    Q_ASSERT(start <= end);

    QTextCursor cursor(p_block);
    cursor.setPosition(p_block.position() + start);
    if (start != end) {
        cursor.setPosition(p_block.position() + end, QTextCursor::KeepAnchor);
    }

    if (p_seq.isEmpty()) {
        cursor.removeSelectedText();
    } else {
        cursor.insertText(p_seq + ' ');
    }
}

void VMdEdit::updateHeaders(const QVector<VElementRegion> &p_headerRegions)
{
    QTextDocument *doc = document();

    QVector<VTableOfContentItem> headers;
    QVector<int> headerBlockNumbers;
    QVector<QString> headerSequences;
    if (!p_headerRegions.isEmpty()) {
        headers.reserve(p_headerRegions.size());
        headerBlockNumbers.reserve(p_headerRegions.size());
        headerSequences.reserve(p_headerRegions.size());
    }

    // Assume that each block contains only one line
    // Only support # syntax for now
    QRegExp headerReg(VUtils::c_headerRegExp);
    int baseLevel = -1;
    for (auto const & reg : p_headerRegions) {
        QTextBlock block = doc->findBlock(reg.m_startPos);
        if (!block.isValid()) {
            continue;
        }

        Q_ASSERT(block.lineCount() == 1);

        if (!block.contains(reg.m_endPos - 1)) {
            continue;
        }

        if ((block.userState() == HighlightBlockState::Normal) &&
            headerReg.exactMatch(block.text())) {
            int level = headerReg.cap(1).length();
            VTableOfContentItem header(headerReg.cap(2).trimmed(),
                                       level,
                                       block.blockNumber(),
                                       headers.size());
            headers.append(header);
            headerBlockNumbers.append(block.blockNumber());
            headerSequences.append(headerReg.cap(3));

            if (baseLevel == -1) {
                baseLevel = level;
            } else if (baseLevel > level) {
                baseLevel = level;
            }
        }
    }

    m_headers.clear();

    bool autoSequence = m_config.m_enableHeadingSequence
                        && !isReadOnly()
                        && m_file->isModifiable();
    int headingSequenceBaseLevel = g_config->getHeadingSequenceBaseLevel();
    if (headingSequenceBaseLevel < 1 || headingSequenceBaseLevel > 6) {
        headingSequenceBaseLevel = 1;
    }

    QVector<int> seqs(7, 0);
    QRegExp preReg(VUtils::c_headerPrefixRegExp);
    int curLevel = baseLevel - 1;
    for (int i = 0; i < headers.size(); ++i) {
        VTableOfContentItem &item = headers[i];
        while (item.m_level > curLevel + 1) {
            curLevel += 1;

            // Insert empty level which is an invalid header.
            m_headers.append(VTableOfContentItem(c_emptyHeaderName,
                                                 curLevel,
                                                 -1,
                                                 m_headers.size()));
            if (autoSequence) {
                addHeaderSequence(seqs, curLevel, headingSequenceBaseLevel);
            }
        }

        item.m_index = m_headers.size();
        m_headers.append(item);
        curLevel = item.m_level;
        if (autoSequence) {
            addHeaderSequence(seqs, item.m_level, headingSequenceBaseLevel);

            QString seqStr = headerSequenceStr(seqs);
            if (headerSequences[i] != seqStr) {
                // Insert correct sequence.
                insertSequenceToHeader(doc->findBlockByNumber(headerBlockNumbers[i]),
                                       headerReg,
                                       preReg,
                                       seqStr);
            }
        }
    }

    emit headersChanged(m_headers);

    updateCurrentHeader();
}

bool VMdEdit::scrollToHeader(int p_blockNumber)
{
    if (p_blockNumber < 0) {
        return false;
    }

    return scrollToBlock(p_blockNumber);
}

QString VMdEdit::toPlainTextWithoutImg()
{
    QString text;
    bool readOnly = isReadOnly();
    setReadOnlyAndHighlight(true);
    text = getPlainTextWithoutPreviewImage();
    setReadOnlyAndHighlight(readOnly);

    return text;
}

QString VMdEdit::getPlainTextWithoutPreviewImage() const
{
    QVector<Region> deletions;

    while (true) {
        deletions.clear();

        /*
        while (m_imagePreviewer->isPreviewing()) {
            VUtils::sleepWait(100);
        }
        */

        // Iterate all the block to get positions for deletion.
        // QTextBlock block = document()->begin();
        bool tryAgain = false;
        /*
        while (block.isValid()) {
            if (VTextBlockData::containsPreviewImage(block)) {
                if (!getPreviewImageRegionOfBlock(block, deletions)) {
                    tryAgain = true;
                    break;
                }
            }

            block = block.next();
        }
        */

        if (tryAgain) {
            continue;
        }

        QString text = toPlainText();
        // deletions is sorted by m_startPos.
        // From back to front.
        for (int i = deletions.size() - 1; i >= 0; --i) {
            const Region &reg = deletions[i];
            qDebug() << "img region to delete" << reg.m_startPos << reg.m_endPos;
            text.remove(reg.m_startPos, reg.m_endPos - reg.m_startPos);
        }

        return text;
    }
}

bool VMdEdit::getPreviewImageRegionOfBlock(const QTextBlock &p_block,
                                           QVector<Region> &p_regions) const
{
    QTextDocument *doc = document();
    QVector<Region> regs;
    QString text = p_block.text();
    int nrOtherChar = 0;
    int nrImage = 0;
    bool hasBlock = false;

    // From back to front.
    for (int i = text.size() - 1; i >= 0; --i) {
        if (text[i].isSpace()) {
            continue;
        }

        if (text[i] == QChar::ObjectReplacementCharacter) {
            int pos = p_block.position() + i;
            Q_ASSERT(doc->characterAt(pos) == QChar::ObjectReplacementCharacter);

            QTextImageFormat imageFormat = VPreviewUtils::fetchFormatFromPosition(doc, pos);
            if (imageFormat.isValid()) {
                ++nrImage;
                bool isBlock = VPreviewUtils::getPreviewImageType(imageFormat) == PreviewImageType::Block;
                if (isBlock) {
                    hasBlock = true;
                } else {
                    regs.push_back(Region(pos, pos + 1));
                }
            } else {
                return false;
            }
        } else {
            ++nrOtherChar;
        }
    }

    if (hasBlock) {
        if (nrOtherChar > 0 || nrImage > 1) {
            // Inconsistent state.
            return false;
        }

        regs.push_back(Region(p_block.position(), p_block.position() + p_block.length()));
    }

    p_regions.append(regs);
    return true;
}

void VMdEdit::handleClipboardChanged(QClipboard::Mode p_mode)
{
    if (!hasFocus()) {
        return;
    }

    if (p_mode == QClipboard::Clipboard) {
        QClipboard *clipboard = QApplication::clipboard();
        const QMimeData *mimeData = clipboard->mimeData();
        if (mimeData->hasText()) {
            QString text = mimeData->text();
            if (clipboard->ownsClipboard()) {
                if (text.trimmed() == QString(QChar::ObjectReplacementCharacter)) {
                    QImage image = tryGetSelectedImage();
                    clipboard->clear(QClipboard::Clipboard);
                    if (!image.isNull()) {
                        clipboard->setImage(image, QClipboard::Clipboard);
                    }
                } else {
                    // Try to remove all the preview image in text.
                    VEditUtils::removeObjectReplacementCharacter(text);
                    if (text.size() != mimeData->text().size()) {
                        clipboard->clear(QClipboard::Clipboard);
                        clipboard->setText(text);
                    }
                }
            }
        }
    }
}

QImage VMdEdit::tryGetSelectedImage()
{
    QImage image;
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) {
        return image;
    }
    int start = cursor.selectionStart();
    int end = cursor.selectionEnd();
    QTextDocument *doc = document();
    QTextImageFormat format;
    for (int i = start; i < end; ++i) {
        if (doc->characterAt(i) == QChar::ObjectReplacementCharacter) {
            format = VPreviewUtils::fetchFormatFromPosition(doc, i);
            break;
        }
    }

    if (format.isValid()) {
        PreviewImageSource src = VPreviewUtils::getPreviewImageSource(format);
        // long long id = VPreviewUtils::getPreviewImageID(format);
        if (src == PreviewImageSource::Image) {
            /*
            Q_ASSERT(m_imagePreviewer->isEnabled());
            image = m_imagePreviewer->fetchCachedImageByID(id);
            */
        }
    }

    return image;
}

void VMdEdit::resizeEvent(QResizeEvent *p_event)
{
    /*
    m_imagePreviewer->updatePreviewImageWidth();
    */

    VEdit::resizeEvent(p_event);
}

int VMdEdit::indexOfCurrentHeader() const
{
    if (m_headers.isEmpty()) {
        return -1;
    }

    int blockNumber = textCursor().block().blockNumber();
    for (int i = m_headers.size() - 1; i >= 0; --i) {
        if (!m_headers[i].isEmpty()
            && m_headers[i].m_blockNumber <= blockNumber) {
            return i;
        }
    }

    return -1;
}

bool VMdEdit::jumpTitle(bool p_forward, int p_relativeLevel, int p_repeat)
{
    if (m_headers.isEmpty()) {
        return false;
    }

    QTextCursor cursor = textCursor();
    int cursorLine = cursor.block().blockNumber();
    int targetIdx = -1;
    // -1: skip level check.
    int targetLevel = 0;
    int idx = indexOfCurrentHeader();
    if (idx == -1) {
        // Cursor locates at the beginning, before any headers.
        if (p_relativeLevel < 0 || !p_forward) {
            return false;
        }
    }

    int delta = 1;
    if (!p_forward) {
        delta = -1;
    }

    bool firstHeader = true;
    for (targetIdx = idx == -1 ? 0 : idx;
         targetIdx >= 0 && targetIdx < m_headers.size();
         targetIdx += delta) {
        const VTableOfContentItem &header = m_headers[targetIdx];
        if (header.isEmpty()) {
            continue;
        }

        if (targetLevel == 0) {
            // The target level has not been init yet.
            Q_ASSERT(firstHeader);
            targetLevel = header.m_level;
            if (p_relativeLevel < 0) {
                targetLevel += p_relativeLevel;
                if (targetLevel < 1) {
                    // Invalid level.
                    return false;
                }
            } else if (p_relativeLevel > 0) {
                targetLevel = -1;
            }
        }

        if (targetLevel == -1 || header.m_level == targetLevel) {
            if (firstHeader
                && (cursorLine == header.m_blockNumber
                    || p_forward)
                && idx != -1) {
                // This header is not counted for the repeat.
                firstHeader = false;
                continue;
            }

            if (--p_repeat == 0) {
                // Found.
                break;
            }
        } else if (header.m_level < targetLevel) {
            // Stop by higher level.
            return false;
        }

        firstHeader = false;
    }

    if (targetIdx < 0 || targetIdx >= m_headers.size()) {
        return false;
    }

    // Jump to target header.
    int line = m_headers[targetIdx].m_blockNumber;
    if (line > -1) {
        QTextBlock block = document()->findBlockByNumber(line);
        if (block.isValid()) {
            cursor.setPosition(block.position());
            setTextCursor(cursor);
            return true;
        }
    }

    return false;
}

void VMdEdit::finishOneAsyncJob(int p_idx)
{
    Q_ASSERT(m_freshEdit);
    if (m_finishedAsyncJobs[p_idx]) {
        return;
    }

    m_finishedAsyncJobs[p_idx] = true;
    if (-1 == m_finishedAsyncJobs.indexOf(false)) {
        // All jobs finished.
        setUndoRedoEnabled(true);

        setReadOnlyAndHighlight(false);

        setModified(false);
        m_freshEdit = false;
        emit statusChanged();

        updateHeaders(m_mdHighlighter->getHeaderRegions());

        emit ready();
    }
}
