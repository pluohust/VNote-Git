#include "vsnippet.h"

#include <QObject>
#include <QDebug>

#include "vconstants.h"
#include "utils/vutils.h"
#include "utils/veditutils.h"
#include "utils/vmetawordmanager.h"

extern VMetaWordManager *g_mwMgr;

const QString VSnippet::c_defaultCursorMark = "@@";

const QString VSnippet::c_defaultSelectionMark = "$$";

QVector<QChar> VSnippet::s_allShortcuts;

VSnippet::VSnippet()
    : m_type(Type::PlainText),
      m_cursorMark(c_defaultCursorMark),
      m_autoIndent(false)
{
}

VSnippet::VSnippet(const QString &p_name,
                   Type p_type,
                   const QString &p_content,
                   const QString &p_cursorMark,
                   const QString &p_selectionMark,
                   QChar p_shortcut,
                   bool p_autoIndent)
    : m_name(p_name),
      m_type(p_type),
      m_content(p_content),
      m_cursorMark(p_cursorMark),
      m_selectionMark(p_selectionMark),
      m_shortcut(p_shortcut),
      m_autoIndent(p_autoIndent)
{
    Q_ASSERT(m_selectionMark != m_cursorMark);
}

bool VSnippet::update(const QString &p_name,
                      Type p_type,
                      const QString &p_content,
                      const QString &p_cursorMark,
                      const QString &p_selectionMark,
                      QChar p_shortcut,
                      bool p_autoIndent)
{
    bool updated = false;
    if (m_name != p_name) {
        m_name = p_name;
        updated = true;
    }

    if (m_type != p_type) {
        m_type = p_type;
        updated = true;
    }

    if (m_content != p_content) {
        m_content = p_content;
        updated = true;
    }

    if (m_cursorMark != p_cursorMark) {
        m_cursorMark = p_cursorMark;
        updated = true;
    }

    if (m_selectionMark != p_selectionMark) {
        m_selectionMark = p_selectionMark;
        updated = true;
    }

    if (m_shortcut != p_shortcut) {
        m_shortcut = p_shortcut;
        updated = true;
    }

    if (m_autoIndent != p_autoIndent) {
        m_autoIndent = p_autoIndent;
        updated = true;
    }

    qDebug() << "snippet" << m_name << "updated" << updated;

    return updated;
}

QString VSnippet::typeStr(VSnippet::Type p_type)
{
    switch (p_type) {
    case Type::PlainText:
        return QObject::tr("PlainText");

    case Type::Html:
        return QObject::tr("Html");

    default:
        return QObject::tr("Invalid");
    }
}

QJsonObject VSnippet::toJson() const
{
    QJsonObject snip;
    snip[SnippetConfig::c_name] = m_name;
    snip[SnippetConfig::c_type] = (int)m_type;
    snip[SnippetConfig::c_cursorMark] = m_cursorMark;
    snip[SnippetConfig::c_selectionMark] = m_selectionMark;
    snip[SnippetConfig::c_shortcut] = m_shortcut.isNull() ? "" : QString(m_shortcut);
    snip[SnippetConfig::c_autoIndent] = m_autoIndent;

    return snip;
}

VSnippet VSnippet::fromJson(const QJsonObject &p_json)
{
    QChar shortcut;
    QString shortcutStr = p_json[SnippetConfig::c_shortcut].toString();
    if (!shortcutStr.isEmpty() && isValidShortcut(shortcutStr[0])) {
        shortcut = shortcutStr[0];
    }

    VSnippet snip(p_json[SnippetConfig::c_name].toString(),
                  static_cast<VSnippet::Type>(p_json[SnippetConfig::c_type].toInt()),
                  "",
                  p_json[SnippetConfig::c_cursorMark].toString(),
                  p_json[SnippetConfig::c_selectionMark].toString(),
                  shortcut,
                  p_json[SnippetConfig::c_autoIndent].toBool());

    return snip;
}

const QVector<QChar> &VSnippet::getAllShortcuts()
{
    if (s_allShortcuts.isEmpty()) {
        // Init.
        char ch = 'a';
        while (true) {
            s_allShortcuts.append(ch);
            if (ch == 'z') {
                break;
            }

            ch++;
        }
    }

    return s_allShortcuts;
}

bool VSnippet::isValidShortcut(QChar p_char)
{
    if (p_char >= 'a' && p_char <= 'z') {
        return true;
    }

    return false;
}

bool VSnippet::apply(QTextCursor &p_cursor) const
{
    p_cursor.beginEditBlock();
    // Delete selected text.
    QString selection = VEditUtils::selectedText(p_cursor);
    p_cursor.removeSelectedText();

    // Evaluate the content.
    QString content = g_mwMgr->evaluate(m_content);

    if (content.isEmpty()) {
        p_cursor.endEditBlock();
        return true;
    }

    // Find the cursor mark and break the content.
    QString secondPart;
    if (!m_cursorMark.isEmpty()) {
        QStringList parts = content.split(m_cursorMark);
        Q_ASSERT(parts.size() < 3 && parts.size() > 0);

        content = parts[0];
        if (parts.size() == 2) {
            secondPart = parts[1];
        }
    }

    // Replace the selection mark.
    // Content may be empty.
    if (!m_selectionMark.isEmpty() && !content.isEmpty()) {
        content.replace(m_selectionMark, selection);
    }

    int pos = p_cursor.position() + content.size();

    if (!m_selectionMark.isEmpty() && !secondPart.isEmpty()) {
        secondPart.replace(m_selectionMark, selection);
    }

    content += secondPart;

    // Auto indent.
    QTextBlock startBlock = p_cursor.block();
    QTextBlock endBlock;
    QString indentation = VEditUtils::fetchIndentSpaces(startBlock);

    // Insert it.
    switch (m_type) {
    case Type::Html:
        p_cursor.insertHtml(content);
        break;

    case Type::PlainText:
        V_FALLTHROUGH;

    default:
        p_cursor.insertText(content);
        break;
    }

    endBlock = p_cursor.block();

    if (m_autoIndent
        && startBlock != endBlock
        && !indentation.isEmpty()) {
        // Indent (startBlock, endBlock].
        startBlock = startBlock.next();
        while (startBlock.isValid()) {
            int tmpPos = startBlock.position();
            p_cursor.setPosition(tmpPos);
            VEditUtils::indentBlock(p_cursor, indentation, false);
            if (tmpPos <= pos) {
                pos += indentation.size();
            }

            if (startBlock == endBlock) {
                break;
            }

            startBlock = startBlock.next();
        }
    }

    p_cursor.setPosition(pos);

    p_cursor.endEditBlock();
    return true;
}
