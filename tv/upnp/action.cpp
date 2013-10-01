#include "action.h"
#include "device.h"
#include <iostream>

namespace TV {
namespace UPnP {

Action::Action(Service *parent) {
    this->parent = parent;
    this->actionName = "";
}
Action::~Action() {
    std::map<std::string,struct Argument*>::iterator it;
    for ( it=inArgumentList.begin() ; it != inArgumentList.end(); it++ )
        delete it->second;
    for ( it=outArgumentList.begin() ; it != outArgumentList.end(); it++ )
        delete it->second;
    inArgumentList.clear();
    outArgumentList.clear();
}

bool Action::Init(IXML_Node *node) {
    IXML_NodeList *nodeList = 0;
    int nodeLength;
    bool ret = false;

    nodeList = ixmlNode_getChildNodes(node);
    if (!nodeList) goto InitError;
    nodeLength = ixmlNodeList_length(nodeList);
    for (int i=0; i<nodeLength; i++) {
        IXML_Node *tmpNode = ixmlNodeList_item(nodeList, i);
        if (!tmpNode) continue;
        std::string nodeName = ixmlNode_getLocalName(tmpNode);
        std::string nodeValue = Device::XMLGetTagValue(tmpNode);
        if (nodeName == "name")
            actionName = nodeValue;
        else if (nodeName == "argumentList")
            if (!UPnPLoadArgumentListXML(tmpNode))
                goto InitError;
    }
    if (actionName != "") ret = true;

    InitError:
    if (nodeList) ixmlNodeList_free(nodeList);
    return ret;
}

bool Action::setArgumentValue(std::string argumentName, std::string argumentValue) {
    ithread_mutex_lock(parent->parent->deviceMutex);
    std::map<std::string,struct Argument*>::iterator it = inArgumentList.find(argumentName);
    if (it==inArgumentList.end()) {
        ithread_mutex_unlock(parent->parent->deviceMutex);
        return false;
    }
    it->second->value = argumentValue;
    ithread_mutex_unlock(parent->parent->deviceMutex);
    return true;
}

bool Action::UPnPLoadArgumentListXML(IXML_Node *node) {
    IXML_NodeList *nodeList = 0;
    int nodeLength;
    bool ret = false;

    nodeList = ixmlNode_getChildNodes(node);
    if (!nodeList) goto UPnPLoadArgumentListXMLError;
    nodeLength = ixmlNodeList_length(nodeList);
    for (int i=0; i<nodeLength; i++) {
        IXML_Node *tmpNode = ixmlNodeList_item(nodeList, i);
        if (!tmpNode) goto UPnPLoadArgumentListXMLError;
        struct Argument *argument = UPnPLoadArgumentXML(tmpNode);
        if (argument) {
            if (argument->direction == "in")
                inArgumentList.insert(std::pair<std::string,struct Argument*>(argument->name,argument));
            else
                outArgumentList.insert(std::pair<std::string,struct Argument*>(argument->name,argument));
        } else goto UPnPLoadArgumentListXMLError;
    }
    ret = true;

    UPnPLoadArgumentListXMLError:
    if (nodeList) ixmlNodeList_free(nodeList);
    return ret;
}

struct Argument *Action::UPnPLoadArgumentXML(IXML_Node *node) {
    struct Argument *argument = new struct Argument;
    IXML_NodeList *nodeList = 0;
    int nodeLength;
    bool ret = false;

    nodeList = ixmlNode_getChildNodes(node);
    if (!nodeList) goto UPnPLoadArgumentXMLError;
    nodeLength = ixmlNodeList_length(nodeList);
    for (int i=0; i<nodeLength; i++) {
        IXML_Node *tmpNode = ixmlNodeList_item(nodeList, i);
        if (!tmpNode) continue;
        std::string nodeName = ixmlNode_getLocalName(tmpNode);
        std::string nodeValue = Device::XMLGetTagValue(tmpNode);
        if (nodeName == "name")
            argument->name = nodeValue;
        else if (nodeName == "direction")
            argument->direction = nodeValue;
    }
    if (argument->name != "" && (argument->direction=="in" || argument->direction=="out"))
        ret = true;

    UPnPLoadArgumentXMLError:
    if (nodeList) ixmlNodeList_free(nodeList);
    if (ret) return argument;
    delete argument;
    return 0;
}

bool Action::postControlAction(ActionResponse &action_response) {
    ithread_mutex_lock(parent->parent->deviceMutex);
    bool ret = false;
    IXML_Document *actionNode = 0, *response = 0;
    if (inArgumentList.size() == 0)
        actionNode = UpnpMakeAction(actionName.c_str(),parent->getServiceType().c_str(), 0, 0);
    else {
        std::map<std::string,struct Argument*>::iterator it;
        for ( it=inArgumentList.begin() ; it != inArgumentList.end(); it++ ) {
            //std::cout << "AddToAction - " << it->first.c_str()  << "=" << it->second->value.c_str() << std::endl;
            if (UpnpAddToAction(&actionNode,actionName.c_str(),parent->getServiceType().c_str(),
                    it->first.c_str(), it->second->value.c_str()) != UPNP_E_SUCCESS) {
                if (actionNode) ixmlDocument_free(actionNode);
                actionNode = 0;
                break;
            }
        }
    }

    if (actionNode) {
        int upnp_ret = UpnpSendAction(parent->parent->ctrlpt_handle, parent->controlURL.c_str(),
            parent->getServiceType().c_str(), 0, actionNode, &response);
        if (upnp_ret == UPNP_E_SUCCESS || upnp_ret > 0) {
            //std::cout << response << std::endl;

            IXML_NodeList *tags = ixmlDocument_getElementsByTagName(response, "*");
            if (tags) {
                IXML_Node *node = ixmlNodeList_item(tags, 0);
                action_response.setResponseType(ixmlNode_getLocalName(node));
                //std::cout << ixmlNode_getLocalName(node) << std::endl;
                IXML_NodeList *nodeList = ixmlNode_getChildNodes(node);
                if (nodeList) {
                    int nodeLength = ixmlNodeList_length(nodeList);
                    for (int i=0; i<nodeLength; i++) {
                        IXML_Node *tmpNode = ixmlNodeList_item(nodeList, i);
                        if (!tmpNode) continue;
                        std::string nodeName = ixmlNode_getLocalName(tmpNode);
                        std::string nodeValue = Device::XMLGetTagValue(tmpNode);
                        action_response.setValue(nodeName, nodeValue);
                        //std::cout << nodeName << ":" << nodeValue << std::endl;
                    }
                    ixmlNodeList_free(nodeList);
                }
                ixmlNodeList_free(tags);
            }

            ret = true;
        }
        if (response) ixmlDocument_free(response);
        ixmlDocument_free(actionNode);
    }
    ithread_mutex_unlock(parent->parent->deviceMutex);
    return ret;
}

}}
