#ifndef CCLIB_SANITY_TEST
#define CCLIB_SANITY_TEST

#define USERNAME "wukon_hsieh@acer.com.tw"
#define PASSWORD "1qaz2wsx3edc"

const char* getHomePath();
const char* getTestPath();
void runSanityTest();
void test_ccd_sync(int argc, const char ** argv);
#endif
