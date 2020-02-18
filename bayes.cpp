#include<cmath>
#include<iostream>
#include<fstream>
#include<string>
#include<vector>

struct word {
    std::string word;
    int ham_freq;
    int spam_freq;
};

std::vector<word> dict;
int ham = 0;
int spam = 0;
double hamp = 0;
double spamp = 0;

const double smoothing = 1;

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
    return ((double) tf + smoothing) / (total + smoothing * dict.size());
}

int main() {
    // read training data from file
    std::ifstream data_stream;
    data_stream.open("training_data.txt");
    // if it exists, load data into dict
    std::string line;
    if(data_stream.is_open()) {
        getline(data_stream, line);
        ham = std::stoi(line);
        getline(data_stream, line);
        spam = std::stoi(line);
        while(getline(data_stream, line)) {
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


    std::string message = "We compute the alpha invariant of any smooth complex ";

    int pos = 0;
    std::string token;
    double message_likelihood_ham = 0;
    double message_likelihood_spam = 0;
    while((pos = message.find(" ")) != std::string::npos) {
        token = message.substr(0, pos);
        message.erase(0, pos + 1);
        std::cout << "Found word " << token << std::endl;
        std::cout << '\t' << compute_word_likelihood(token, true) << " vs. " << compute_word_likelihood(token, false) << std::endl;
        message_likelihood_ham += std::log2(compute_word_likelihood(token, true));
        message_likelihood_spam += std::log2(compute_word_likelihood(token, false));
    }

    std::cout << "log posterior(ham) = " << message_likelihood_ham + std::log2(hamp) << std::endl;
    std::cout << "log posterior(spam) = " << message_likelihood_spam + std::log2(spamp) << std::endl;

    return 0;
}
