#ifndef TV_UPNP_ACTION_H
#define TV_UPNP_ACTION_H

#include <map>
#include <string>
#include <upnp/upnp.h>

namespace TV {
namespace UPnP {

    class Service;
    class ActionResponse;

    struct Argument {
        std::string name;
        std::string direction;
        std::string value;
    };

    class Action {
    public:
        Action(Service *parent);
        ~Action();

        std::string getName() {return actionName;}

        bool Init(IXML_Node *node);
        bool postControlAction(ActionResponse &action_response);
        bool setArgumentValue(std::string argumentName, std::string argumentValue);
    private:
        bool UPnPLoadArgumentListXML(IXML_Node *node);
        struct Argument *UPnPLoadArgumentXML(IXML_Node *node);

        Service *parent;
        std::string actionName;
        std::map<std::string,struct Argument*> inArgumentList;
        std::map<std::string,struct Argument*> outArgumentList;

        Action(const Action&);     // do not give these a body
        Action& operator = (const Action&);
    };

    class ActionResponse {
    public:
        std::string getResponseType() { return resp_type; }
        std::string getValue(std::string name) {
            std::map<std::string,std::string>::iterator it = args.find(name);
            if (it==args.end())
                return "";
            return it->second;
        }

        void setResponseType(std::string resp_type) { this->resp_type = resp_type; }
        void setValue(std::string name, std::string value) {
            args.insert(std::pair<std::string,std::string>(name,value));
        }

    private:
        std::string resp_type;
        std::map<std::string,std::string> args;
    };

}}

#endif // TV_UPNP_ACTION_H
