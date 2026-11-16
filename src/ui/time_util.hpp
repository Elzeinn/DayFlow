#pragma once

#include <QDate>
#include <QDateTime>

#include <chrono>

#include "core/timestamp.hpp"

namespace dayflow {

inline Timestamp utc_from_local(const QDateTime& local)
{
    return Timestamp{std::chrono::seconds{local.toSecsSinceEpoch()}};
}

inline QDateTime local_from_utc(Timestamp value)
{
    return QDateTime::fromSecsSinceEpoch(value.time_since_epoch().count());
}

}  // namespace dayflow
