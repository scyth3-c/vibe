//
// Created by scythe on 5/07/23.
//

#ifndef VIBE_ENUMS_H
#define VIBE_ENUMS_H

namespace enums {

    class neo {
    public:
        enum eReturn {
            OK = 0x0,
            ERROR = -0x1,
            NA = 0x2,
        };
        enum eSize {
            BUFFER = 0x800,
            SESSION = 0x1,
            DEF_PORT =  0xbb8,
            DEF_REG = 0x0,
            MIN_PORT = 0x3e8
        };
        enum eStatus {
            START = 0x1,
            STOP = 0x0
        };
        enum LISTEN_TYPE {
            WHILE,
            UNIQUE
        };
    };

} // enums

#endif //VIBE_ENUMS_H
