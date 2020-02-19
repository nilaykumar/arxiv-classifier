#ifndef ARXIV_CLASSIFIER_H
#define ARXIV_CLASSIFIER_H

#include<fstream>
#include<string>
#include<vector>

struct word {
    std::string word;
    int ham_freq;
    int spam_freq;
};

extern std::vector<word> dict;
extern int ham;
extern int spam;
extern double hamp;
extern double spamp;

extern const std::string TRAINING_FILE;
extern const std::string SUBJECTS_FILE;

extern const double SMOOTHING;

inline std::ostream& operator<<(std::ostream& os, const word& w) {
    os << "(" << w.word << ", " << w.ham_freq << ", " << w.spam_freq << ")";
    return os;
}

int load_training_data(std::string);

std::vector<std::string> load_subject_data(std::string);

int write_training_data(std::string);

double compute_word_likelihood(std::string, bool);

double compute_log_posterior(std::string, bool);

bool is_interesting(std::string);

std::string query_arXiv_api(std::string);

std::string query_subjects(std::vector<std::string>&);

std::string sanitize_summary(std::string);

void add_words_in_summary(std::string, bool);

#endif // ARXIV_CLASSIFIER_H
