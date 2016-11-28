/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: James Henstridge <james.henstridge@canonical.com>
 */

#pragma once

#include <QIODevice>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QXmlInputSource>
#include <QXmlReader>

#include <memory>
#include <vector>

struct MultiStatusProperty {
    QString ns;
    QString name;
    QString value;
    int status;
    QString responsedescription;
};

class MultiStatusParser : public QObject
{
    Q_OBJECT
public:
    MultiStatusParser(QUrl const& base_url, QIODevice* input);
    virtual ~MultiStatusParser();

    void startParsing();
    QString const& errorString() const;

Q_SIGNALS:
    void response(QUrl const& href, std::vector<MultiStatusProperty> const& properties, int status);
    void finished();

private Q_SLOTS:
    void onReadyRead();
    void onReadChannelFinished();

private:
    class Handler;
    friend class Handler;

    QUrl const base_url_;

    // These two represent the same input: we need to keep the
    // QIODevice around to access bytesAvailable() method.
    QIODevice* const input_;
    QXmlInputSource xmlinput_;

    QXmlSimpleReader reader_;
    std::unique_ptr<Handler> handler_;
    QString error_string_;
    bool started_ = false;
    bool finished_ = false;
};

Q_DECLARE_METATYPE(MultiStatusProperty)
Q_DECLARE_METATYPE(std::vector<MultiStatusProperty>)
