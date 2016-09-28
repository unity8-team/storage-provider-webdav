#include "MultiStatusParser.h"

#include <QDebug>
#include <QRegularExpression>

#include <cassert>

using namespace std;

namespace
{

char const DAV_NS[] = "DAV:";

enum class ParseState {
    start,           // Starting state
    multistatus,     // Inside <D:multistatus>
    response,        // Inside <D:response>
    href,            // Inside <D:href>
    propstat,        // Inside <D:propstat>
    prop,            // Inside <D:prop>
    property,        // Inside a property
    propstat_status, // Inside <D:status> within <D:propstat>
    response_status, // Inside <D:status> within <D:response>
};

}

class MultiStatusParser::Handler : public QXmlDefaultHandler
{
public:
    Handler(MultiStatusParser *parser) : parser_(parser) {}

    bool atEnd() const { return at_end_; }

    bool endDocument() override;
    bool startElement(QString const& namespace_uri, QString const& local_name,
                      QString const& qname, QXmlAttributes const& attrs) override;
    bool endElement(QString const& namespace_uri, QString const& local_name,
                    QString const& qname) override;
    bool characters(QString const& data) override;

    bool error(QXmlParseException const& exc) override;
    bool fatalError(QXmlParseException const& exc) override;

private:
    MultiStatusParser* const parser_;

    // Current state
    ParseState state_ = ParseState::start;
    int unknown_depth_ = 0;
    bool at_end_ = false;

    QString char_data_;
    QUrl current_href_;
    int current_response_status_;
    // All properties for the given href
    vector<MultiStatusProperty> current_properties_;
    // Properties within the current <D:propstat>
    vector<MultiStatusProperty> current_propstat_;
    int current_propstat_status_;
    // The property currently being processed
    QString current_prop_namespace_;
    QString current_prop_name_;
};


MultiStatusParser::MultiStatusParser(QUrl const& base_url, QIODevice* input)
    : base_url_(base_url), input_(input), xmlinput_(input),
      handler_(new MultiStatusParser::Handler(this))
{
    assert(input->isReadable());

    connect(input, &QIODevice::readyRead,
            this, &MultiStatusParser::onReadyRead);
    connect(input, &QIODevice::readChannelFinished,
            this, &MultiStatusParser::onReadChannelFinished);
    reader_.setContentHandler(handler_.get());
    reader_.setErrorHandler(handler_.get());
}

MultiStatusParser::~MultiStatusParser() = default;

void MultiStatusParser::startParsing()
{
    if (input_->bytesAvailable() > 0)
    {
        onReadyRead();
    }
}

QString const& MultiStatusParser::errorString() const
{
    return error_string_;
}

void MultiStatusParser::onReadyRead()
{
    if (finished_)
    {
        return;
    }
    bool ok;
    if (!started_)
    {
        started_ = true;
        ok = reader_.parse(&xmlinput_, true);
    }
    else
    {
        ok = reader_.parseContinue();
    }
    if (!ok || !error_string_.isEmpty())
    {
        finished_ = true;
        Q_EMIT finished();
    }
}

void MultiStatusParser::onReadChannelFinished()
{
    if (input_->bytesAvailable() > 0)
    {
        onReadyRead();
    }
    if (finished_)
    {
        return;
    }
    // Perform one final call to parseContinue() to signal end of file
    if (started_)
    {
        reader_.parseContinue();
    }
    if (error_string_.isEmpty() && !handler_->atEnd())
    {
        error_string_ = "Unexpectedly reached end of input";
    }
    finished_ = true;
    Q_EMIT finished();
}

bool MultiStatusParser::Handler::endDocument()
{
    at_end_ = true;
    return true;
}


bool MultiStatusParser::Handler::startElement(QString const& namespace_uri,
                                              QString const& local_name,
                                              QString const& qname,
                                              QXmlAttributes const& attrs)
{
    Q_UNUSED(qname);
    Q_UNUSED(attrs);

    // Are we processing an unknown element?
    if (unknown_depth_ > 0)
    {
        unknown_depth_++;
        return true;
    }
    switch (state_)
    {
    case ParseState::start:
        if (namespace_uri == DAV_NS && local_name == "multistatus")
        {
            state_ = ParseState::multistatus;
        }
        else
        {
            unknown_depth_++;
        }
        break;
    case ParseState::multistatus:
        if (namespace_uri == DAV_NS && local_name == "response")
        {
            state_ = ParseState::response;
            current_href_.clear();
            current_response_status_ = 0;
            current_properties_.clear();
        }
        else
        {
            unknown_depth_++;
        }
        break;
    case ParseState::response:
        if (namespace_uri == DAV_NS)
        {
            if (local_name == "href")
            {
                state_ = ParseState::href;
                current_href_.clear();
                char_data_.clear();
            }
            else if (local_name == "status")
            {
                state_ = ParseState::response_status;
                current_response_status_ = 0;
                char_data_.clear();
            }
            else if (local_name == "propstat")
            {
                state_ = ParseState::propstat;
                current_propstat_.clear();
                current_propstat_status_ = 0;
            }
            else
            {
                unknown_depth_++;
            }
        }
        else
        {
            unknown_depth_++;
        }
        break;
    case ParseState::href:
        unknown_depth_++;
        break;
    case ParseState::propstat:
        if (namespace_uri == DAV_NS)
        {
            if (local_name == "prop")
            {
                state_ = ParseState::prop;
            }
            else if (local_name == "status")
            {
                state_ = ParseState::propstat_status;
                current_propstat_status_ = 0;
                char_data_.clear();
            }
            else
            {
                unknown_depth_++;
            }
        }
        else
        {
            unknown_depth_++;
        }
        break;
    case ParseState::prop:
        // Any element at this level represents a property
        state_ = ParseState::property;
        current_prop_namespace_ = namespace_uri;
        current_prop_name_ = local_name;
        char_data_.clear();
        break;
    case ParseState::property:
        // We don't handle extra elements within a property, but need
        // to handle DAV:resourcetype, so special case DAV:collection
        // here.
        if (namespace_uri == DAV_NS && local_name == "collection")
        {
            char_data_ += "DAV:collection";
        }
        unknown_depth_++;
        break;
    case ParseState::propstat_status:
        unknown_depth_++;
        break;
    case ParseState::response_status:
        unknown_depth_++;
        break;
    }
    return true;
}

bool MultiStatusParser::Handler::endElement(QString const& namespace_uri,
                                            QString const& local_name,
                                            QString const& qname)
{
    Q_UNUSED(namespace_uri);
    Q_UNUSED(local_name);
    Q_UNUSED(qname);

    // Are we processing an unknown element?
    if (unknown_depth_ > 0)
    {
        unknown_depth_--;
        return true;
    }

    static QRegularExpression const http_status(
        R"(^HTTP/\d+\.\d+ (\d\d\d) )");
    QRegularExpressionMatch match;

    switch (state_)
    {
    case ParseState::start:
        // We should never see an endElement callback in this state.
        assert(false);
        break;
    case ParseState::multistatus:
        state_ = ParseState::start;
        break;
    case ParseState::response:
        Q_EMIT parser_->response(current_href_, current_properties_,
                                 current_response_status_);
        state_ = ParseState::multistatus;
        break;
    case ParseState::href:
    {
        QUrl relative(char_data_.trimmed(), QUrl::StrictMode);
        if (!relative.isValid()) {
            parser_->error_string_ = QStringLiteral("Invalid URL: ") + char_data_;
            return false;
        }
        current_href_ = parser_->base_url_.resolved(relative);
        state_ = ParseState::response;
        break;
    }
    case ParseState::propstat:
        for (auto& prop : current_propstat_)
        {
            prop.status = current_propstat_status_;
            current_properties_.emplace_back(move(prop));
        }
        current_propstat_.clear();
        state_ = ParseState::response;
        break;
    case ParseState::prop:
        state_ = ParseState::propstat;
        break;
    case ParseState::property:
        current_propstat_.push_back({
                current_prop_namespace_,
                current_prop_name_,
                char_data_.trimmed(),
                0,
                QString()
            });
        state_ = ParseState::prop;
        break;
    case ParseState::propstat_status:
        match = http_status.match(char_data_.trimmed());
        if (match.hasMatch())
        {
            current_propstat_status_ = match.captured(1).toInt();
        }
        state_ = ParseState::propstat;
        break;
    case ParseState::response_status:
        match = http_status.match(char_data_.trimmed());
        if (match.hasMatch())
        {
            current_response_status_ = match.captured(1).toInt();
        }
        state_ = ParseState::response;
        break;
    }
    return true;
}

bool MultiStatusParser::Handler::characters(QString const& data)
{
    if (unknown_depth_ > 0)
    {
        return true;
    }
    // We only collect character data in certain states
    switch (state_)
    {
    case ParseState::href:
    case ParseState::property:
    case ParseState::propstat_status:
    case ParseState::response_status:
        char_data_ += data;
        break;
    default:
        break;
    }
    return true;
}

bool MultiStatusParser::Handler::error(QXmlParseException const& exc)
{
    qWarning() << "XML error on line" << exc.lineNumber()
               << ":" << exc.message();
    return true;
}

bool MultiStatusParser::Handler::fatalError(QXmlParseException const& exc)
{
    parser_->error_string_ = exc.message();
    return false;
}
