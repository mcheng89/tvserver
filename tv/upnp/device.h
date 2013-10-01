#ifndef TV_UPNP_DEVICE_H
#define TV_UPNP_DEVICE_H

#include "service.h"
#include "action.h"
#include "../threads/mutex.h"

#include <string>
#include <vector>
#include <upnp/upnp.h>
#include <upnp/ithread.h>
#include <upnp/upnptools.h>

namespace TV {
namespace UPnP {

    class Device {
    public:
        Device();
        ~Device();

        bool Init(std::string description_url);
        void Finish();

        std::string getFriendlyName() {return friendlyName;}
        std::string getDeviceType() {return deviceType;}

        std::vector<Device *> getDeviceList() {return subDevices;}
        std::vector<Service *> getServiceList() {return services;}

    private:
        Device(Device *parent);

        static Threads::Mutex init_mutex;
        static int init_count;
        bool initialized;

        bool UPnPInit();
        void UPnPFinish();
        bool UPnPRegisterClient();
        void UPnPUnregisterClient();
        static int UPnPCallback(Upnp_EventType type, void* event, void* cookie);

        bool UPnPLoadDescriptionXML(std::string url);
        bool UPnPLoadDescriptionXML(IXML_Node *rootNode);
        static std::string UPnPResolveURL(std::string baseURL, std::string relativeURL);

        static std::string XMLGetFirstDocumentItem(IXML_Document *doc, std::string item);
        static std::string XMLGetTagValue(IXML_Node *node);

        Device *parent;
        ithread_mutex_t *deviceMutex;
        UpnpClient_Handle ctrlpt_handle;

        std::vector<Service *> services;
        std::vector<Device *> subDevices;
        std::string friendlyName, deviceType;
        std::string URLBase;

        friend class Action;
        friend class Service;

        Device(const Device&);     // do not give these a body
        Device& operator = (const Device&);
    };

}}

#endif // TV_UPNP_DEVICE_H
