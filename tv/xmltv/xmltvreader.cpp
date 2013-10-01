#include "xmltvreader.h"
#include "../utils/stringutils.h"
#include <stdio.h>

//#include <xercesc/util/PlatformUtils.hpp>

namespace TV {
namespace XMLTV {

XMLTV_SAXHandler::XMLTV_SAXHandler() {
    xercesc::XMLTransService::Codes resCode;
    utf8Conv = xercesc::XMLPlatformUtils::fgTransService->makeNewTranscoderFor("UTF-8", resCode, 16*1024);
}
XMLTV_SAXHandler::~XMLTV_SAXHandler() {
    delete utf8Conv;
}

std::string XMLTV_SAXHandler::getAttribute(const xercesc::Attributes &attrs, std::string attrname) {
    XMLCh *_attrname = xercesc::XMLString::transcode(attrname.c_str());
    std::string _attrvalue = transcodeString(attrs.getValue(_attrname));
    xercesc::XMLString::release(&_attrname);
    return _attrvalue;
}
std::string XMLTV_SAXHandler::transcodeString( const XMLCh *const toTranscode ) {
    if (!toTranscode) return "";
    unsigned int charsEaten;
    char resultXMLString_Encoded[16*1024+4];
    XMLSize_t bytesWritten = utf8Conv->transcodeTo(toTranscode, xercesc::XMLString::stringLen(toTranscode),
        (XMLByte*) resultXMLString_Encoded, 16*1024, charsEaten, xercesc::XMLTranscoder::UnRep_Throw );
    //char *fLocalForm = xercesc::XMLString::transcode( toTranscode );
    std::string theString;
    theString.append(resultXMLString_Encoded, bytesWritten);
    //xercesc::XMLString::release( &fLocalForm );
    return theString;
}
time_t XMLTV_SAXHandler::parseTime(std::string str) {
    struct tm timeParser;

    if ( str.length() < 12 || !Utils::StringUtils::isNumeric(str.substr(0,12)) )
        return 0;

    timeParser.tm_year = atoi( str.substr( 0,4 ).c_str() ) - 1900; //year
    timeParser.tm_mon = atoi( str.substr( 4,2 ).c_str() ) - 1; //month
    timeParser.tm_mday = atoi( str.substr( 6,2 ).c_str() ); //day of month
    timeParser.tm_hour = atoi( str.substr( 8,2 ).c_str() ); //hours since midnight
    timeParser.tm_min = atoi( str.substr( 10,2 ).c_str() ); //mins
    timeParser.tm_sec = 0;
    //timeParser.tm_isdst = -1;
    timeParser.tm_isdst = 0;

    time_t result = mktime( &timeParser )-timezone;

    // Analyse offset if it looks like there is one
	// some formats include 2 digits for seconds
	if (str.find(" ")!=std::string::npos) {
        std::string timezone = str.substr(str.find(" ")+1);
        if ( timezone.length()!=5 || !Utils::StringUtils::isNumeric(timezone.substr(1)) )
            return 0;
        time_t shiftHour = atoi( timezone.substr(1,2).c_str() );
		time_t shiftMin = atoi( timezone.substr(3,2).c_str() );
		time_t shiftTimeT = ( shiftHour * 3600 ) + ( shiftMin * 60 );
		if ( timezone.substr(0,1) == "+" )
			result -= shiftTimeT;
		else if ( timezone.substr(0,1) == "-" )
			result += shiftTimeT;
		else return 0;
	}
	return result;
}
void XMLTV_SAXHandler::startDocument () {
    state = SEARCHING;
}
void XMLTV_SAXHandler::startElement (const XMLCh *const uri, const XMLCh *const localname, const XMLCh *const qname, const xercesc::Attributes &attrs) {
    if (state == DONECHANNEL || state==DONEPROGRAM) state = SEARCHING;

    std::string _qname = transcodeString(qname);
    if (_qname.compare("channel")==0) {
        state = BUILDCHANNEL;
        channelInfo.number = 0;
        channelInfo.id = getAttribute(attrs,"id");
        channelInfo.name = "";
    } else if (_qname.compare("programme")==0) {
        state = BUILDPROGRAM;
        programInfo.channel_id = getAttribute(attrs, "channel");
        programInfo.title = "";
        programInfo.subtitle = "";
        programInfo.description = "";
        programInfo.start_time = parseTime(getAttribute(attrs, "start"));
        programInfo.end_time = parseTime(getAttribute(attrs, "stop"));
        programInfo.zap2it = "";
    } else if (state==BUILDCHANNEL && _qname.compare("display-name")==0) {
        state = BUILDCHANNELINFO;
    } else if (state==BUILDPROGRAM && programInfo.title=="" && _qname.compare("title")==0) {
        state = BUILDPROGRAMTITLE;
    } else if (state==BUILDPROGRAM && programInfo.subtitle=="" && _qname.compare("sub-title")==0) {
        state = BUILDPROGRAMSUBTITLE;
    } else if (state==BUILDPROGRAM && programInfo.description=="" && _qname.compare("desc")==0) {
        state = BUILDPROGRAMDESCRIPT;
    } else if (state==BUILDPROGRAM && programInfo.zap2it=="" && _qname.compare("episode-num")==0) {
        if (getAttribute(attrs, "system") == "dd_progid")
            state = BUILDPROGRAMZAP2IT;
    }
    //else Logger::Info() << "startElement <" << _qname << ">" << std::endl;
}
void XMLTV_SAXHandler::characters (const XMLCh *const chars, const XMLSize_t length) {
    if (state == DONECHANNEL || state==DONEPROGRAM) state = SEARCHING;

    std::string _chars = transcodeString(chars);
    if (state==BUILDCHANNELINFO) {
        if (channelInfo.name.length()==0)
            channelInfo.name = _chars;
        if (Utils::StringUtils::isNumeric(_chars))
            channelInfo.number = atoi(_chars.c_str());
    } else if (state==BUILDPROGRAMTITLE) {
        programInfo.title += _chars;
    } else if (state==BUILDPROGRAMSUBTITLE) {
        programInfo.subtitle += _chars;
    } else if (state==BUILDPROGRAMDESCRIPT) {
        programInfo.description += _chars;
    } else if (state==BUILDPROGRAMZAP2IT) {
        programInfo.zap2it += _chars;
    }
    //Logger::Info() << "node text = " << _chars << std::endl;
}
void XMLTV_SAXHandler::endElement (const XMLCh *const uri, const XMLCh *const localname, const XMLCh *const qname) {
    if (state == DONECHANNEL || state==DONEPROGRAM) state = SEARCHING;

    std::string _qname = transcodeString(qname);
    if (state==BUILDCHANNEL && _qname.compare("channel")==0) {
        char strChannel[6];
        snprintf(strChannel, 5, "%i ", channelInfo.number);
        if (channelInfo.name.find(strChannel)==0) {
            channelInfo.name = channelInfo.name.substr(strlen(strChannel));
        }
        state = DONECHANNEL;
    } else if (state==BUILDPROGRAM && _qname.compare("programme")==0) {
        state = DONEPROGRAM;
    } else if (state==BUILDCHANNELINFO && _qname.compare("display-name")==0) {
        state = BUILDCHANNEL;
    } else if (state==BUILDPROGRAMTITLE && _qname.compare("title")==0) {
        state = BUILDPROGRAM;
    } else if (state==BUILDPROGRAMSUBTITLE && _qname.compare("sub-title")==0) {
        state = BUILDPROGRAM;
    } else if (state==BUILDPROGRAMDESCRIPT && _qname.compare("desc")==0) {
        state = BUILDPROGRAM;
    }  else if (state==BUILDPROGRAMZAP2IT && _qname.compare("episode-num")==0)
        state = BUILDPROGRAM;
    //else Logger::Info() << "endElement <" << _qname << ">" << std::endl;
}
enum ParseState XMLTV_SAXHandler::getParseState() {
    return state;
}
struct ProgramInfo XMLTV_SAXHandler::getProgramInfo() {
    return programInfo;
}
struct ChannelInfo XMLTV_SAXHandler::getChannelInfo() {
    return channelInfo;
}

}}
