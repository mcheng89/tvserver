#ifndef TV_UPNP_SERVICE_H
#define TV_UPNP_SERVICE_H

#include "action.h"

#include <map>
#include <string>
#include <upnp/upnp.h>

namespace TV {
namespace UPnP {

    class Device;

    class Service {
    public:
        Service(Device *parent);
        ~Service();

        bool Init(IXML_Node *node);
        std::string getServiceType() {return serviceType;}
        std::string getStateVariable(std::string stateVar);
        Action *getAction(std::string actionName);

    private:
        bool UPnPLoadSCPDURLXML(std::string SCPDURL);
        bool UPnPLoadActionListXML(IXML_Node *node);

        Device *parent;
        std::string URLBase;
        std::string SCPDURL;
        std::string serviceType;
        std::string controlURL;
        std::string eventSubURL;

        std::map<std::string,Action *> actionList;

        friend class Action;

        Service(const Service&);     // do not give these a body
        Service& operator = (const Service&);
    };

}}

#endif // TV_UPNP_SERVICE_H
