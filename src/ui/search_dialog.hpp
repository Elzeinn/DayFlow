#pragma once

#include <QDialog>

#include <cstdint>

class QLineEdit;
class QListWidget;

namespace dayflow {

struct AppContext;

enum class SearchKind {
    task = 0,
    note = 1,
    event = 2,
};

struct SearchChoice {
    SearchKind kind = SearchKind::task;
    std::int64_t id = 0;
};

class SearchDialog : public QDialog {
public:
    explicit SearchDialog(const AppContext& context, QWidget* parent = nullptr);

    SearchChoice choice() const { return choice_; }

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void refresh(const QString& query);
    void activate();

    const AppContext& context_;
    QLineEdit* field_ = nullptr;
    QListWidget* results_ = nullptr;
    SearchChoice choice_;
};

}  // namespace dayflow
