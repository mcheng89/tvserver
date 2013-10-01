#ifndef TV_UTILS_JSON_H
#define TV_UTILS_JSON_H

#include <string>
#include <json-c/json.h>

namespace TV {
namespace Utils {

    class JSON {
    public:
        enum JSONType {ARRAY, OBJECT};
        JSON(JSONType type=OBJECT, bool init=true);
        ~JSON();

        bool load(std::string data);

        void setArray(std::string field, JSON *obj);
        void addArrayObject(JSON *obj);
        void addArrayBoolean(bool value);

        bool getArray(std::string field, JSON *obj);
        int getArraySize();
        bool getArrayObject(JSON *obj, int idx);
        bool getArrayBoolean(bool &value, int idx);
        bool getArrayInteger(int &value, int idx);

        void setObject(std::string field, JSON *obj);
        void setBoolean(std::string field, bool value);
        void setInteger(std::string field, int value);
        void setString(std::string field, std::string value);

        bool getObject(std::string field, JSON *obj);
        bool getBoolean(std::string field, bool &value);
        bool getInteger(std::string field, int &value);
        bool getString(std::string field, std::string &value);

        std::string toString();
        void clone(JSON *new_obj);

    private:
        bool require_cleanup;
        json_object *json_data;

        JSON(const JSON&);
        JSON& operator = (const JSON&);
    };

}}

#endif // TV_UTILS_JSON_H
