#ifndef EVALUATION_OF_IMPLEMENTATION_DETAILS_FOR_ZERO_EVALUATION_FRAMEWORK_HPP
#define EVALUATION_OF_IMPLEMENTATION_DETAILS_FOR_ZERO_EVALUATION_FRAMEWORK_HPP

#include <chrono>
#include <random>
#include <thread>
#include <iostream>
#include <cstring>
#include <boost/program_options.hpp>
#include <sys/ioctl.h>

#include <iomanip>
#include <sstream>
#include <tuple>

template <typename Container, typename Fun>
void tupleForEach(const Container &c, Fun fun) {
    for (auto& e : c)
        fun(std::get<0>(e), std::get<1>(e), std::get<2>(e));
}

std::string createString(std::chrono::nanoseconds time) {
    using namespace std::chrono;

    constexpr std::tuple<std::chrono::nanoseconds, u_long, const char*> formats[] = {
            std::tuple<std::chrono::nanoseconds, u_long, const char*>{std::chrono::hours(1), 2, ""},
            std::tuple<std::chrono::nanoseconds, u_long, const char*>{std::chrono::minutes(1), 2, ":"},
            std::tuple<std::chrono::nanoseconds, u_long, const char*>{std::chrono::seconds(1), 2, ":"},
            std::tuple<std::chrono::nanoseconds, u_long, const char*>{std::chrono::milliseconds(1), 3, "."},
            std::tuple<std::chrono::nanoseconds, u_long, const char*>{std::chrono::microseconds(1), 3, "."},
            std::tuple<std::chrono::nanoseconds, u_long, const char*>{std::chrono::nanoseconds(1), 3, "."}
    };

    std::ostringstream o;
    tupleForEach(formats, [&time, &o](auto denominator, auto width, auto separator) {
        o << separator << std::setw(width) << std::setfill('0') << (time / denominator);
        time = time % denominator;
    });
    return o.str();
}

std::ostream &operator<<(std::ostream &stream, std::chrono::nanoseconds time) {
    stream << createString(time);
    return stream;
}


namespace po = boost::program_options;

class Evaluation {
public:
    Evaluation(std::string name) : name(name) {
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

        generalOptions = new po::options_description("General Options", unsigned(w.ws_col), unsigned(w.ws_col / 2));
        specificOptions = new po::options_description("Specific Options", w.ws_col, w.ws_col / 2);
        allOptions = new po::options_description(name, w.ws_col, w.ws_col / 2);
    }

    virtual ~Evaluation() {}

    void run(int argc, char *argv[]) {
        setOptions();
        setSpecificOptions();
        combineOptions();
        parseSettings(argc, argv);
        setSpecificConfig();

        initialize();
        printConfiguration();

        runTimeoutThread();

        runBenchmark();

        printResult(timeElapsed);
        unInitialize();

        exit(0);
        timeoutThread->join();
    }

protected:
    std::string                 name;

    po::options_description*    generalOptions;
    po::options_description*    specificOptions;
    po::options_description*    allOptions;
    po::variables_map           settings;

    uint_fast32_t               threadCount;
    uint_fast64_t               iterationsCount;
    uint_fast64_t               timeoutInNS;
    bool                        extendedOutput;
    bool                        debugOutput;

    uint_fast64_t               timeElapsed;

    std::thread**               threads;
    std::thread*                timeoutThread;

private:
    void setOptions() {
        generalOptions->add_options()
                ("help,h", "Print this help messages.")
                ("threads,t", po::value<uint_fast32_t>(&threadCount)->default_value(std::thread::hardware_concurrency() ? std::thread::hardware_concurrency() : 1)->notifier([](uint_fast32_t value) { if (value <= 0) {throw po::invalid_option_value(std::to_string(value));}}), "Number of threads to use.")
                ("iterations,i", po::value<uint_fast32_t>(&iterationsCount)->default_value(1000000)->notifier([](uint_fast32_t value) { if (value <= 0) {throw po::invalid_option_value(std::to_string(value));}}), "Number of iterations per thread.")
                ("timeout", po::value<uint_fast64_t>(&timeoutInNS)->default_value(10000), "Timeout per thread and iteration until the running threads get terminated (0 is no timeout).")
                ("debug,d", po::bool_switch(&debugOutput)->default_value(false), "Print additional debug information (implies --extended).")
                ("extended,e", po::bool_switch(&extendedOutput)->default_value(false), "Print extended output.");
    }

protected:
    virtual void setSpecificOptions() {};

private:
    void combineOptions() {
        allOptions->add(*generalOptions);
        allOptions->add(*specificOptions);
    }

    void parseSettings(int argc, char *argv[]) {
        try {
            po::store(po::parse_command_line(argc, argv, *allOptions), settings); // can throw

            if (settings.count("help")) {
                std::cout << *allOptions << std::endl;
                exit(0);
            }

            po::notify(settings); // throws on error, so do after help in case there are any problems
        } catch(po::error& e) {
            std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
            std::cerr << *allOptions << std::endl;
            exit(1);
        }

        if (debugOutput)
            extendedOutput = true;

    }

protected:
    virtual void setSpecificConfig() {};

    virtual void initialize() {};

private:
    void printConfiguration() {
        if (extendedOutput) {
            struct winsize w;
            ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

            for (int i = 1; i <= w.ws_col; i++)
                std::cout << "#";
            std::cout << std::endl;

            std::cout << "Configuration:" << std::endl;
            std::cout << "Threads: " << threadCount << std::endl;
            std::cout << "Iterations: " << iterationsCount << std::endl;
            printSpecificConfigurationExtended();
            std::cout << "Timeout: " << std::chrono::nanoseconds(timeoutInNS) << std::endl;
            std::cout << "Debug: " << (debugOutput ? "Yes" : "No") << std::endl;

            for (int i = 1; i <= w.ws_col; i++)
                std::cout << "#";
            std::cout << std::endl;
        } else {
            std::cout << threadCount << "\t" << iterationsCount;
            printSpecificConfiguration();
        }
    }

protected:
    virtual void printSpecificConfiguration() {};

    virtual void printSpecificConfigurationExtended() {};

private:
    void runTimeoutThread() {
        if (timeoutInNS)
            timeoutThread = new std::thread([&]{timeout();});
    }

    void timeout() {
        std::this_thread::sleep_for(std::chrono::nanoseconds(threadCount * iterationsCount * timeoutInNS));
        if (extendedOutput) {
            std::cout << "##################################################################################################################" << std::endl
                      << "Results:" << std::endl
                      << "Time Elapsed: " << "Timeout after " << std::chrono::nanoseconds(threadCount * iterationsCount * timeoutInNS) << "ns" << std::endl
                      << "##################################################################################################################" << std::endl;
        } else {
            std::cout << "\t" << "timeout" << std::endl;
        }
        exit(512);
    }

    void runBenchmark() {
        threads = new std::thread*[threadCount]();

        u_long start = std::chrono::high_resolution_clock::now().time_since_epoch().count();

        if (extendedOutput) std::cout << "Start spawning " << threadCount << " threads ..." << std::endl;
        for (uint_fast32_t i = 0; i < threadCount; i++) {
            threads[i] = new std::thread([&]{doWork([&]{work();}, [&]{before();}, [&]{after();});});
        }
        if (extendedOutput) std::cout << "Finished spawning " << threadCount << " threads ..." << std::endl;

        if (extendedOutput) std::cout << "Waiting for " << threadCount << " threads to complete ..." << std::endl;
        for (uint_fast32_t i = 0; i < threadCount; i++) {
            threads[i]->join();
        }
        if (extendedOutput) std::cout << "All " << threadCount << " threads completed ..." << std::endl;

        timeElapsed = std::chrono::high_resolution_clock::now().time_since_epoch().count() - start;
    }

    void doWork(std::function<void()> workLoad, std::function<void()> before, std::function<void()> after) {
        before();
        for (uint_fast32_t i = 1; i <= iterationsCount; i++) {
            workLoad();
        }
        after();
    }

protected:
    virtual void work() {};

    virtual void before() {};

    virtual void after() {};

private:
    void printResult(uint_fast64_t timeElapsed) {
        if (extendedOutput) {
            struct winsize w;
            ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

            for (int i = 1; i <= w.ws_col; i++)
                std::cout << "#";
            std::cout << std::endl;

            std::cout << "Results:" << std::endl;
            std::cout << "Time Elapsed: " << std::chrono::nanoseconds(timeElapsed) << std::endl;
            printSpecificResultExtended();

            for (int i = 1; i <= w.ws_col; i++)
                std::cout << "#";
            std::cout << std::endl;
        } else {
            std::cout << "\t" << timeElapsed;
            printSpecificResult();
            std::cout << std::endl;
        }
    }

protected:
    virtual void printSpecificResult() {};

    virtual void printSpecificResultExtended() {};

    virtual void unInitialize() {};

};

#endif //EVALUATION_OF_IMPLEMENTATION_DETAILS_FOR_ZERO_EVALUATION_FRAMEWORK_HPP
