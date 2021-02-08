// Pipeline.cpp : https://www.coursera.org/ C++ Development Fundamentals: Brown Belt, Week 3.
// Task: Chain of responsibility - develop a pipeline of email handlers
//

#include "test_runner.h"
//#include "..\..\test_runner.h"

#include <functional>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

struct Email {
    string from;
    string to;
    string body;
};

class Worker {
private:
    unique_ptr<Worker> _next;
public:
    virtual ~Worker() = default;
    virtual void Process(unique_ptr<Email> email) = 0;
    virtual void Run()
    {
        // ?????? ??????? worker-? ? ????????? ????? ??? ????????????????
        throw logic_error("Unimplemented");
    }

protected:
    // ?????????? ?????? ???????? PassOn, ????? ???????? ?????? ??????
    // ?? ??????? ????????????

    void PassOn(unique_ptr<Email> email) const {
        if (_next != nullptr) {
            _next->Process(move(email));
            //cout << "=============== Passed to the next Worker ==============\n";
        }
    }

public:
    void SetNext(unique_ptr<Worker> next)
    {
        if (_next) {
            _next->SetNext(move(next));
            //cout << "=============== Set Next Worker ==============\n";
        } 
        else {
            _next = move(next);
            //cout << "=============== END ==============\n";
        }
            
    }
};

class Reader : public Worker {
public:
    // ?????????? ?????
    Reader(istream& in) : _in(in) {
        //cout << "=============== Reader created ==============\n";
    }
    void Process(unique_ptr<Email> email) override {}

    void Run() override
    {
        while (_in) {
            unique_ptr<Email> email = make_unique<Email>();
            getline(_in, email->from);
            getline(_in, email->to);
            getline(_in, email->body);
            if (_in) {
                PassOn(move(email));
                //cout << "=============== Reader passed email ==============\n";
            }
        }
    }

private:
    istream& _in;
};

class Filter : public Worker {
public:
    using Function = function<bool(const Email&)>;
    // ?????????? ?????
    Filter(const Function& filter) : _filter{ filter } {
        //cout << "=============== Filter created ==============\n"; 
    }

    void Process(unique_ptr<Email> email) override {
        if (_filter(*email)) {
            PassOn(move(email));
            //cout << "=============== Filter passed email ==============\n";
        }
    }

private:
    Function _filter;
};

class Copier : public Worker {
public:
    // ?????????? ?????
    explicit Copier(const string& recipient) : _recipient(recipient) {
    //Copier(string&& recipient) : _recipient(recipient) {
        //cout << "=============== Copier created ==============\n";
    }

    void Process(unique_ptr<Email> email) override
    {
        if (email->to != _recipient) {
          //unique_ptr<Email> mail = make_unique<Email>(email->from, recipient_, email->body);
            unique_ptr<Email> cc_email = make_unique<Email>(Email{ email->from, _recipient, email->body });
            PassOn(move(email));
            PassOn(move(cc_email));
            //cout << "=============== Copier passed original and cc emails ==============\n";
        }
        else {
            PassOn(move(email));
            //cout << "=============== Copier passed original_email ==============\n";
        }
            
    }

private:
    string _recipient;
};

class Sender : public Worker {
public:
    // ?????????? ?????
    Sender(ostream& out) : _out(out) {
       //cout << "=============== Sender created ==============\n";
    }

    void Process(unique_ptr<Email> email) override {
        _out << email->from << '\n'
             << email->to << '\n'
             << email->body << '\n';
        PassOn(move(email));
        //cout << "=============== Sender passed email ==============\n";
    }

private:
    ostream& _out;
};

// ?????????? ?????
class PipelineBuilder {
public:
    // ????????? ? ???????? ??????? ??????????? Reader
    explicit PipelineBuilder(istream& in) {
        _pipeline = make_unique<Reader>(in);
        //cout << "Reader -> ";
    }

    // ????????? ????? ?????????? Filter
    PipelineBuilder& FilterBy(Filter::Function filter) {
        _pipeline->SetNext(make_unique<Filter>(filter));
        //cout << "Filter -> ";
        return *this;
    }

    // ????????? ????? ?????????? Copier
    PipelineBuilder& CopyTo(string recipient) {
        _pipeline->SetNext(make_unique<Copier>(move(recipient)));
        //cout << "Copier -> ";
        return *this;
    }

    // ????????? ????? ?????????? Sender
    PipelineBuilder& Send(ostream& out) {
        _pipeline->SetNext(make_unique<Sender>(out));
        //cout << "Sender -> ";
        return *this;
    }

    // ?????????? ??????? ??????? ????????????
    unique_ptr<Worker> Build() { 
        return move(_pipeline); 
    }

private:
    unique_ptr<Worker> _pipeline;
};

void TestFilter() {
    string input = (
        "erich@example.com\n"
        "richard@example.com\n"
        "Hello there\n"

        "erich@example.com\n"
        "ralph@example.com\n"
        "Are you sure you pressed the right button?\n"

        "ralph@example.com\n"
        "erich@example.com\n"
        "I do not make mistakes of that kind\n"
        );
    istringstream inStream(input);
    ostringstream outStream;
    //cout << input
    //    << "=======================================================" << endl;

    PipelineBuilder builder(inStream);
    builder.FilterBy([](const Email& email) {
        return email.from == "erich@example.com";
        });
    //builder.CopyTo("richard@example.com");
    builder.Send(outStream);
    auto pipeline = builder.Build();

    pipeline->Run();

    string expectedOutput = (
        "erich@example.com\n"
        "richard@example.com\n"
        "Hello there\n"

        "erich@example.com\n"
        "ralph@example.com\n"
        "Are you sure you pressed the right button?\n"
        );

    ASSERT_EQUAL(expectedOutput, outStream.str());
}

void TestReader() {
    string input = "\n\n\n\n";
    istringstream inStream(input);
    ostringstream outStream;

    PipelineBuilder builder(inStream);
    builder.Send(outStream);
    auto pipeline = builder.Build();

    pipeline->Run();

    string expectedOutput = "\n\n\n";

    ASSERT_EQUAL(expectedOutput, outStream.str());
}

void TestSanity() {
    string input = (
        "erich@example.com\n"
        "richard@example.com\n"
        "Hello there\n"

        "erich@example.com\n"
        "ralph@example.com\n"
        "Are you sure you pressed the right button?\n"

        "ralph@example.com\n"
        "erich@example.com\n"
        "I do not make mistakes of that kind\n"
        );
    istringstream inStream(input);
    ostringstream outStream;
    //cout << input
    //     << "=======================================================" << endl;

    PipelineBuilder builder(inStream);
    builder.FilterBy([](const Email& email) {
        return email.from == "erich@example.com";
        });
    builder.CopyTo("richard@example.com");
    builder.Send(outStream);
    auto pipeline = builder.Build();

    pipeline->Run();

    string expectedOutput = (
        "erich@example.com\n"
        "richard@example.com\n"
        "Hello there\n"

        "erich@example.com\n"
        "ralph@example.com\n"
        "Are you sure you pressed the right button?\n"

        "erich@example.com\n"
        "richard@example.com\n"
        "Are you sure you pressed the right button?\n"
        );

    ASSERT_EQUAL(expectedOutput, outStream.str());
}

int main() {
    TestRunner tr;
    //RUN_TEST(tr, TestFilter);
    RUN_TEST(tr, TestSanity);
    RUN_TEST(tr, TestReader);
    return 0;
}