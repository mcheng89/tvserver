#ifndef TV_XMLTV_XMLTVREADER_H
#define TV_XMLTV_XMLTVREADER_H

#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>
#include <xercesc/util/TransService.hpp>
#include <string>
#include <time.h>

namespace TV {
namespace XMLTV {

    struct ChannelInfo {
        int number;
        std::string id;
        std::string name;
    };
    struct ProgramInfo {
        std::string channel_id;
        std::string title;
        std::string subtitle;
        std::string description;
        time_t start_time;
        time_t end_time;
        std::string zap2it;
    };
    enum ParseState {SEARCHING,
        BUILDCHANNEL,BUILDCHANNELINFO,DONECHANNEL,
        BUILDPROGRAM,BUILDPROGRAMTITLE,BUILDPROGRAMSUBTITLE,BUILDPROGRAMDESCRIPT,BUILDPROGRAMZAP2IT,DONEPROGRAM};

    class XMLTV_SAXHandler : public xercesc::DefaultHandler {
    public:
        XMLTV_SAXHandler();
        ~XMLTV_SAXHandler();

        void startDocument ();
        void startElement (const XMLCh *const uri, const XMLCh *const localname, const XMLCh *const qname, const xercesc::Attributes &attrs);
        void characters (const XMLCh *const chars, const XMLSize_t length);
        void endElement (const XMLCh *const uri, const XMLCh *const localname, const XMLCh *const qname);

        enum ParseState getParseState();
        struct ProgramInfo getProgramInfo();
        struct ChannelInfo getChannelInfo();
    private:
        std::string getAttribute(const xercesc::Attributes &attrs, std::string attrname);
        std::string transcodeString(const XMLCh *const toTranscode);
        static time_t parseTime(std::string str);

        enum ParseState state;
        struct ProgramInfo programInfo;
        struct ChannelInfo channelInfo;

        xercesc::XMLTranscoder* utf8Conv;
    };

}}

#endif // TV_XMLTV_XMLTVREADER_H
