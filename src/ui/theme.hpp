#pragma once

#include <QFont>
#include <QString>

class QLabel;

namespace dayflow::theme {

QString style_sheet();
QFont micro_font();
QFont display_font();
QFont dialog_title_font();
QLabel* make_micro_label(const QString& text);

}  // namespace dayflow::theme
