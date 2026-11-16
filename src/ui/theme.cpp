#include "ui/theme.hpp"

#include <QFile>
#include <QLabel>

namespace dayflow::theme {

QString style_sheet()
{
    QFile file{QStringLiteral(":/dayflow/theme.qss")};
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

QFont micro_font()
{
    QFont font{QStringLiteral("Segoe UI")};
    font.setPixelSize(10);
    font.setWeight(QFont::DemiBold);
    font.setLetterSpacing(QFont::AbsoluteSpacing, 1.4);
    return font;
}

QFont display_font()
{
    QFont font{QStringLiteral("Segoe UI")};
    font.setPixelSize(26);
    font.setWeight(QFont::Bold);
    font.setItalic(true);
    return font;
}

QFont dialog_title_font()
{
    QFont font{QStringLiteral("Segoe UI")};
    font.setPixelSize(20);
    font.setWeight(QFont::Bold);
    font.setItalic(true);
    return font;
}

QLabel* make_micro_label(const QString& text)
{
    auto* label = new QLabel{text};
    label->setObjectName(QStringLiteral("microLabel"));
    label->setFont(micro_font());
    return label;
}

}  // namespace dayflow::theme
