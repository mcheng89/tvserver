#include "epgupdater.h"
#include "xmltvreader.h"
#include "../utils/file.h"

#include "../tv.h"
#include "../log.h"
#include "../sqlite/sqlite3.h"
#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/framework/XMLPScanToken.hpp>

namespace TV {
namespace XMLTV {

EPGUpdater::EPGUpdater(Server *parent) {
    this->parent = parent;
    next_time = 0;
    runOnce = true;
    runNext = false;
    isRunning = false;
}

bool EPGUpdater::start() {
    startup_mutex.lock();
    Threads::Thread::start();
    runOnce = false;
    runNext = true;
    isRunning = true;
    startup_mutex.unlock();
    return true;
}

void EPGUpdater::run() {
    update_mutex.lock();
    Log::Info() << "EPGUpdater: Running EPGUpdater.bat script" << std::endl;
    xmltv_updater.execute("C:\\Windows\\System32\\cmd.exe","/c EPGUpdater.bat",true);
    std::string epglog = xmltv_updater.readOutput();
    Log::Info() << epglog << std::endl;
    //replaceAll(epglog, "\r\n", "\n");
    Log::Info() << "EPGUpdater: EPGUpdater.bat script completed" << std::endl;

    /*FILE *f = fopen("logs/LOG-EPGUpdater.txt", "a");
    fwrite(epglog.c_str(),1,epglog.size(),f);
    fclose(f);*/
    update_mutex.unlock();

    update_mutex.lock();
    if (isRunning) {
        Log::Info() << "EPGUpdater: Updating guide database from xmltv" << std::endl;
        remove("resources/epgdata.db.tmp");
        //http://stackoverflow.com/questions/1711631/how-do-i-improve-the-performance-of-sqlite
        sqlite3 *db;
        sqlite3_stmt * stmtChannels;
        sqlite3_stmt * stmtPrograms;
        char * dbErrMsg = 0;
        sqlite3_open("resources/epgdata.db.tmp", &db);
        sqlite3_exec(db, "PRAGMA synchronous = OFF", NULL, NULL, &dbErrMsg);
        sqlite3_exec(db, "PRAGMA journal_mode = OFF", NULL, NULL, &dbErrMsg);
        sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &dbErrMsg);
        //channels table: channel number, channel name, channel id
        sqlite3_exec(db, "CREATE TABLE TVChannels (channel_num INTEGER PRIMARY KEY, channel_name TEXT, channel_id TEXT)", NULL, NULL, &dbErrMsg);
        sqlite3_prepare_v2(db, "INSERT INTO TVChannels (channel_num,channel_name,channel_id) VALUES (?,?,?)", -1, &stmtChannels, NULL);
        //programs table: channel id, program title, start time, end time
        sqlite3_exec(db, "CREATE TABLE TVPrograms (channel_id TEXT, program_title TEXT, program_subtitle TEXT, program_descript TEXT, start_time INTEGER, end_time INTEGER, zap2it_id TEXT)", NULL, NULL, &dbErrMsg);
        sqlite3_prepare_v2(db, "INSERT INTO TVPrograms (channel_id,program_title,program_subtitle,program_descript,start_time,end_time,zap2it_id) VALUES (?,?,?,?,?,?,?)", -1, &stmtPrograms, NULL);

        xercesc::SAX2XMLReader *parser = 0;
        try {
            parser = xercesc::XMLReaderFactory::createXMLReader();
            parser->setFeature(xercesc::XMLString::transcode("http://xml.org/sax/features/namespaces"),false);
            parser->setFeature(xercesc::XMLString::transcode("http://apache.org/xml/features/nonvalidating/load-external-dtd"),false);
            parser->setFeature(xercesc::XMLString::transcode("http://apache.org/xml/features/validation/schema/skip-dtd-validation"),true);
            XMLTV_SAXHandler docHandler;
            parser->setContentHandler(&docHandler);

            xercesc::XMLPScanToken token;
            //first tag should be tv tag - safe to ignore :)
            if (parser->parseFirst("resources/xmltv.xml", token))
                Log::Info() << "EPGUpdater: Starting to parse xmltv data. This might take some time!" << std::endl;
            // When parseNext() returns false, the XML file is compeltely read.
            while (parser->parseNext(token)) {
                if (docHandler.getParseState() == DONECHANNEL) {
                    struct ChannelInfo channelInfo = docHandler.getChannelInfo();
                    //Logger::Info() << "EPGUpdater: ChannelInfo[number=" << channelInfo.number << ",name=" << channelInfo.name << ",id=" << channelInfo.id << "]" << std::endl;
                    if (channelInfo.number != 0) {
                        sqlite3_bind_int(stmtChannels, 1, channelInfo.number);
                        sqlite3_bind_text(stmtChannels, 2, channelInfo.name.c_str(), -1, SQLITE_TRANSIENT);
                        sqlite3_bind_text(stmtChannels, 3, channelInfo.id.c_str(), -1, SQLITE_TRANSIENT);
                        sqlite3_step(stmtChannels);
                        sqlite3_clear_bindings(stmtChannels);
                        sqlite3_reset(stmtChannels);
                    }
                } else if (docHandler.getParseState() == DONEPROGRAM) {
                    struct ProgramInfo programInfo = docHandler.getProgramInfo();
                    //Logger::Info() << "EPGUpdater: ProgramInfo[channel=" << programInfo.channel_id << ",title=" << programInfo.title << ",start_time=" << programInfo.start_time << "]" << std::endl;
                    sqlite3_bind_text(stmtPrograms, 1, programInfo.channel_id.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_text(stmtPrograms, 2, programInfo.title.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_text(stmtPrograms, 3, programInfo.subtitle.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_text(stmtPrograms, 4, programInfo.description.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_int(stmtPrograms, 5, programInfo.start_time);
                    sqlite3_bind_int(stmtPrograms, 6, programInfo.end_time);
                    sqlite3_bind_text(stmtPrograms, 7, programInfo.zap2it.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_step(stmtPrograms);
                    sqlite3_clear_bindings(stmtPrograms);
                    sqlite3_reset(stmtPrograms);
                }
            }
        } catch (const xercesc::XMLException& toCatch) {
            char* message = xercesc::XMLString::transcode(toCatch.getMessage());
            Log::Info() << "EPGUpdater: Error - " << message << std::endl;
            xercesc::XMLString::release(&message);
        } catch (const xercesc::SAXParseException& toCatch) {
            char* message = xercesc::XMLString::transcode(toCatch.getMessage());
            Log::Info() << "EPGUpdater: Error - " << message << std::endl;
            xercesc::XMLString::release(&message);
        }
        if (parser) delete parser;

        sqlite3_exec(db, "CREATE INDEX UNIQUE channel_num_idx ON TVChannels(channel_num)", NULL, NULL, &dbErrMsg);
        sqlite3_exec(db, "CREATE INDEX channel_id_idx ON TVPrograms(channel_id)", NULL, NULL, &dbErrMsg);
        sqlite3_exec(db, "CREATE INDEX start_time_idx ON TVPrograms(start_time)", NULL, NULL, &dbErrMsg);
        sqlite3_exec(db, "CREATE INDEX end_time_idx ON TVPrograms(end_time)", NULL, NULL, &dbErrMsg);
        sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &dbErrMsg);
        sqlite3_finalize(stmtPrograms); //prepared statement
        sqlite3_finalize(stmtChannels); //prepared statement
        sqlite3_close(db);

        parent->epg_rwlock.writeLock();
        sqlite3_close(parent->epg_db);
        remove("resources/epgdata.db");
        rename("resources/epgdata.db.tmp", "resources/epgdata.db");
        sqlite3_open("resources/epgdata.db", &parent->epg_db);
        parent->epg_rwlock.writeUnlock();
        Log::Info() << "EPGUpdater: Completed update of guide data" << std::endl;
    } else {
        Log::Info() << "EPGUpdater: Shutting down. Guide update skipped" << std::endl;
    }
    isRunning = false;
    update_mutex.unlock();
}

bool EPGUpdater::stop(bool shutdown) {
    startup_mutex.lock();
    if (isRunning) {
        if (shutdown)
            Log::Info() << "EPGUpdater: Waiting for epg update to complete" << std::endl;
        update_mutex.lock();
        isRunning = false;
        update_mutex.unlock();
        Threads::Thread::stop();
    }
    startup_mutex.unlock();
    return true;
}

time_t EPGUpdater::nextRunTime(time_t current_time) {
    struct tm *calc_time = localtime(&current_time);
    int EPG_HOUR_OF_DAY = parent->getSettings()->getEPGHourOfDay();
    if (calc_time->tm_hour > EPG_HOUR_OF_DAY || (calc_time->tm_hour==EPG_HOUR_OF_DAY && (calc_time->tm_min>0 || calc_time->tm_sec>0)))
        calc_time->tm_mday += 1;
    calc_time->tm_hour = EPG_HOUR_OF_DAY;
    calc_time->tm_min = 0;
    calc_time->tm_sec = 0;
    time_t update_time = mktime(calc_time);

    bool epg_stale = !Utils::File::exists("resources/epgdata.db") ||
                    (current_time-Utils::File::lastModified("resources/epgdata.db") > 86400);
    if (runOnce && !isRunning && epg_stale) {
        next_time = update_time+86400;
        return current_time;
    }
    if (next_time==0 || (runNext && next_time < update_time)) {
    //dont modify next_time until we actually run
        runNext = false;
        next_time = update_time;
    } else if (runNext && next_time == update_time)
    //case when epg is running and current time is next_time, force an extra dayy
        nextRunTime(update_time+1);
    return next_time;
}

}}
