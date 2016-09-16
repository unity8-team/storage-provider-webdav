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
    QString status;
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
    void response(QString const& href, std::vector<MultiStatusProperty> const& properties, QString const& status);
    void finished();

private Q_SLOTS:
    void onReadyRead();
    void onReadChannelFinished();

private:
    class Handler;
    friend class Handler;

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
