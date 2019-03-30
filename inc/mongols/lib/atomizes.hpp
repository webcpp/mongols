/**
 * MIT License
 * 
 * Copyright (c) 2018 Maxwell Flynn
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef ATOMIZES_HPP_
#define ATOMIZES_HPP_

#include <string>
#include <unordered_map>
#include <cstdint>
#include <vector>
#include <sstream>

namespace atomizes
{
    /**
     * HTTP defines that any lines must be seperated with a carriage return.
     * 
     * Define this as a constant to use across the library code and to change easily.
     */
    const std::string CarriageReturn = "\r\n";

    /**
     * The string for HTTP version 1.0
     */
    const std::string HTTPVersion1 = "HTTP/1.0";

    /**
     * The string for HTTP version 1.1
     */
    const std::string HTTPVersion11 = "HTTP/1.1";

    /**
     * An enum of HTTP methods for use in a message.
     * 
     * Set to NONE for use in a response message, as responses do not use methods.
     */
    enum class MessageMethod : uint8_t
    {
        NONE,
        GET,
        HEAD,
        POST,
        PUT,
        DELETE,
        CONNECT,
        TRACE,
        PATCH
    };

    /**
     * Converts a MessageMethod enum to a string for use in a request.
     */
    inline std::string MessageMethodToString(const MessageMethod& method)
    {
        switch (method)
        {
        case MessageMethod::NONE:
            return "NONE";
        case MessageMethod::GET:
            return "GET";
        case MessageMethod::HEAD:
            return "HEAD";
        case MessageMethod::POST:
            return "POST";
        case MessageMethod::PUT:
            return "PUT";
        case MessageMethod::DELETE:
            return "DELETE";
        case MessageMethod::CONNECT:
            return "CONNECT";
        case MessageMethod::TRACE:
            return "TRACE";
        case MessageMethod::PATCH:
            return "PATCH";
		default:
			return "NONE";
        }
    }

    /**
     * Converts a string to a MessageMethod enum for use in a HTTPMessage object.
     */
    inline MessageMethod StringToMessageMethod(const std::string& method)
    {
        if (method == "NONE")
            return MessageMethod::NONE;
        if (method == "GET")
            return MessageMethod::GET;
        if (method == "HEAD")
            return MessageMethod::HEAD;
        if (method == "POST")
            return MessageMethod::POST;
        if (method == "PUT")
            return MessageMethod::PUT;
        if (method == "DELETE")
            return MessageMethod::DELETE;
        if (method == "CONNECT")
            return MessageMethod::CONNECT;
        if (method == "TRACE")
            return MessageMethod::TRACE;
        if (method == "PATCH")
            return MessageMethod::PATCH;

        return MessageMethod::NONE;
    }

    /**
     * To be returned with a status code in a response is a status text describing the
     * status code by text rather than by a code.
     * 
     * This method takes in one of those codes and tries to return a text for it.
     */
    inline std::string StatusTextFromStatusCode(const uint16_t statusCode)
    {
        switch (statusCode)
        {
        case 100:
            return "Continue";
        case 101:
            return "Switching Protocol";
        case 200:
            return "OK";
        case 201:
            return "Created";
        case 202:
            return "Accepted";
        case 203:
            return "Non-Authoritative Information";
        case 204:
            return "No Content";
        case 205:
            return "Reset Content";
        case 206:
            return "Partial Content";
        case 300:
            return "Multiple Choice";
        case 301:
            return "Moved Permanently";
        case 302:
            return "Found";
        case 303:
            return "See Other";
        case 304:
            return "Not Modified";
        // 305 is deprecated and 306 is only reserved, skip
        case 307:
            return "Temporary Redirect";
        case 308:
            return "Permanent Redirect";
        case 400:
            return "Bad Request";
        case 401:
            return "Unauthorized";
        case 402:
            // 402 is reserved for future use but has a status message, adding it
            return "Payment Required";
        case 403:
            return "Forbidden";
        case 404:
            return "Not Found";
        case 405:
            return "Method Not Allowed";
        case 406:
            return "Not Acceptable";
        case 407:
            return "Proxy Authentication Required";
        case 408:
            return "Request Timeout";
        case 409:
            return "Conflict";
        case 410:
            return "Gone";
        case 411:
            return "Length Required";
        case 412:
            return "Precondition Failed";
        case 413:
            return "Payload Too Large";
        case 414:
            return "URI Too Long";
        case 415:
            return "Unsupported Media Type";
        case 416:
            return "Requested Range Not Satisfiable";
        case 417:
            return "Expectation Failed";
        case 418:
            // might as well return the teapot joke
            return "I'm a teapot";
        case 421:
            return "Misdirected Request";
        case 425:
            return "Too Early";
        case 426:
            return "Upgrade Required";
        case 428:
            return "Precondition Required";
        case 429:
            return "Too Many Requests";
        case 431:
            return "Request Header Fields Too Large";
        case 451:
            return "Unavailable for Legal Reasons";
        case 500:
            return "Internal Server Error";
        case 501:
            return "Not Implemented";
        case 502:
            return "Bad Gateway";
        case 503:
            return "Service Unavailable";
        case 504:
            return "Gateway Timeout";
        case 505:
            return "HTTP Version Not Supported";
        case 506:
            return "Variant Also Negotiates";
        case 507:
            return "Insufficient Storage";
        case 510:
            return "Not Extended";
        case 511:
            return "Network Authentication Required";
        default:
            return "Undefined";
        }
    }

    /**
     * The basic class to represent both HTTP requests and responses.
     * 
     * Contains a method for grabbing the message as a string formatted for
     * sending through a server or other means.
     * 
     * Allows for setting a message body that is represented by a vector of
     * 8-bit unsigned integers. This is to allow binary data to be sent through
     * the message. But, you also can use the included methods to send plain strings.
     * 
     * This class returns as many methods as possible with a reference to the current
     * object, to try and allow for chaining methods.
     * 
     * Note: for convenience, the header for Content-Length is automatically included
     * as it is grabbed from the `m_body` member. Though, it is only included if the
     * body member isn't empty.
     */
    class HTTPMessage
    {
    public:
        /**
         * Set a header in the map to the value provided.
         */
        HTTPMessage& SetHeader(const std::string& name, const std::string& value)
        {
            m_headers[name] = value;

            return *this;
        }

        /**
         * Set a number of headers based on a generic map of keys and values.
         */
        HTTPMessage& SetHeaders(const std::unordered_map<std::string, std::string>& headers)
        {
            m_headers.insert(headers.begin(), headers.end());
        
            return *this;
        }

        /**
         * Get the string value of a single header from the message.
         * 
         * Will return an empty string if the header does not exist.
         */
        inline std::string GetHeader(const std::string& name) const
        {
            auto find = m_headers.find(name);

            if (find != m_headers.end())
                return find->second;
            
            return "";
        }

        /**
         * Set the associated message method for this message.
         * 
         * Use `NONE` to switch this into a response.
         */
        HTTPMessage& SetMethod(const MessageMethod& method)
        {
            this->m_method = method;

            return *this;
        }

        /**
         * Grab the current method for this message.
         * 
         * Returns `NONE` if this is a response.
         */
        inline MessageMethod GetMethod() const
        {
            return m_method;
        }

        /**
         * Set the path of this message, which will be used if it is a request.
         */
        HTTPMessage& SetPath(const std::string& path)
        {
            this->m_path = path;

            return *this;
        }

        /**
         * Grab the current associated path of this message.
         */
        inline std::string GetPath() const
        {
            return m_path;
        }

        /**
         * Set the version of this HTTP message to the string specified.
         */
        HTTPMessage& SetVersion(const std::string& version)
        {
            this->m_version = version;

            return *this;
        }

        /**
         * Get the current HTTP version for this message.
         */
        inline std::string GetVersion() const
        {
            return m_version;
        }

        /**
         * Set the status code of this HTTP message.
         */
        HTTPMessage& SetStatusCode(uint16_t code)
        {
            this->m_statusCode = code;
            
            return *this;
        }

        /**
         * Get the status code for this message.
         */
        inline uint16_t GetStatusCode() const
        {
            return m_statusCode;
        }

        /**
         * Set the status message of this HTTP message.
         */
        HTTPMessage& SetStatusMessage(const std::string& message)
        {
            this->m_statusMessage = message;

            return *this;
        }

        /**
         * Get the current status message for this message.
         * 
         * Returns an autogenerated status if one isn't specified.
         */
        inline std::string GetStatusMessage() const
        {
            if (m_statusMessage.empty())
                return StatusTextFromStatusCode(m_statusCode);
            else
                return m_statusMessage;
        }

        /**
         * Takes the headers added to the message along with
         * the body and outputs it to a `std::string` for use
         * in client/server HTTP messages.
         */
        std::string ToString() const
        {
            std::stringstream output;

            // begin by forming the start line of the message
            if (m_method == MessageMethod::NONE)
            {
                output << HTTPVersion11 << " " << m_statusCode << " ";
                
                if (m_statusMessage.empty())
                    output << StatusTextFromStatusCode(m_statusCode);
                else
                    output << m_statusMessage;
            }
            else
            {
                output << MessageMethodToString(m_method) << " ";
                output << m_path << " ";
                output << HTTPVersion11;
            }

            // output the status lines line break to move on
            output << CarriageReturn;

            // output headers to the message string
            for (auto& header : m_headers)
                output << header.first << ": " << header.second << CarriageReturn;

            // automatically output the content length based on
            // the size of the body member if body isn't empty
            if (!m_body.empty())
                output << "Content-Length: " << m_body.size() << CarriageReturn;

            // seperate headers and body with an extra carriage return
            output << CarriageReturn;

            // convert the 8-bit unsigned body to a std::string for output
            std::string body(m_body.begin(), m_body.end());

            output << body;

            return output.str();
        }

        /**
         * Set the body of this message to a string value.
         */
        HTTPMessage& SetMessageBody(const std::string& body)
        {
            return SetMessageBody(std::vector<uint8_t>(body.begin(), body.end()));
        }

        /**
         * Set the body of this message to an unsigned 8-bit binary value.
         */
        HTTPMessage& SetMessageBody(const std::vector<uint8_t>& body)
        {
            this->m_body = std::vector<uint8_t>(body);

            return *this;
        }

        /**
         * Get the body vector for this message.
         */
        inline std::vector<uint8_t>& GetMessageBody()
        {
            return m_body;
        }

        /**
         * Return the size of the binary body vector.
         */
        inline size_t ContentLength()
        {
            return m_body.size();
        }

        /**
         * Return the amount of headers in the message.
         */
        inline size_t HeaderCount()
        {
            return m_headers.size();
        }

    private:
        /**
         * The HTTP method for this message.
         * 
         * Defaults to `NONE` denoting a response.
         */
        MessageMethod m_method = MessageMethod::NONE;

        /**
         * A status code for this message.
         * 
         * This is ignored if this is a request, as requests have no notion of statuses.
         */
        uint16_t m_statusCode = 0;

        /**
         * A status message to be associated with the status code for this message.
         * 
         * Keep blank to use an automatically generated status message.
         */
        std::string m_statusMessage = "";

        /**
         * The path for the resource specified in the message. Only used for a request.
         * 
         * Defaults to blank.
         */
        std::string m_path = "";

        /**
         * The version used for this HTTP message as a string.
         * 
         * Defaults to "HTTP/1.1"
         */
        std::string m_version = HTTPVersion11;

        /**
         * An `unordered_map` of headers using a `std::string` for both the key and the
         * value.
         */
        std::unordered_map<std::string, std::string> m_headers;

        /**
         * A vector of unsigned 8-bit integers used to store message bodies.
         */
        std::vector<uint8_t> m_body;

    };

    /**
     * An enum of states that the HTTPMessageParser can be in.
     */
    enum class MessageParserState : uint8_t
    {
        NONE,
        PARSING_START_LINE,
        START_LINE_REQUEST,
        START_LINE_RESPONSE,
        HEADER_KEY,
        HEADER_VALUE,
        PARSING_BODY,
    };

    /**
     * A basic class to parse a HTTP message, both request and response.
     * 
     * Allows for either string data to be passed in, or for a vector of
     * unsigned 8-bit integers to be passed in.
     * 
     * Requires that a whole message be sent in for parsing.
     */
    class HTTPMessageParser
    {
    public:
        /**
         * Parse a std::string to a HTTP message.
         * 
         * Pass in a pointer to an HTTPMessage which is then written to for headers
         * and other message data.
         * 
         * note: this must be a complete HTTP message
         */
        void Parse(HTTPMessage* httpMessage, const std::string& buffer)
        {
            Parse(httpMessage, std::vector<uint8_t>(buffer.begin(), buffer.end()));
        }
        
        /**
         * Parse a binary vector to an HTTP message.
         * 
         * Pass in a pointer to an HTTPMessage which is written to for headers and
         * other message data.
         * 
         * note: shrink the vector buffer before passing it in with `shrink_to_fit`
         * otherwise empty characters will show up for the body
         * note: must be a complete HTTP message.
         */
        void Parse(HTTPMessage* httpMessage, const std::vector<uint8_t>& buffer)
        {
            // begin by parsing the start line without knowing if it is a
            // request or a response by setting as undetermined
            MessageParserState state = MessageParserState::PARSING_START_LINE;
        
            // a temporary string instance used for storing characters of a
            // current line in the message being parsed
            std::string temp = "";

            // whether to skip the next character (for a carriage return)
            bool skipNext = false;

            // the current key for a header
            std::string headerKey = "";

            // whether or not a message body is present
            bool hasMessageBody = false;

            // the index at which the message body begins
            size_t bodyStartIndex = 0;

            for (size_t index = 0; index < buffer.size(); index++)
            {
                uint8_t character = buffer[index];

                // skip this character as it was marked
                if (skipNext)
                {
                    skipNext = false;

                    continue;
                }

                // if we are parsing the body, then we only need to grab an index and break
                // out of this loop as we want to merely insert the data from this vector
                // into the body vector
                if (state == MessageParserState::PARSING_BODY)
                {
                    hasMessageBody = true;

                    bodyStartIndex = index;

                    break;
                }

                // if we are parsing the start line but neither a response or request
                if (state == MessageParserState::PARSING_START_LINE)
                {
                    // if we hit a space, we have to check if the start line begins
                    // with the HTTP version or the method verb
                    if (character == ' ')
                    {
                        // this message has a leading version string, thus it is
                        // a response and not a request
                        if (temp == HTTPVersion1 || temp == HTTPVersion11)
                        {
                            httpMessage->SetMethod(MessageMethod::NONE);

                            state = MessageParserState::START_LINE_RESPONSE;

                            temp = "";

                            continue;
                        }
                        // this must be a request, so grab the MessageMethod type
                        // for the request, set it, and move on
                        else
                        {
                            httpMessage->SetMethod(StringToMessageMethod(temp));

                            state = MessageParserState::START_LINE_REQUEST;
                        
                            temp = "";

                            continue;
                        }
                    }
                }
                // do actions for when the start line is a request
                else if (state == MessageParserState::START_LINE_REQUEST)
                {
                    // once a space is hit, add the path to the message
                    if (character == ' ')
                    {
                        httpMessage->SetPath(temp);

                        temp = "";

                        continue;
                    }
                    // when the beginning of a carriage return is hit, add the version string
                    // to the message and then skip the following new line character, setting
                    // the state of the parser to be parsing headers
                    else if (character == '\r')
                    {
                        httpMessage->SetVersion(temp);

                        temp = "";

                        state = MessageParserState::HEADER_KEY;

                        skipNext = true;

                        continue;
                    }
                }
                // do actions for when the start line is a response
                else if (state == MessageParserState::START_LINE_RESPONSE)
                {
                    // if we are at a space, then we have hit the status code for the response
                    if (character == ' ')
                    {
                        int code = std::stoi(temp);

                        httpMessage->SetStatusCode(static_cast<uint16_t>(code));

                        temp = "";

                        continue;
                    }
                    // if we are at a carriage return start, then set the status message for
                    // the response, this can be blank in which it will use a generated status
                    //
                    // this will also set the state of the parser to move on to headers
                    else if (character == '\r')
                    {
                        httpMessage->SetStatusMessage(temp);

                        temp = "";

                        state = MessageParserState::HEADER_KEY;

                        skipNext = true;

                        continue;
                    }
                }
                // if we are parsing header keys and hit a colon, then the key for the header has
                // been fully parsed and should be added to the temporary key holder
                else if (state == MessageParserState::HEADER_KEY && character == ':')
                {
                    headerKey = temp;

                    temp = "";

                    state = MessageParserState::HEADER_VALUE;

                    // HTTP defines that the next character in a header should be a space
                    // so skip that for parsing the value of the header
                    skipNext = true;

                    continue;
                }
                // if we are parsing header values and hit the beginning of a carriage return then
                // it is time to add the header to the message with the key and value, and move the
                // state back to parsing keys
                else if (state == MessageParserState::HEADER_VALUE && character == '\r')
                {
                    httpMessage->SetHeader(headerKey, temp);

                    headerKey = "";
                    temp      = "";

                    state = MessageParserState::HEADER_KEY;

                    // skip the next character as it will just be a newline
                    skipNext = true;

                    continue;
                }
                // if we are parsing header keys and we hit a carriage return, then we should assume
                // that the headers have ended, and that we are now parsing a message body.
                else if (state == MessageParserState::HEADER_KEY && character == '\r')
                {
                    temp = "";

                    state = MessageParserState::PARSING_BODY;

                    // skip the next character as it'll be a newline
                    skipNext = true;

                    continue;
                }

                temp += character;
            }

            // add the body to the message if it is present
            if (hasMessageBody)
                httpMessage->GetMessageBody().insert(
                    httpMessage->GetMessageBody().begin(),
                    buffer.begin() + bodyStartIndex,
                    buffer.end()
                );
        }

    };
}
#endif
