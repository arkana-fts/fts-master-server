#ifndef D_DB_H
#define D_DB_H

#include "Mutex.h"

#include <list>
#if defined(_MSC_VER)
#  include <winsock.h>
#  include <mysql.h>
#else
#  include <mysql/mysql.h>
#endif
#include "constants.h"

namespace FTSSrv2 {

class DataBase {
private:
    MYSQL *m_pSQL;

    FTS::Mutex m_mutex;

    static std::string m_psTblUsrFields[DSRV_TBL_USR_COUNT];
    static std::string m_psTblChansFields[DSRV_TBL_CHANS_COUNT];
    static std::string m_psTblChanOpsFields[DSRV_VIEW_CHANOPS_COUNT];
    static std::string m_psTblFeedbackFields[DSRV_TBL_FEEDBACK_COUNT];

public:
    DataBase();
    virtual ~DataBase();

    int init();
    int free(MYSQL_RES *&out_pRes);

    std::string escape( const std::string & in_sStr );
    std::string getError();

    int buildupDataBase();

    bool query(MYSQL_RES *&out_pRes, std::string in_pszQuery);

    int storedFunctionInt(std::string in_pszFunc, std::string in_pszArgs);
    bool storedProcedure(MYSQL_RES *&out_pRes, std::string in_pszProc, std::string in_pszArgs);

    // Singleton-like stuff.
    static int initUniqueDB();
    static DataBase *getUniqueDB();
    static int deinitUniqueDB();

    inline static std::string TblUsrField(int i) {if(i >= DSRV_TBL_USR_COUNT) return ""; else return m_psTblUsrFields[i];};
    inline static std::string TblChansField(int i) {if(i >= DSRV_TBL_CHANS_COUNT) return ""; else return m_psTblChansFields[i];};
    inline static std::string TblChanOpsField(int i) {if(i >= DSRV_VIEW_CHANOPS_COUNT) return ""; else return m_psTblChanOpsFields[i];};
    inline static std::string TblFeedbackField(int i) {if(i >= DSRV_TBL_FEEDBACK_COUNT) return ""; else return m_psTblFeedbackFields[i];};
};

} // namespace FTSSrv2

#endif                          /* D_DB_H */
