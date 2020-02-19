#include<algorithm>
#include<cctype>
#include<iostream>
#include<sstream>

#include "pugixml.hpp"
#include "arxiv-classifier.hpp"

int main() {
    load_training_data(TRAINING_FILE);

    std::vector<std::string> subject_vector = load_subject_data(SUBJECTS_FILE); 

    std::string response_buffer = query_subjects(subject_vector);
        
    pugi::xml_document doc;
    std::stringstream xml_stream(response_buffer);
    pugi::xml_parse_result result = doc.load(xml_stream);

    std::vector<std::string> summary_vector;
    std::vector<std::string> raw_summary_vector;
    std::vector<std::string> title_vector;
    std::vector<std::string> id_vector;
    std::vector<std::string> published_vector;

    pugi::xml_node feed = doc.child("feed");
    // obtain the current day from the response
    std::string today(feed.child("updated").child_value());
    today = today.substr(0, 10);
    // TODO for testing purposes: hardcode one day in
    today = "2020-02-18";
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
            std::string raw_summary = entry.child("summary").child_value();
            raw_summary_vector.push_back(raw_summary);
            std::string sanitized_summary = sanitize_summary(raw_summary);
            summary_vector.push_back(sanitized_summary);

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

    // training time!
    for(int i = 0; i < summary_vector.size(); i++) {
        std::cout << title_vector.at(i) << std::endl;
        std::cout << "---------------" << std::endl;
        std::cout << raw_summary_vector.at(i) << std::endl << std::endl;

        // guess based on training so far whether this might be interesting
        if(dict.size() != 0) {
            double ham_pos = compute_log_posterior(summary_vector.at(i), true);
            double spam_pos = compute_log_posterior(summary_vector.at(i), false);
            std::string guess;
            if(ham_pos >= spam_pos)
                guess = "interesting";
            else
                guess = "not interesting";
            std::cout << "My guess is that you will find this " << guess << " (" << ham_pos << ", " << spam_pos << ")." << std::endl;
        }

        std::cout << "Interesting or not? (y/n): ";

        std::string input;
        std::getline(std::cin, input);
        bool interesting = (tolower(input.front()) == 'y');
        if(!interesting) {
            std::cout << std::endl << "Marked as NOT INTERESTING." << std::endl;
            spam ++;
        } else {
            std::cout << std::endl << "Marked as INTERESTING." << std::endl;
            ham ++;
        }

        add_words_in_summary(summary_vector.at(i), interesting);
        std::cout << std::endl << std::endl << std::endl;
    }

    write_training_data(TRAINING_FILE);

    return 0;
}
