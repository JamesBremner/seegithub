#include <iostream>
#include <fstream>
#include "curl/curl.h"
#include "raven_sqlite.h"
#include "json.h"

using namespace std;


string getUserPswd()
{
    raven::sqlite::cDB db(L"seegithub_teleIMRI.db");
    int r = db.Query("SELECT user FROM config;");
    if( r != 1 )
    {
        cout << "Cannot open db\n";
        exit(1);
    }
    return db.myResultA[0];


}

void WriteIssuesToDB( const char* j )
{
    json::Array root = json::Deserialize( std::string( j ) );
//    if (root.GetType() == json::NULLVal)
//    {
//        cout << "invalid JSON\n";
//             exit(1);
//    }
    cout << root.size() << " issues\n";

    raven::sqlite::cDB db(L"seegithub_teleIMRI.db");

    for( auto& i : root )
    {
        cout << (int)i["number"] << " " << i["title"].ToString() << "\n";
        db.Query("INSERT INTO issue VALUES ( %d, '%s' ); ",
                (int)i["number"], i["title"].ToString().c_str()  );
    }
}

struct MemoryStruct
{
    char *memory;
    size_t size;
};

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL)
    {
        /* out of memory! */
        cout << "not enough memory (realloc returned NULL)\n";
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}


int main()
{
    struct MemoryStruct chunk;
    chunk.memory = (char*)malloc(1);
    chunk.size = 0;

    // init the curl library
    CURL *curl = curl_easy_init();

    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    // specify destination
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/repos/JamesBremner/TeleIMRI/issues");
    //curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/repos/JamesBremner/TeleIMRI/issues/56/events");

    /* send all data to this function  */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

    /* we pass our 'chunk' struct to the callback function */
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);


    // disable SSL certificate checking
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    // authenticate
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "JamesBremner");
    curl_easy_setopt(curl, CURLOPT_USERPWD, getUserPswd().c_str() );

    // send the message ( blocks! )
    curl_easy_perform(curl);

    // tidy up

    curl_easy_cleanup(curl);


    //cout << chunk.memory << endl;
    ofstream of("test.txt");
    of << chunk.memory;
    of.close();

    WriteIssuesToDB( chunk.memory );

    return 0;
}
