#pragma once

#include <QProcess>
#include <QString>
#include <QUrl>

class DavEnvironment {
public:
    DavEnvironment(QString const& base_dir);
    ~DavEnvironment();

    QUrl const& base_url() const;

private:
    QProcess server_;
    QUrl base_url_;
};
