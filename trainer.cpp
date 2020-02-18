#include<algorithm>
#include<cctype>
#include<iostream>
#include<fstream>
#include<regex>
#include<sstream>
#include<string>
#include<vector>

#include<curl/curl.h>
#include "pugixml.hpp"

static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*) userp)->append((char*) contents, size * nmemb);
    return size * nmemb;
}

struct word {
    std::string word;
    int ham_freq;
    int spam_freq;
};

std::ostream& operator<<(std::ostream& os, const word& w) {
    os << "(" << w.word << ", " << w.ham_freq << ", " << w.spam_freq << ")";
    return os;
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
        std::string query = "http://export.arxiv.org/api/query?search_query=cat:" + s + "&sortBy=submittedDate&max_results=30";
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
    
    pugi::xml_document doc;
    std::stringstream xml_stream(response_buffer);
    pugi::xml_parse_result result = doc.load(xml_stream);

    std::vector<std::string> summary_vector;
    std::vector<std::string> raw_summary_vector;
    std::vector<std::string> title_vector;
    std::vector<std::string> id_vector;
    std::vector<std::string> published_vector;

    // TODO make sure we aren't double counting papers by using the ID

    pugi::xml_node feed = doc.child("feed");
    // obtain the current day from the response
    std::string today(feed.child("updated").child_value());
    today = today.substr(0, 10);
    // TODO for testing purposes: hardcode one day in
    today = "2020-02-17";
    std::cout << "Parsing results from today, " << today << std::endl;
    for(std::string s : subject_vector) {
        for(pugi::xml_node entry = feed.child("entry"); entry; entry = entry.next_sibling("entry")) {
            std::string date(entry.child("published").child_value());
            // only look at articles from today
            if(date.substr(0, 10) != today) 
                continue;
            // make sure this is not a cross-post that we are double-counting
            if(std::find(id_vector.begin(), id_vector.end(), entry.child("id").child_value()) != id_vector.end())
                continue;
            // keep the unparsed summary for user interaction
            raw_summary_vector.push_back(entry.child("summary").child_value());
            // erase everything between $ $
            std::string latex_removed = std::regex_replace(entry.child("summary").child_value(), std::regex("\\$(.*?)\\$"), "");
            // replace newlines with spaces
            std::string newlines_removed = std::regex_replace(latex_removed, std::regex("\\n"), " ");
            // remove punctuation
            std::string punc_removed = std::regex_replace(newlines_removed, std::regex("[^\\w\\s]"), "");
            // convert to lower case
            std::transform(punc_removed.begin(), punc_removed.end(), punc_removed.begin(), [](unsigned char c) { return std::tolower(c); });
            // TODO remove stop words?
            summary_vector.push_back(punc_removed);

            // populate auxiliary data
            title_vector.push_back(entry.child("title").child_value());
            id_vector.push_back(entry.child("id").child_value());
            published_vector.push_back(entry.child("published").child_value());
        }
        // go to the next subject
        feed = feed.next_sibling("feed");
    }

    std::cout << "Found " << summary_vector.size() << " results." << std::endl;
    std::cout << "Initiating user-based training." << std::endl << std::endl;
    // create dictionary of words
    std::vector<word> dict;

    // training time!
    for(int i = 0; i < summary_vector.size(); i++) {
        std::cout << title_vector.at(i) << std::endl;
        std::cout << "---------------" << std::endl;
        std::cout << raw_summary_vector.at(i) << std::endl << std::endl;
        std::cout << "Interesting or not? (y/n): ";
        std::string input;
        std::getline(std::cin, input);
        bool interesting = (tolower(input.front()) == 'y');
        if(!interesting)
            std::cout << std::endl << "Marked as NOT INTERESTING." << std::endl;
        else
            std::cout << std::endl << "Marked as INTERESTING." << std::endl;
        // loop through each non-null word in the summary
        int pos = 0;
        std::string s(summary_vector.at(i));
        std::string token;
        while((pos = s.find(' ')) != std::string::npos) {
            token = s.substr(0, pos);
            s.erase(0, pos + 1);
            if(token != "") {
                // check if already in dictionary
                bool exists = false;
                int existsAt = -1;
                for(int j = 0; j < dict.size(); j++)
                    if(dict.at(j).word == token) {
                        exists = true;
                        existsAt = j;
                    }
                if(exists) {
                    if(interesting)
                        dict.at(existsAt).ham_freq ++;
                    else
                        dict.at(existsAt).spam_freq ++;
                } else {
                    word w;
                    w.word = token;
                    if(interesting) {
                        w.ham_freq = 1;
                        w.spam_freq = 0;
                    } else {
                        w.ham_freq = 0;
                        w.spam_freq = 1;
                    }
                    dict.push_back(w);
                }
            }
        }
        /*
        std::cout << std::endl;
        for(word w : dict)
            std::cout << w;
        std::cout << std::endl;
        */
        std::cout << std::endl << std::endl << std::endl;
    }

    std::cout << std::endl;
    for(word w : dict)
        std::cout << w;
    std::cout << std::endl;


    // append results to data file

    return 0;
}
