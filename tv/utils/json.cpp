#include "json.h"

namespace TV {
namespace Utils {

JSON::JSON(JSONType type, bool init) {
    json_data = 0;
    if (init) {
        require_cleanup = true;
        if (type == ARRAY)
            json_data = json_object_new_array();
        else
            json_data = json_object_new_object();
    }
}
JSON::~JSON() {
    if (require_cleanup)
        json_object_put(json_data);
}

bool JSON::load(std::string data) {
    if (require_cleanup)
        json_object_put(json_data);
    require_cleanup = true;
    json_data = json_tokener_parse(data.c_str());
    return json_data;
}

void JSON::setArray(std::string field, JSON *obj) {
    json_object_object_add(json_data, field.c_str(), obj->json_data);
    obj->require_cleanup = false;
}
void JSON::addArrayObject(JSON *obj) {
    json_object_array_add(json_data, obj->json_data);
    obj->require_cleanup = false;
}
void JSON::addArrayBoolean(bool value) {
    json_object_array_add(json_data, json_object_new_boolean(value));
}

bool JSON::getArray(std::string field, JSON *obj) {
    json_object *array = json_object_object_get(json_data, field.c_str());
    if (!array || !json_object_is_type(array, json_type_array)) return false;
    if (obj->json_data && obj->require_cleanup)
        json_object_put(obj->json_data);
    obj->json_data = array;
    obj->require_cleanup = false;
    return true;
}
int JSON::getArraySize() {
    if (!json_data || !json_object_is_type(json_data, json_type_array)) return -1;
    return json_object_array_length(json_data);
}
bool JSON::getArrayObject(JSON *obj, int idx) {
    if (!json_data || !json_object_is_type(json_data, json_type_array)) return false;
    json_object *json_obj = json_object_array_get_idx(json_data, idx);
    if (!json_obj || !json_object_is_type(json_obj, json_type_object)) return false;
    if (obj->json_data && obj->require_cleanup)
        json_object_put(obj->json_data);
    obj->json_data = json_obj;
    obj->require_cleanup = false;
    return true;
}
bool JSON::getArrayBoolean(bool &value, int idx) {
    if (!json_data || !json_object_is_type(json_data, json_type_array)) return false;
    json_object *json_obj = json_object_array_get_idx(json_data, idx);
    if (!json_obj || !json_object_is_type(json_obj, json_type_boolean)) return false;
    value = json_object_get_boolean(json_obj);
    return true;
}
bool JSON::getArrayInteger(int &value, int idx) {
    if (!json_data || !json_object_is_type(json_data, json_type_array)) return false;
    json_object *json_obj = json_object_array_get_idx(json_data, idx);
    if (!json_obj || !json_object_is_type(json_obj, json_type_int)) return false;
    value = json_object_get_int(json_obj);
    return true;
}

void JSON::setObject(std::string field, JSON *obj) {
    json_object_object_add(json_data, field.c_str(), obj->json_data);
    obj->require_cleanup = false;
}
void JSON::setBoolean(std::string field, bool value) {
    json_object_object_add(json_data, field.c_str(), json_object_new_boolean(value));
}
void JSON::setInteger(std::string field, int value) {
    json_object_object_add(json_data, field.c_str(), json_object_new_int(value));
}
void JSON::setString(std::string field, std::string value) {
    json_object_object_add(json_data, field.c_str(), json_object_new_string(value.c_str()));
}

bool JSON::getObject(std::string field, JSON *obj) {
    json_object *json_obj = json_object_object_get(json_data, field.c_str());
    if (!json_obj || !json_object_is_type(json_obj, json_type_object)) return false;
    if (obj->json_data && obj->require_cleanup)
        json_object_put(obj->json_data);
    obj->json_data = json_obj;
    obj->require_cleanup = false;
    return true;
}
bool JSON::getBoolean(std::string field, bool &value) {
    json_object *obj = json_object_object_get(json_data, field.c_str());
    if (!obj || !json_object_is_type(obj, json_type_boolean)) return false;
    value = json_object_get_boolean(obj);
    return true;
}
bool JSON::getInteger(std::string field, int &value) {
    json_object *obj = json_object_object_get(json_data, field.c_str());
    if (!obj || !json_object_is_type(obj, json_type_int)) return false;
    value = json_object_get_int(obj);
    return true;
}
bool JSON::getString(std::string field, std::string &value) {
    json_object *obj = json_object_object_get(json_data, field.c_str());
    if (!obj || !json_object_is_type(obj, json_type_string)) return false;
    value = json_object_get_string(obj);
    return true;
}

std::string JSON::toString() {
    return json_object_to_json_string(json_data);
}
void JSON::clone(JSON *new_obj) {
    if (json_data) {
        new_obj->load(toString());
    }
}

}}
