#ifndef HEADERS_H
#define HEADERS_H

#include "../include/vermell/vermell.h"
#include "../include/vermell/util/sysprocess.h"
#include "utils/min.veridic.h"
#include <future>
#include <string>
#include <gtest/gtest.h>
#include <memory>
#include <regex>

using std::string, std::future, std::shared_ptr, std::make_shared;
#define ISOLATE(logic) std::future<void> isolate_method = std::async(std::launch::async, ([&]() { logic }));

class TestSuite : public ::testing::Test {

  protected:

    static shared_ptr<Veridic> http;
	static string url, expected_default;

     static void SetUpTestCase(){

     	url = "http://localhost:8080";
        expected_default = "success";
        http = make_shared<Veridic>(url);

     }
};

string TestSuite::url;
string TestSuite::expected_default;
shared_ptr<Veridic> TestSuite::http;


#endif //HEADERS_H
