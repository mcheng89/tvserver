#include "device.h"
#include <iostream>

namespace TV {
namespace UPnP {

Threads::Mutex Device::init_mutex;
int Device::init_count = 0;

Device::Device() {
    parent = 0;
    URLBase = "";
    ctrlpt_handle = 0;
    deviceMutex = new ithread_mutex_t;
    ithread_mutex_init(deviceMutex, 0);
    initialized = false;
}
Device::Device(Device *parent) {
    this->parent = parent;
    this->URLBase = parent->URLBase;
    ctrlpt_handle = parent->ctrlpt_handle;
    deviceMutex = parent->deviceMutex;
    initialized = false;
}
Device::~Device() {
    for (int i=0; i<services.size(); i++)
        delete services.at(i);
    services.clear();
    for (int i=0; i<subDevices.size(); i++)
        delete subDevices.at(i);
    subDevices.clear();
    if (parent == 0)
        ithread_mutex_destroy(deviceMutex);
}

bool Device::Init(std::string description_url) {
    init_mutex.lock();
    if (!initialized)
    {
        if (init_count == 0) {
            if (!UPnPInit() || !UPnPRegisterClient()) {
                Finish();
                return false;
            }
            UpnpSetMaxContentLength(50000);
        }
        init_count++;
        initialized = true;
    }
    init_mutex.unlock();
    if (!UPnPLoadDescriptionXML(description_url)) {
        Finish();
        return false;
    }
    return true;
}
void Device::Finish() {
    init_mutex.lock();
    if (initialized)
    {
        init_count--;
        initialized = false;
        if (init_count == 0) {
            UPnPUnregisterClient();
            UPnPFinish();
        }
    }
    init_mutex.unlock();
}

bool Device::UPnPInit() {
    int ret;
	if ( (ret=UpnpInit(NULL, 0)) != UPNP_E_SUCCESS) {
	    //std::cout << "** ERROR UpnpInit(): " << ret <<  UpnpGetErrorMessage (ret) << std::endl;
        return false;
    }
    return true;
}
void Device::UPnPFinish() {
    UpnpFinish();
}
bool Device::UPnPRegisterClient() {
    UpnpRegisterClient(&UPnPCallback, 0, &ctrlpt_handle);
    return true;
}
void Device::UPnPUnregisterClient() {
    if (ctrlpt_handle)
        UpnpUnRegisterClient(ctrlpt_handle);
}

int Device::UPnPCallback(Upnp_EventType type, void* event, void* cookie) {
    return UPNP_E_SUCCESS;
}

bool Device::UPnPLoadDescriptionXML(std::string url) {
    IXML_Document *descDoc = 0;
    IXML_NodeList *rootNodeHandle = 0;
    IXML_Node *rootNode = 0;
    bool ret = false;

    int err;
    if ( (err=UpnpDownloadXmlDoc(url.c_str(), &descDoc)) != UPNP_E_SUCCESS) {
        //std::cout << "** ERROR UPnPDownloadXML(): " << err <<  UpnpGetErrorMessage (err) << std::endl;
        goto UPnPLoadDescriptionXMLError;
	}
	rootNodeHandle = ixmlDocument_getElementsByTagName(descDoc, "device");
	if (!rootNodeHandle) goto UPnPLoadDescriptionXMLError;
	rootNode = ixmlNodeList_item(rootNodeHandle, 0);//first node only
    if (!rootNode) goto UPnPLoadDescriptionXMLError;

    URLBase = XMLGetFirstDocumentItem(descDoc, "URLBase");
    if (URLBase == "") URLBase = url;
    ret = UPnPLoadDescriptionXML(rootNode);

    UPnPLoadDescriptionXMLError:
    if (rootNodeHandle) ixmlNodeList_free(rootNodeHandle);
    if (descDoc) ixmlDocument_free(descDoc);
    return ret;
}

bool Device::UPnPLoadDescriptionXML(IXML_Node *rootNode) {
    int nodeLength = 0;
    bool ret = false;

    IXML_NodeList *nodeList = ixmlNode_getChildNodes(rootNode);
    if (!nodeList) goto UPnPLoadDescriptionXMLError2;

    nodeLength = ixmlNodeList_length(nodeList);
    for (int i=0; i<nodeLength; i++) {
        IXML_Node *tmpNode = ixmlNodeList_item(nodeList, i);
        if (!tmpNode) continue;
        std::string nodeName = ixmlNode_getLocalName(tmpNode);
        std::string nodeValue = XMLGetTagValue(tmpNode);
        if (nodeName == "friendlyName")
            friendlyName = nodeValue;
        else if (nodeName == "deviceType")
            deviceType = nodeValue;
        else if (nodeName == "serviceList") {
            IXML_NodeList *serviceList = ixmlNode_getChildNodes(tmpNode);
            if (serviceList) {
                int serviceLength = ixmlNodeList_length(serviceList);
                for (int i=0; i<serviceLength; i++) {
                    IXML_Node *serviceNode = ixmlNodeList_item(serviceList, i);
                    if (!serviceNode) continue;
                    Service *service = new Service(this);
                    if (service->Init(serviceNode))
                        services.push_back(service);
                    else delete service;
                }
                ixmlNodeList_free(serviceList);
            }
        } else if (nodeName == "deviceList") {
            IXML_NodeList *deviceList = ixmlNode_getChildNodes(tmpNode);
            if (deviceList) {
                int deviceLength = ixmlNodeList_length(deviceList);
                for (int i=0; i<deviceLength; i++) {
                    IXML_Node *deviceNode = ixmlNodeList_item(deviceList, i);
                    if (!deviceNode) continue;
                    Device *subDevice = new Device(this);
                    if (subDevice->UPnPLoadDescriptionXML(deviceNode))
                        subDevices.push_back(subDevice);
                    else delete subDevice;
                }
                ixmlNodeList_free(deviceList);
            }
        }
    }

    ret = true;
    UPnPLoadDescriptionXMLError2:
    if (nodeList) ixmlNodeList_free(nodeList);
    return ret;
}

std::string Device::UPnPResolveURL(std::string baseURL, std::string relativeURL) {
    std::string ret = "";
    char *buffer = new char[baseURL.length()+relativeURL.length()+1];
    if (UpnpResolveURL(baseURL.c_str(), relativeURL.c_str(), buffer) == UPNP_E_SUCCESS)
        ret = buffer;
    delete buffer;
    return ret;
}

std::string Device::XMLGetFirstDocumentItem(IXML_Document *doc, std::string item) {
    std::string ret = "";
    IXML_Node *tmpNode = 0;
    IXML_NodeList *nodeList = ixmlDocument_getElementsByTagName(doc, item.c_str());
    if (!nodeList) goto XMLGetFirstDocumentItemRet;
    tmpNode = ixmlNodeList_item(nodeList, 0);
    if (!tmpNode) goto XMLGetFirstDocumentItemRet;

    ret = XMLGetTagValue(tmpNode);

    XMLGetFirstDocumentItemRet:
    if (nodeList) ixmlNodeList_free(nodeList);
    return ret;
}

std::string Device::XMLGetTagValue(IXML_Node *node) {
    IXML_Node *textNode = ixmlNode_getFirstChild(node);
    if (!textNode) return "";
    const char *value = ixmlNode_getNodeValue(textNode);
    if (!value) return "";
    return value;
}

}}
