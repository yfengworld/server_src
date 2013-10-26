#ifndef CMD_H_INCLUDED
#define CMD_H_INCLUDED

/*

topology:
      
      -------------------------> login
      |                            ^
      |                            |
      |                            |-----------------
      |                                             |
   client ----------------------> gate ---------> center
                                    |               ^
                                    |               |
                                    |               |
                                    -------------> game --------------> cache
client          'c'
login           'l'
gate            'g'
center          'e'
game            'm"
cache           'a'

*/

enum
{

    /********************************* login -> center *********************************/
    LE_BEGIN = 1,
    LE_END,

    /********************************* center -> login *********************************/
    EL_BEGIN = 21,
    EL_END,

    /********************************* gate -> game *********************************/
    GM_BEGIN = 41,
    GM_END,

    /********************************* game -> gate *********************************/
    MG_BEGIN = 61,
    MG_END,

    /********************************* gate -> center *********************************/
    GE_BEGIN = 81,
    GE_END,

    /********************************* center -> gate *********************************/
    EG_BEGIN = 101,
    EG_END,

    /********************************* gate -> cache *********************************/
    GA_BEGIN = 121,
    GA_END,

    /********************************* cache -> gate *********************************/
    AG_BEGIN = 141,
    AG_END,

    /********************************* game -> center *********************************/
    ME_BEGIN = 161,
    ME_END,

    /********************************* center -> game *********************************/
    EM_BEGIN = 181,
    EM_END,

    /********************************* game -> cache *********************************/
    MA_BEGIN = 201,
    MA_END,

    /********************************* cache -> game *********************************/
    AM_BEGIN = 221,
    AM_END,

    /************************
     * external divding line
     * **********************/

    CS_BEGIN = 1000,

    /********************************* client -> login *********************************/
    CL_BEGIN = CS_BEGIN,
    CL_END,

    /********************************* client -> gate *********************************/
    CG_BEGIN = 1011,
    CG_END,

    /********************************* client -> center *********************************/
    CE_BEGIN = 1021,
    CE_END,

    /********************************* client -> cache *********************************/
    CA_BEGIN = 1031,
    CA_END,

    /********************************* client -> game *********************************/
    CM_BEGIN = 1041,
    CM_END,

    SC_BEGIN = 2000,

    /********************************* login -> client *********************************/
    LC_BEGIN = SC_BEGIN,
    LC_END,

    /********************************* gate -> client *********************************/
    GC_BEGIN = 2011,
    GC_END,

    /********************************* center -> client *********************************/
    EC_BEGIN = 2021,
    EC_END,

    /********************************* cache -> client *********************************/
    AC_BEGIN = 2031,
    AC_END,

    /********************************* game -> client *********************************/
    MC_BEGIN = 2041,
    MC_END,

    SC_END = MC_END
};

int check_cmd();

#endif /* CMD_H_INCLUDED */
