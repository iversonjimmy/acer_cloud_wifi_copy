#include "http_request.hpp"

#include <iostream>
#include <string>
#include <map>
#include <cstdlib>

using namespace std;

static void show_map(const http_request::query_list &m)
{
    http_request::query_list::const_iterator it;
    for (it = m.begin(); it != m.end(); it++) {
        cout << it->first << "=" << it->second << endl;
    }
}

static int test(const string &uri, 
                const http_request::query_list &ans)
{
    int nfailed = 0;  // number of failures encountered

    http_request req;
    string input;
    input = "GET " + uri + "\r\n\r\n";
    req.receive(input.data(), input.size());

    bool show_maps = false;

    if (req.query != ans) {
        cout << "ERROR: unexpected parse result" << endl;
        nfailed++;
        show_maps = true;
    }

    if (show_maps) {
        cout << "parsed:" << endl;
        show_map(req.query);
        cout << "expected:" << endl;
        show_map(ans);
    }

    return nfailed;
}

int main(int argc, char *argv[])
{
    int nfailed = 0;

    http_request::query_list ans;

    nfailed += test("/rf/rd", ans);
    nfailed += test("/rf/rd?", ans);
    nfailed += test("/rf/rd?=", ans);

    ans["abc"] = "";
    nfailed += test("/tf/rd?abc", ans);
    nfailed += test("/tf/rd?abc=", ans);
    nfailed += test("/tf/rd?abc&", ans);
    nfailed += test("/tf/rd?abc=&", ans);
    nfailed += test("/tf/rd?abc;", ans);
    nfailed += test("/tf/rd?abc=;", ans);

    ans["abc"] = "123";
    nfailed += test("/tf/rd?abc=123", ans);
    nfailed += test("/tf/rd?abc=123&", ans);
    nfailed += test("/tf/rd?abc=123;", ans);

    ans["def"] = "456";
    nfailed += test("/tf/rd?abc=123&def=456", ans);
    nfailed += test("/tf/rd?abc=123;def=456", ans);
    nfailed += test("/tf/rd?abc=123&def=456;", ans);
    nfailed += test("/tf/rd?abc=123;def=456&", ans);

    // this should skip zzz=11=22
    nfailed += test("/tf/rd?abc=123&zzz=11=22&def=456", ans);

    ans["abc"] = "1 3";
    nfailed += test("/tf/rd?abc=1+3&def=456", ans);
    nfailed += test("/tf/rd?abc=%31+3&def=456", ans);

    cout << "TC_RESULT=" << (nfailed > 0 ? "FAIL" : "PASS") << " ;;; TC_NAME=test_parse_query_string" << endl;

    return nfailed;
}
