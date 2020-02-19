#include<algorithm>
#include<cmath>
#include<iostream>
#include<sstream>

#include "pugixml.hpp"
#include "arxiv-classifier.hpp"

int main() {
    std::cout << "Using training data stored in " + TRAINING_FILE << std::endl;
    load_training_data(TRAINING_FILE);

    std::vector<std::string> subject_vector = load_subject_data(SUBJECTS_FILE); 

    std::string response_buffer = query_subjects(subject_vector);


    // TODO the next like 40 lines should probably be refactored as they appear in trainer.cpp
    pugi::xml_document doc;
    std::stringstream xml_stream(response_buffer);
    pugi::xml_parse_result result = doc.load(xml_stream);

    pugi::xml_node feed = doc.child("feed");
    // obtain the current day from the response
    std::string today(feed.child("updated").child_value());
    today = today.substr(0, 10);
    // TODO for testing purposes: hardcode one day in
    today = "2020-02-18";

    std::vector<std::string> raw_summary_vector;
    std::vector<std::string> summary_vector;
    std::vector<std::string> title_vector;
    std::vector<std::string> id_vector;

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
        }
        // go to the next subject
        feed = feed.next_sibling("feed");
    }
    std::cout << "Found " << summary_vector.size() << " results." << std::endl;

    // loop through articles and display those that are interesting
    for(int i = 0; i < summary_vector.size(); ++i) {
        bool interesting = is_interesting(summary_vector.at(i));
        if(!interesting)
            continue;
        std::cout << std::endl;
        std::cout << "Found an interesting article: " << id_vector.at(i) << std::endl;
        std::cout << title_vector.at(i) << std::endl;
        std::cout << raw_summary_vector.at(i) << std::endl << std::endl << std::endl;
    }


    return 0;
}
