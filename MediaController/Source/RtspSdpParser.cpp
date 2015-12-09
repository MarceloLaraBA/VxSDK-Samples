#include "stdafx.h"
#include "RtspSdpParser.h"

using namespace std;
using namespace MediaController::Constants;
using namespace MediaController::Rtsp;

static const MediaDescription EmptyMediaDescription;

void ParseMediaDescription(const string& line, string& media, unsigned short& port, string& protocol, unsigned short& payload);
void ParseConnectionInformation(const string& line, string& ip, unsigned short& ttl, bool& isMulticast);
void ParseRtpMap(const string& line, const unsigned short payload, string& encoding, unsigned int& rate);

SdpParser::SdpParser() {}

SdpParser::~SdpParser() {}

const vector<MediaDescription>& SdpParser::GetMediaDescriptions() {
    return this->_mediaDescriptions;
}

void SdpParser::Parse(const string& sdp) {
    this->_mediaDescriptions.clear();
    stringstream sdpStream(sdp);
    string line;

    MediaDescription media;
    while (getline(sdpStream, line)) {
        // Check if the current line is the start a new media stream description.
        if (line.substr(0, 2) == "m=") {
            if (!media.type.empty()) {
                this->_mediaDescriptions.push_back(media);
                media = EmptyMediaDescription;
            }
            ParseMediaDescription(line, media.type, media.port, media.protocol, media.payload);
        }
        else if (!media.type.empty()) {
            // If the line contains the connection information, parse it.
            if (line.substr(0, 2) == "c=")
                ParseConnectionInformation(line, media.ip, media.ttl, media.isMulticast);
            // If the line contains the RTP map, parse it.
            else if (line.substr(0, 9) == "a=rtpmap:")
                ParseRtpMap(line, media.payload, media.encoding, media.rate);
        }
    }

    if (!media.type.empty())
        this->_mediaDescriptions.push_back(media);
}

const MediaDescription& SdpParser::GetFirstVideo() {
    for (size_t i = 0; i < _mediaDescriptions.size(); i++) {
        const MediaDescription& md = _mediaDescriptions[i];
        if (md.type == "video")
            return md;
    }
    return EmptyMediaDescription;
}

void ParseMediaDescription(const string& line, string& media, unsigned short& port, string& protocol, unsigned short& payload) {
    vector<string> items;
    boost::split(items, line, boost::is_any_of(kWhitespace));
    for (size_t i = 0; i < items.size(); i++) boost::trim(items[i]);

    if (items.size() >= 4) {
        media = items[0].substr(2);
        port = boost::lexical_cast<unsigned short>(items[1]);
        protocol = items[2];
        payload = boost::lexical_cast<unsigned short>(items[3]);
    }
}

void ParseConnectionInformation(const string& line, string& ip, unsigned short& ttl, bool& isMulticast) {
    vector<string> items;
    boost::split(items, line, boost::is_any_of(kWhitespace));
    for (size_t i = 0; i < items.size(); i++) boost::trim(items[i]);

    if (items.size() >= 3) {
        vector<string> items2;
        boost::split(items2, items[2], boost::is_any_of(kForwardSlash));
        for (size_t i = 0; i < items2.size(); i++) boost::trim(items2[i]);

        ip = items2[0];
        int firstOctet = atoi(ip.c_str());
        isMulticast = firstOctet >= 224 && firstOctet <= 239;

        if (items2.size() >= 2)
            ttl = boost::lexical_cast<unsigned short>(items2[1]);
    }
}

void ParseRtpMap(const string& line, const unsigned short payload, string& encoding, unsigned int& rate) {
    vector<string> items;
    boost::split(items, line, boost::is_any_of(kColon));
    for (size_t i = 0; i < items.size(); i++) boost::trim(items[i]);

    if (items.size() >= 2) {
        vector<string> items2;
        boost::split(items2, items[1], boost::is_any_of(kWhitespace));
        for (size_t i = 0; i < items2.size(); i++) boost::trim(items2[i]);
        unsigned short payloadFound = boost::lexical_cast<unsigned short>(items2[0]);

        if (payload == payloadFound) {
            vector<string> items3;
            boost::split(items3, items2[1], boost::is_any_of(kForwardSlash));
            for (size_t i = 0; i < items3.size(); i++) boost::trim(items3[i]);
            encoding = items3[0];
            rate = boost::lexical_cast<unsigned int>(items3[1]);
        }
    }
}