#ifndef D_DB_H
#define D_DB_H

#include <mutex>

#include <list>
#include <vector>
#include <tuple>

#include "constants.h"

namespace FTSSrv2 {
struct db_result;
struct db_ptr;

class DataBase {
private:
    db_ptr *m_pSQL;

    std::recursive_mutex m_mutex;

    std::string m_psTblUsrFields[DSRV_TBL_USR_COUNT];
    std::string m_psTblChansFields[DSRV_TBL_CHANS_COUNT];
    std::string m_psTblChanOpsFields[DSRV_VIEW_CHANOPS_COUNT];
    std::string m_psTblFeedbackFields[DSRV_TBL_FEEDBACK_COUNT];

    int free(db_result *&out_pRes);
    std::string escape(const std::string & in_sStr);
    std::string getError();
    int buildupDataBase();
    bool query(db_result *&out_pRes, std::string in_pszQuery);
    int storedFunctionInt(std::string in_pszFunc, std::string in_pszArgs);
    bool storedProcedure(db_result *&out_pRes, std::string in_pszProc, std::string in_pszArgs);

    std::string TblUsrField(int i) { if( i >= DSRV_TBL_USR_COUNT ) return ""; else return m_psTblUsrFields[i]; };
    std::string TblChansField(int i) { if( i >= DSRV_TBL_CHANS_COUNT ) return ""; else return m_psTblChansFields[i]; };
    std::string TblChanOpsField(int i) { if( i >= DSRV_VIEW_CHANOPS_COUNT ) return ""; else return m_psTblChanOpsFields[i]; };
    std::string TblFeedbackField(int i) { if( i >= DSRV_TBL_FEEDBACK_COUNT ) return ""; else return m_psTblFeedbackFields[i]; };
    

public:
    DataBase();
    DataBase(const DataBase& other) = delete;
    DataBase& operator=(const DataBase& other) = delete;
    DataBase(DataBase&& other) = delete;
    DataBase& operator=(DataBase&& other) = delete;
    ~DataBase();

    int init();

    std::vector<std::tuple<int, bool, std::string, std::string, std::string>> getChannels();
    std::vector<std::tuple<std::string, std::string>> getChannelOperators();
    bool channelCreate(const std::tuple<bool, std::string, std::string, std::string>& record);
    bool channelUpdate(const std::tuple<int, bool, std::string, std::string, std::string>& record);
    void channelAddOp(const std::tuple<std::string, int>& record);
    int channelDestroy(const std::tuple<std::string, std::string>& record);
    int disconnect(const std::tuple<std::string, std::string, std::string, int>& record);
    int connect(const std::tuple<std::string, std::string, std::string, int>& record);
    int signup(const std::tuple<std::string, std::string, std::string>& record);
    int insertFeedback(const std::tuple<std::string, std::string>& record);
    int updatePlayerSet(const std::tuple<std::uint8_t, std::string, std::string, std::string>& record);
    std::tuple<int, std::string> getUserPropertyNo(const std::tuple<std::uint8_t, std::string, std::string>& record);
    int setUserFlag(const std::tuple<std::string, std::uint32_t, bool>& record);
    int updateLocation(const std::tuple<std::string, std::string>& record);
};

} // namespace FTSSrv2

#endif                          /* D_DB_H */
