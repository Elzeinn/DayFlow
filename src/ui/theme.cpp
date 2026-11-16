#include "ui/theme.hpp"

#include <QFile>

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

}  // namespace dayflow::theme
