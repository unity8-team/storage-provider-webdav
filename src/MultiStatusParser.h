#pragma once

#include <QIODevice>
#include <QObject>
#include <QString>
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
    MultiStatusParser(QIODevice* input);
    virtual ~MultiStatusParser();

    void startParsing();
    QString const& errorString() const;

Q_SIGNALS:
    void response(QString const& href, std::vector<MultiStatusProperty> const& properties, int status);
    void finished();

private Q_SLOTS:
    void onReadyRead();
    void onReadChannelFinished();

private:
    class Handler;
    friend class Handler;

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
