#include "tvdata.h"
#include "../utils/json.h"
#include "../utils/file.h"
#include "../utils/stringutils.h"

namespace TV {
namespace API {

void EPGService::handleRequest(HTTP::HTTPRequest *request) {
    bool handledRequest = false;
	//Logger::Info() << "Server: Client requested program guide data" << std::endl;
    server->epg_rwlock.readLock();
    //http://old.nabble.com/Simple-example-for-dummy-user-writing-C-code-td22840833.html
    char buffer[1024];
    std::string post_data = request->getPostData();
    Utils::JSON json_data(Utils::JSON::OBJECT, false);
    if (json_data.load(post_data.c_str())) {
        int channel_len, start_time, end_time;
        Utils::JSON channels(Utils::JSON::ARRAY, false);
        json_data.getInteger("start_time", start_time);
        json_data.getInteger("end_time", end_time);
        json_data.getArray("channels", &channels);
        channel_len = channels.getArraySize();
        if (channel_len && start_time && end_time) {
            std::string stmt_query = "SELECT channel_num, program_title, program_subtitle, program_descript, start_time, end_time FROM TVChannels "
                "INNER JOIN TVPrograms ON TVChannels.channel_id=TVPrograms.channel_id WHERE channel_num IN (";
            for (int i=0; i<channel_len; i++)
                stmt_query += (i==0?"?":",?");
            stmt_query += ") AND end_time>? AND start_time<? ORDER BY channel_num, start_time ASC";

            sqlite3_stmt * stmt;
            sqlite3_prepare_v2(server->epg_db, stmt_query.c_str(), -1, &stmt, NULL);
            if (stmt) {
                for (int i=0; i<channel_len; i++) {
                    int channel = 0;
                    channels.getArrayInteger(channel, i);
                    sqlite3_bind_int(stmt, i+1, channel);
                }
                sqlite3_bind_int(stmt, channel_len+1, start_time);
                sqlite3_bind_int(stmt, channel_len+2, end_time);
            }

            request->startChunked(HTTP::HTTPRequest::HTTP_OK);
            handledRequest = true;
            std::string buffer = "{";
            int current_channel = 0;
            while (stmt && sqlite3_step(stmt) == SQLITE_ROW) {
                int channel_num = sqlite3_column_int(stmt, 0);
                std::string program_title, program_subtitle, program_descript;
                program_title.append((char*)sqlite3_column_text(stmt, 1),sqlite3_column_bytes(stmt, 1));
                program_subtitle.append((char*)sqlite3_column_text(stmt, 2),sqlite3_column_bytes(stmt, 2));
                program_descript.append((char*)sqlite3_column_text(stmt, 3),sqlite3_column_bytes(stmt, 3));

                int start_time = sqlite3_column_int(stmt, 4);
                int end_time = sqlite3_column_int(stmt, 5);
                if (current_channel != channel_num) {
                    if (current_channel!=0) buffer += "],";
                    buffer += "\""+Utils::StringUtils::toString(channel_num)+"\":[";
                    current_channel = channel_num;
                } else buffer += ",";
                Utils::StringUtils::replaceAll(program_title,"\"","\\\"");
                Utils::StringUtils::replaceAll(program_descript,"\"","\\\"");
                buffer += "{\"program_title\":\""+program_title+
                        "\",\"program_subtitle\":\""+program_subtitle+
                        "\",\"program_descript\":\""+program_descript+
                        "\",\"start_time\":"+Utils::StringUtils::toString(start_time);
                buffer += ",\"end_time\":"+Utils::StringUtils::toString(end_time)+"}";

                if (buffer.size() > 1024) {
                    request->writeChunked(buffer);
                    buffer = "";
                }
            }
            if (current_channel!=0) buffer += "]";
            buffer += "}";
            request->writeChunked(buffer);
            request->endChunked();

            if (stmt) {
                sqlite3_clear_bindings(stmt);
                sqlite3_finalize(stmt);
            }
        }
    }
    server->epg_rwlock.readUnlock();

    if (!handledRequest) request->sendResponse(HTTP::HTTPRequest::HTTP_OK, "{}");
}

void ChannelListService::handleRequest(HTTP::HTTPRequest *request) {
    bool json = false;
    if ( request->getParameter("format") == "json")
        json = true;
    Devices::Device *device = server->getDevices().front();
    if (!json) {
        std::string device_type = device->getDeviceType();
        long last_modified = 0;
        char strLastModified[11];
        if (request->getParameter("last_modified") != "")
            last_modified = atol(request->getParameter("last_modified").c_str());
        if (last_modified < Utils::File::lastModified("resources/ChannelMap-"+device_type+".csv"))
            request->sendFile("resources/ChannelMap-"+device_type+".csv");
        else request->sendResponse(HTTP::HTTPRequest::HTTP_OK, "");
    } else {
        int channel_limit = 10;
        int channel_page = 0;
        int channel_size = device->getChannelMap().size();
        if ( request->getParameter("page") != "" )
            channel_page = atoi(request->getParameter("page").c_str())-1;
        if ( request->getParameter("limit") != "" ) {
            channel_limit = atoi(request->getParameter("limit").c_str());
            if (channel_limit <= 0) {
                channel_limit = channel_size;
                channel_page = 0;
            }
        }
        int channel_start = channel_page*channel_limit;
        int channel_end = (channel_page+1)*channel_limit;
        int num_pages = channel_limit==0?1:
            channel_size/channel_limit+(channel_size-(channel_size/channel_limit)*channel_limit>0?1:0);

        request->startChunked(HTTP::HTTPRequest::HTTP_OK);

        std::string buffer = "{\"limit\":"+Utils::StringUtils::toString(channel_limit)+",";
        buffer += "\"num_pages\":"+Utils::StringUtils::toString(num_pages)+",";
        buffer += "\"page\":"+Utils::StringUtils::toString(channel_page+1)+",\"channels\":[";
        int i=channel_start;
        while (i<channel_size && i>=0 && i<channel_end) {
            std::string channel_name = device->getChannelMap().at(i)->name;
            Utils::StringUtils::replaceAll(channel_name,"\"","\\\"");
            buffer += std::string(i==channel_start?"":",")+"{\"number\":\""+
                Utils::StringUtils::toString(device->getChannelMap().at(i)->number)+"\","+
                "\"name\":\""+channel_name+"\"}";

            if (buffer.size() > 1024) {
                request->writeChunked(buffer);
                buffer = "";
            }
            i++;
        }
        buffer += "]}";
        request->writeChunked(buffer);
        request->endChunked();
    }
}

}}
