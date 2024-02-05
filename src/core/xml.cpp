#include "xml.hpp"

#include <fstream>

namespace lightwave {

XMLParser::XMLParser(Delegate &delegate, std::istream &stream)
: m_delegate(delegate), m_stream(&stream) {
    m_loc.filename = "stream";
    parse();
}

XMLParser::XMLParser(Delegate &delegate, const std::filesystem::path &path)
: m_delegate(delegate) {
    m_loc.filename = path.string();
    std::ifstream file { path };
    if (!std::filesystem::is_regular_file(path)) {
        lightwave_throw("%s is not a file", path.string());
    }
    if (!file.is_open()) {
        lightwave_throw("could not open %s", path.string());
    }
    m_stream = &file;
    parse();
}

void XMLParser::parse() {
    try {
        while (readNode(""));
    } catch (...) {
        lightwave_throw_nested("while parsing %s:%d:%d", m_loc.filename, m_loc.line, m_loc.column);
    }
}

int XMLParser::peek() {
    return m_stream->peek();
}

int XMLParser::get() {
    const int chr = m_stream->get();
    if (chr == '\n') {
        m_loc.line++;
        m_loc.column = 1;
    } else {
        m_loc.column++;
    }
    return chr;
}

void XMLParser::expectToken(char token) {
    skipWhitespace();
    if (get() != token) {
        lightwave_throw("expected token '%c'", token);
    }
}

std::string XMLParser::readIdentifier() {
    skipWhitespace();

    if (!isalpha(peek())) {
        lightwave_throw("expected identifier");
    }

    std::string identifier = std::string("") + char(get());
    while (isalnum(peek())) identifier += char(get());
    return identifier;
}

std::string XMLParser::readString() {
    skipWhitespace();
    if (get() != '"') lightwave_throw("expected string");
    
    std::string string = "";
    while (true) {
        int chr = get();
        switch (chr) {
        case '\\':
            switch (get()) {
            case 'n': string += '\n'; break;
            case 'r': string += '\r'; break;
            case 't': string += '\t'; break;
            }
            break;
        case EOF: lightwave_throw("expected end of string");
        case '"': return string;
        default: string += (std::string::value_type)chr;
        }
    }
}

void XMLParser::readComment() {
    int dashCount = 0;
    while (true) {
        int chr = get();
        if (chr == '>' && dashCount == 2) return;
        if (chr == EOF) lightwave_throw("expected end of comment");
        dashCount = chr == '-' ? dashCount + 1 : 0;
    }
}

void XMLParser::skipWhitespace() {
    while (isspace(peek())) get();
}

bool XMLParser::readNode(std::string enclosingTag) {
    skipWhitespace();

    if (peek() == EOF) {
        if (enclosingTag != "") {
            lightwave_throw("expected closing tag for <%s />", enclosingTag);
        }
        return false;
    }

    if (get() != '<') {
        lightwave_throw("expected node");
    }

    switch (peek()) {
    case '/': {
        get();
        const std::string closingTag = readIdentifier();
        expectToken('>');
        if (enclosingTag != closingTag) {
            lightwave_throw("expected closing tag of </%s> but found </%s>", enclosingTag, closingTag);
        }
        m_delegate.close();
        return false;
    }
    case '!': {
        get();
        if (get() != '-') lightwave_throw("expected comment");
        if (get() != '-') lightwave_throw("expected comment");
        readComment();
        return true;
    }
    }

    const std::string tag = readIdentifier();
    m_delegate.open(tag);
    while (true) {
        skipWhitespace();

        switch (peek()) {
        case '/': {
            get();
            expectToken('>');
            m_delegate.enter();
            m_delegate.close();
            return true;
        }
        case '>': {
            get();
            m_delegate.enter();
            while (readNode(tag));
            return true;
        }
        }

        const std::string attr = readIdentifier();
        expectToken('=');
        const std::string value = readString();

        m_delegate.attribute(attr, value);
    }
}

}
