#include<cmath>
#include<iostream>
#include<regex>

#include "arxiv-classifier.hpp"
#include<curl/curl.h>

std::vector<word> dict;
int ham = 0;
int spam = 0;
double hamp = 0;
double spamp = 0;

const std::string TRAINING_FILE = "training_data.txt";
const std::string SUBJECTS_FILE = "subjects.txt";
const int MAX_RESULTS = 30;

const double SMOOTHING = 1;


// TODO use the return int to indicate errors
int load_training_data(std::string TRAINING_FILE) { 
    // read training data from file
    std::ifstream data_stream;
    data_stream.open(TRAINING_FILE);
    // load data into dict
    std::string line;
    if(data_stream.is_open()) {
        std::getline(data_stream, line);
        ham = std::stoi(line);
        std::getline(data_stream, line);
        spam = std::stoi(line);
        while(std::getline(data_stream, line)) {
            int i;
            word w;
            i = line.find('/');
            w.word = line.substr(0, i);
            line.erase(0, i + 1);
            i = line.find('/');
            w.ham_freq = std::stoi(line.substr(0, i));
            line.erase(0, i + 1);
            i = line.find('/');
            w.spam_freq = std::stoi(line.substr(0, i));
            dict.push_back(w);
        }
    }
    data_stream.close();

    hamp = ((double) ham) / (ham + spam);
    spamp = ((double) spam) / (ham + spam);
    return 0;
}

std::vector<std::string> load_subject_data(std::string subject_file) {
    // read subject list from data file
    std::cout << "Reading subject file." << std::endl;
    std::ifstream subject_stream("subjects.txt");
    std::vector<std::string> subject_vector;
    std::string subject;
    if(subject_stream.is_open())
        while(std::getline(subject_stream, subject))
            subject_vector.push_back(subject);
    else
        std::cout << "Error opening subject file!" << std::endl;
    subject_stream.close();
    std::cout << "Found subjects: ";
    for(std::string s : subject_vector)
        std::cout << s << ", ";
    std::cout << std::endl;
    return subject_vector;
}

std::string query_subjects(std::vector<std::string> &subject_vector) {
    std::string response_buffer;
    for(std::string s : subject_vector) {
        // TODO refactor max results into a variable
        std::string query = "http://export.arxiv.org/api/query?search_query=cat:" + s + "&sortBy=submittedDate&max_results=" + std::to_string(MAX_RESULTS);
        response_buffer += query_arXiv_api(query);
        response_buffer += "\n";
    }
    return response_buffer;
}

// write training data -- note: this will replace the contents of the output file!
int write_training_data(std::string out_file) {
    // write the training data
    std::cout << "Writing training data to file." << std::endl;
    std::ofstream ostream;
    ostream.open(out_file, std::ofstream::trunc);
    ostream << ham << '\n';
    ostream << spam << '\n';
    for(word w : dict)
        ostream << (w.word + "/" + std::to_string(w.ham_freq) + "/" + std::to_string(w.spam_freq) + '\n');
    ostream.close();
    std::cout << "Done." << std::endl;
    return 0;
}

static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*) userp)->append((char*) contents, size * nmemb);
    return size * nmemb;
}

std::string query_arXiv_api(std::string query) {
    std::cout << "Querying arXiv API: " << query << std::endl;
    CURL* curl;
    CURLcode res;
    std::string response_buffer;
    curl = curl_easy_init();
    if(!curl) {
        std::cout << "Error loading curl! Returning nullptr for pugi::xml_document." << std::endl;
        return "";
    }

    curl_easy_setopt(curl, CURLOPT_URL, query.data());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);
    res = curl_easy_perform(curl);
    response_buffer += "\n";
    if(res != CURLE_OK) {
        std::cout << "curl_easy_perform() failed:" << curl_easy_strerror(res) << std::endl;
        curl_easy_cleanup(curl);
        return "";
    }
    curl_easy_cleanup(curl);
    std::cout << "Finished querying arXiv API." << std::endl;

    return response_buffer;
}

std::string sanitize_summary(std::string summary) {
    // erase everything between $ $
    std::string latex_removed = std::regex_replace(summary, std::regex("\\$(.*?)\\$"), "");
    // replace newlines with spaces
    std::string newlines_removed = std::regex_replace(latex_removed, std::regex("\\n"), " ");
    // replace punctuation with spaces
    std::string punc_removed = std::regex_replace(newlines_removed, std::regex("[^\\w\\s]"), " ");
    // convert to lower case
    std::transform(punc_removed.begin(), punc_removed.end(), punc_removed.begin(), [](unsigned char c) { return std::tolower(c); });
    // TODO remove stop words?

    // add a space at the end for ease of iteration
    return punc_removed + " ";
}

// make sure the argument ends in a space
void add_words_in_summary(std::string s, bool interesting) {
    int pos = 0;
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
}


double compute_word_likelihood(std::string str, bool ham) {
    int tf = 0;
    int total = 0;
    for(word w : dict) {
        if(ham)
            total += w.ham_freq;
        else
            total += w.spam_freq;
        if(w.word == str) {
            if(ham)
                tf += w.ham_freq;
            else
                tf += w.spam_freq;
        }
    }
    return ((double) tf + SMOOTHING) / (total + SMOOTHING * dict.size());
}

double compute_log_posterior(std::string message, bool ham) {
    int pos = 0;
    std::string token;
    double message_likelihood_ham = 0;
    double message_likelihood_spam = 0;
    while((pos = message.find(" ")) != std::string::npos) {
        token = message.substr(0, pos);
        message.erase(0, pos + 1);
        //std::cout << "Found word " << token << std::endl;
        //std::cout << '\t' << compute_word_likelihood(token, true) << " vs. " << compute_word_likelihood(token, false) << std::endl;
        message_likelihood_ham += std::log2(compute_word_likelihood(token, true));
        message_likelihood_spam += std::log2(compute_word_likelihood(token, false));
    }
    message_likelihood_ham += std::log2(hamp);
    message_likelihood_spam += std::log2(spamp);
    if(ham)
        return message_likelihood_ham;
    return message_likelihood_spam;
}

bool is_interesting(std::string message) {
    return (compute_log_posterior(message, true) >= compute_log_posterior(message, false));
}

