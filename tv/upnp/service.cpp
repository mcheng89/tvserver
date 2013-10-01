#include "service.h"
#include "device.h"
#include <iostream>

namespace TV {
namespace UPnP {

Service::Service(Device *parent) {
    this->parent = parent;
    this->URLBase = parent->URLBase;
}
Service::~Service() {
    std::map<std::string,Action*>::iterator it;
    for ( it=actionList.begin() ; it != actionList.end(); it++ )
        delete it->second;
    actionList.clear();
}

bool Service::Init(IXML_Node *node) {
    int nodeLength = 0;
    bool ret = false;

    IXML_NodeList *nodeList = ixmlNode_getChildNodes(node);
    if (!nodeList) goto InitError;

    nodeLength = ixmlNodeList_length(nodeList);
    for (int i=0; i<nodeLength; i++) {
        IXML_Node *tmpNode = ixmlNodeList_item(nodeList, i);
        if (!tmpNode) continue;
        std::string nodeName = ixmlNode_getLocalName(tmpNode);
        std::string nodeValue = Device::XMLGetTagValue(tmpNode);
        if (nodeName == "serviceType")
            serviceType = nodeValue;
        else if (nodeName == "controlURL")
            controlURL = Device::UPnPResolveURL(URLBase, nodeValue);
        else if (nodeName == "eventSubURL")
            eventSubURL = Device::UPnPResolveURL(URLBase, nodeValue);
        else if (nodeName == "SCPDURL") {
            SCPDURL = Device::UPnPResolveURL(URLBase, nodeValue);
            UPnPLoadSCPDURLXML(SCPDURL);
        }
    }

    ret = true;

    InitError:
    if (nodeList) ixmlNodeList_free(nodeList);
    return ret;
}

Action *Service::getAction(std::string actionName) {
    std::map<std::string,Action*>::iterator it = actionList.find(actionName);
    if (it==actionList.end()) return 0;
    return it->second;
}

bool Service::UPnPLoadSCPDURLXML(std::string SCPDURL) {
    IXML_Document *descDoc = 0;
    IXML_NodeList *rootNodeHandle = 0, *nodeList = 0;
    IXML_Node *rootNode = 0;
    int nodeLength;
    bool ret = false;

    int err;
    if ( (err=UpnpDownloadXmlDoc(SCPDURL.c_str(), &descDoc)) != UPNP_E_SUCCESS) {
        //std::cout << "** ERROR UPnPDownloadXML(): " << err <<  UpnpGetErrorMessage (err) << std::endl;
        goto UPnPLoadSCPDURLXMLError;
	}
	rootNodeHandle = ixmlDocument_getElementsByTagName(descDoc, "scpd");
	if (!rootNodeHandle) goto UPnPLoadSCPDURLXMLError;
	rootNode = ixmlNodeList_item(rootNodeHandle, 0);//first node only
    if (!rootNode) goto UPnPLoadSCPDURLXMLError;

    nodeList = ixmlNode_getChildNodes(rootNode);
    if (!nodeList) goto UPnPLoadSCPDURLXMLError;
    nodeLength = ixmlNodeList_length(nodeList);
    for (int i=0; i<nodeLength; i++) {
        IXML_Node *tmpNode = ixmlNodeList_item(nodeList, i);
        if (!tmpNode) continue;
        std::string nodeName = ixmlNode_getLocalName(tmpNode);
        std::string nodeValue = Device::XMLGetTagValue(tmpNode);
        if (nodeName == "serviceStateTable") {
        } else if (nodeName == "actionList") {
            UPnPLoadActionListXML(tmpNode);
        }
    }
    ret = true;

	UPnPLoadSCPDURLXMLError:
    if (nodeList) ixmlNodeList_free(nodeList);
    if (rootNodeHandle) ixmlNodeList_free(rootNodeHandle);
    if (descDoc) ixmlDocument_free(descDoc);
    return ret;
}

bool Service::UPnPLoadActionListXML(IXML_Node *node) {
    IXML_NodeList *nodeList = 0;
    int nodeLength;
    bool ret = false;

    nodeList = ixmlNode_getChildNodes(node);
    if (!nodeList) goto UPnPLoadActionListXMLError;
    nodeLength = ixmlNodeList_length(nodeList);
    for (int i=0; i<nodeLength; i++) {
        IXML_Node *tmpNode = ixmlNodeList_item(nodeList, i);
        if (!tmpNode) continue;
        //These all should be actions...
        Action *action = new Action(this);
        if (action->Init(tmpNode) && actionList.find(action->getName())==actionList.end())
            actionList.insert(std::pair<std::string,Action*>(action->getName(), action));
        else delete action;
    }
    ret = true;

    UPnPLoadActionListXMLError:
    if (nodeList) ixmlNodeList_free(nodeList);
    return ret;
}

std::string Service::getStateVariable(std::string stateVar) {
    ithread_mutex_lock(parent->deviceMutex);
    std::string ret = "";
    char *response = 0;
    int err;
    if ( (err=UpnpGetServiceVarStatus(parent->ctrlpt_handle, controlURL.c_str(), stateVar.c_str(), &response)) != UPNP_E_SUCCESS) {
        //std::cout << "** ERROR getStateVariable(): " << err <<  UpnpGetErrorMessage (err) << std::endl;
    } else {
        ret = response;
    }
    ithread_mutex_unlock(parent->deviceMutex);
    if (response) ixmlFreeDOMString(response);
    return ret;
}

}}

