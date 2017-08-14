#include <atomic>
#include <opencv2/opencv.hpp>
#include <sys/stat.h>
#include <thread>
#include <algorithm>
#include <queue>

using namespace std;
using namespace cv;

mutex cout_mutex;

#ifndef THREADPOOL_H
#define THREADPOOL_H

bool is_delimiter(char c) {
    return c == '/';
}

void create_folder(const std::string& file_path)
{
    size_t found = 0;
    found = file_path.find_first_of("/", found + 1);

    while ( found != string::npos) {
        string dirpath = file_path.substr(0, found);
        mkdir(dirpath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        found = file_path.find_first_of("/", found + 1);
    }
}

class ImageLoader;

class DataSource {
private:

    queue<ImageLoader*> data;
public:
    mutex _mt;
    condition_variable cv;

    DataSource() {}

    void push(ImageLoader* loader)
    {
        lock_guard<mutex> lk(_mt);
        data.push(loader);
        cv.notify_one();
    }

    ImageLoader* wait_and_pop()
    {
        unique_lock<mutex> lk(_mt);
        cv.wait(lk, [this] {
            return !this->data.empty();
        });
        ImageLoader* il =  data.front();
        data.pop();
        lk.unlock();

        return il;
    }

    bool empty()
    {
        lock_guard<mutex> lk(_mt);
        return data.empty();
    }

};

class ImageLoader {
private:

    DataSource* data_source;
    cv::Size _sz;

    Mat output;
public:
    std::string path;
    ImageLoader(const string& image_path): path(image_path), _sz(128,128) {}
    ImageLoader(const ImageLoader& other) {
        path = other.path;
        _sz = other._sz;
        data_source = other.data_source;
    }

    ImageLoader& operator=(const ImageLoader& other) {
        path = other.path;
        _sz = other._sz;
        data_source = other.data_source;
        return *this;
    }

    ImageLoader() {}

    void setDataSource(DataSource* new_data_source)
    {
        data_source = new_data_source;
    }

    Mat getImage()
    {
       return output;
    }

    std::string getImagePath()
    {
        return path;
    }

    bool operator() () {
        // Loading image using imread
        Mat img = imread(path, IMREAD_COLOR);
        // Check whether the image load is valid
        if (!img.data) {
            return false;
        }

        // Resize the image using resize
        resize(img, output, _sz);

        // Push data to data source
        data_source->push(this);

        //cout << "Called from " << this_thread::get_id() << " " << "load completed" << endl;
        return true;
    }
};

class ImageWriter {
private:
    DataSource* data_src;
    ImageLoader* _loader;
public:

    ImageWriter() {}
    ImageWriter(DataSource* src, ImageLoader* loader) : data_src(src), _loader(loader) {}
    ImageWriter(const ImageWriter& other)
    {
        data_src = other.data_src;
        _loader = other._loader;

    }

    bool operator () () {
        ImageLoader* imageLoader = _loader;
        if (_loader->path.empty()) return false;
        Mat img = imageLoader->getImage();
        string path = imageLoader->getImagePath();

        auto prefix_pos = path.find("fddb");
        string c_path = path.substr(prefix_pos);
        const string prefix_path = "/Users/bbphuc/Desktop/output/";
        string output_path = prefix_path + c_path;
        {
            lock_guard<mutex> lk(cout_mutex);
            //cout << "Writing on file " << output_path << endl;
        }

        create_folder(output_path);
        try {
            imwrite(output_path.c_str(), img);
        } catch (...) {
            lock_guard<mutex> lk(cout_mutex);
            cout << "There are some error on file : " << path << endl;
        }

        return true;
    }
};

template <typename Task>
class ThreadPool
{
private:
    std::atomic_bool done;
    std::atomic_int counter;
    threadsafe_queue<Task> work_queue;
    unsigned int numberOfThreads;

    std::vector<std::thread> threads;
    join_threads joiner;

    bool is_done()
    {
        return done && work_queue.empty();
    }

    void worker_thread()
    {
        while (!is_done()) {
            Task task;
            if (work_queue.try_pop(task)) {
                task();
            } else {
                std::this_thread::yield();
            }
        }
    }

    void init()
    {
        try {
            for (unsigned i = 0; i < numberOfThreads; ++ i) {
                threads.push_back(std::thread(&ThreadPool::worker_thread, this));
            }
        }
        catch(...) {
            done = true;
            throw;
        }
    }

public:
    ThreadPool(): done(false), joiner(threads)
    {
        numberOfThreads = std::thread::hardware_concurrency();
        init();
    }

    ThreadPool(unsigned int number_of_thread): done(false), joiner(threads), numberOfThreads(number_of_thread)
    {
        init();
    }

    ~ThreadPool()
    {
        done = true;
    }

    void submit(const Task& path)
    {
        work_queue.push(path);
    }

};

class thread_pool
{
    std::atomic_bool done;
    std::atomic_int counter;
    threadsafe_queue<std::string> work_queue;
    std::vector<std::thread> threads;
    join_threads joiner;

    void load_and_resize(std::string path)
    {
        Mat img, dst;
        char p[256];

        img = imread(path);
        {
            lock_guard<mutex> lk(cout_mutex);
            cout << "Loading " << path << endl;
        }

        // check loading is correct
        if (img.empty()) {
            lock_guard<mutex> lk(cout_mutex);
            cout << "Loading file failed" << path << endl;
            return;
        }

        resize(img, dst, Size(128,128));
        // int value  = (int)counter;
        // Get the file name of a input file
        auto it = find_if(path.rbegin(), path.rend(), is_delimiter).base();
        string basename (it, path.end());


        sprintf(p, "/Users/bbphuc/Desktop/output/%s", basename.c_str());
        auto prefix_pos = path.find("fddb");
        string c_path = path.substr(prefix_pos);
        const string prefix_path = "/Users/bbphuc/Desktop/output/";
        string output_path = prefix_path + c_path;
        {
            lock_guard<mutex> lk(cout_mutex);
            cout << "Writing on file " << output_path << endl;
        }

        create_folder(output_path);
        imwrite(output_path.c_str(), dst);
        counter++;

    }

    bool is_done()
    {
        return done && work_queue.empty();
    }

    void worker_thread()
    {
        while (!is_done()) {
            std::string path;
            if (work_queue.try_pop(path)) {
                load_and_resize(path);
            } else {
                std::this_thread::yield();
            }
        }
    }

public:
    thread_pool(): done(false), joiner(threads)
    {
        unsigned const numOfThreads = std::thread::hardware_concurrency();

        try {
            for (unsigned i = 0; i < numOfThreads; ++ i)
            {
                threads.push_back(
                            std::thread(&thread_pool::worker_thread, this)
                );
            }
        }
        catch(...) {
            {

                cout << "There error" << endl;
            }
            done = true;
            throw;
        }
    }

    ~thread_pool()
    {
        done = true;
    }

    void submit(const std::string& path)
    {
        work_queue.push(path);
    }
};

#endif // THREADPOOL_H
