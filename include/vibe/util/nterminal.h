//
// Created by scythe on 6/4/24.
//
#ifndef NTERMINAL_H
#define NTERMINAL_H
#include <iostream>

constexpr auto VB_NVALUE = -1;
constexpr auto VB_OK = 0;

constexpr auto VB_EPOLL_RANGE =  "AN ERROR OCCURRED WHEN CREATION OF THE EPOLL_FD FILE DESCRIPTOR";
constexpr auto VB_EPOLL_CTL =    "AN ERROR OCCURRED WHEN EXECUTING EPOLL_CTL";
constexpr auto VB_EPOLL_CERR =   "THE FOLLOWING ERROR WAS FOUND WHEN EXECUTING THE EPOLL METHOD: ";

constexpr auto VB_SOCKET_FAIL =  "A PROBLEM OCCURRED WHEN READING THE SOCKET REQUEST, CHECK THE CONNECTION";
constexpr auto VB_SOCKET_CLOSE = "A PROBLEM OCCURRED WHEN TRYING TO CLOSE THE CONNECTION WITH THE SOCKET: normally it is due to an error in the previous code";

constexpr auto VB_MAIN_THREAD =  "AN ERROR OCCURRED IN THE MAIN PROCESSING THREAD";


template<class...P>
auto terminal(P const&... args) -> void {
    ((std::cout<<"[ "<<args<<"]"<<std::endl),...);
}




#endif //NTERMINAL_H
