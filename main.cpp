#include <stdio.h>
#include <utility>

#include <thread>

#include <atomic>
#include <vector>
#include <future>
#include <chrono>
#include <functional>
#include <numeric>
#include <type_traits>


#include <stack>

#include <opencv2/opencv.hpp>

#include "threadsafe_queue.h"
#include "threadpool.h"
#include "file_helper.h"

using namespace std;
using namespace cv;

std::condition_variable conditional_var;

template<typename Iterator,typename T>
struct accumulate_block
{
    T operator()(Iterator first,Iterator last)
    {
        return std::accumulate(first,last, T());
    }
};

namespace pool {

}

int sum(int i, int j) {
    return i + j;
}

int main(int argc, char *argv[])
{
    cout << "Number of threads is : " << thread::hardware_concurrency() << endl;
    auto files = list_image_file("/Users/bbphuc/Desktop/fddb");
    cout << "Number of files is :" << files->size() << endl;
    // Create thread_pool

    // Test
    vector<long> list;
    for (int i = 0; i < 1000; ++i) {
        list.push_back(i);
    }


    mutex m;
    ThreadPool pool;
    for (size_t i = 0; i < 10; ++i) {
        auto r = pool.submit([&pool, &m](const std::string& path) {
            {
                lock_guard<mutex> guard(cout_mutex);
                cout << path << endl;
            }
            future<Mat> src = pool.submit([](const std::string& path) {
                Mat src =  imread(path);
                Mat dst;
                resize(src, dst, Size(128,128));
                return dst;
            }, std::cref(path));

            Mat dst;

            if (path.empty()) return dst;

            Mat img = src.get();

            auto prefix_pos = path.find("fddb");
            string c_path = path.substr(prefix_pos);
            const string prefix_path = "/Users/bbphuc/Desktop/output/";
            string output_path = prefix_path + c_path;
            {
                lock_guard<mutex> lk(cout_mutex);
                cout << "Writing on file " << output_path << endl;
            }

            create_folder(output_path);
            try {
                imwrite(output_path.c_str(), img);
            } catch (...) {
                lock_guard<mutex> lk(cout_mutex);
                cout << "There are some error on file : " << path << endl;
            }

            return dst;
        }, std::cref(files->at(i)));
        r.get();
    }

    //imshow("Gfg", r.get());
    //waitKey();
 /*
 #if 1
    int counter = 0;
    DataSource ds;

    ThreadPool<ImageLoader> pool1;

    for (unsigned i = 0; i < files->size(); ++i) {
        shared_ptr<ImageLoader> loader(new ImageLoader(files->at(i))); // Error here
        loader.setDataSource(&ds);
        pool1.submit(loader);
    }


    ThreadPool<ImageWriter> pool2;
    while(counter < files->size()) {
        auto loader = ds.wait_and_pop();
        ImageWriter writer(&ds, loader);
        cout << loader->getImagePath()
             << endl;
        pool2.submit(writer);
        counter++;
    }
#else


    thread_pool pool;

    // Create two threads
    for (size_t i = 0; i < files->size(); ++i) {
        pool.submit(files->at(i));
    }

#endif
*/
    return 0;
}
