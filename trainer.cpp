#include<iostream>
#include<fstream>
#include<regex>
#include<sstream>
#include<string>
#include<vector>

#include<curl/curl.h>
#include "pugixml/pugixml.hpp"

static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*) userp)->append((char*) contents, size * nmemb);
    return size * nmemb;
}

int main() {

    // read subject list from data file
    std::cout << "Reading subject file." << std::endl;
    std::ifstream subject_stream("subjects.txt");
    std::vector<std::string> subject_vector;
    std::string subject;
    if(subject_stream.is_open())
        while(getline(subject_stream, subject))
            subject_vector.push_back(subject);
    else {
        std::cout << "Error opening subject file! Aborting." << std::endl;
        return 1;
    }
    subject_stream.close();

    std::cout << "Found subjects: ";
    for(std::string s : subject_vector)
        std::cout << s << ", ";
    std::cout << std::endl;

    // query arXiv api for today's papers if haven't done so already
    std::cout << "Querying arXiv API." << std::endl;
    CURL* curl;
    CURLcode res;
    std::string response_buffer;
    curl = curl_easy_init();
    if(!curl) {
        std::cout << "Error loading curl! Aborting." << std::endl;
        return 1;
    }

    for(std::string s : subject_vector) {
        std::string query = "http://export.arxiv.org/api/query?search_query=cat:" + s + "&sortBy=submittedDate";
        curl_easy_setopt(curl, CURLOPT_URL, query.data());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);
        res = curl_easy_perform(curl);
        response_buffer += "\n";
        if(res != CURLE_OK) {
            std::cout << "curl_easy_perform() failed:" << curl_easy_strerror(res) << std::endl;
            curl_easy_cleanup(curl);
            return 1;
        }
    }
    curl_easy_cleanup(curl);
    std::cout << "Finished querying arXiv API." << std::endl;
    
    std::cout << "Parsing response." << std::endl;
    pugi::xml_document doc;
    std::stringstream xml_stream(response_buffer);
    pugi::xml_parse_result result = doc.load(xml_stream);

    std::vector<std::string> summary_vector;

    pugi::xml_node feed = doc.child("feed");
    for(std::string s : subject_vector) {
        for(pugi::xml_node entry = feed.child("entry"); entry; entry = entry.next_sibling("entry")) {
            // erase everything between $ $
            std::string latex_removed = std::regex_replace(entry.child("summary").child_value(), std::regex("\\$(.*?)\\$"), "");
            summary_vector.push_back(latex_removed);
        }
        feed = feed.next_sibling("feed");
    }

    for(std::string s : summary_vector)
        std::cout << s << std::endl << std::endl;


    // process titles, abstracts, and links

    // iterate through the papers, asking the user spam/ham

    // write results to data file

    return 0;
}
