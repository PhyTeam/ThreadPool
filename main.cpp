#include <stdio.h>
#include <utility>

// For read folders and files
#include <sys/types.h>
#include <dirent.h>

#include <thread>

#include <atomic>
#include <vector>
#include <future>
#include <chrono>
#include <functional>

#include <stack>

#include <opencv2/opencv.hpp>

#include "threadsafe_queue.h"
#include "threadpool.h"

using namespace std;
using namespace cv;

std::condition_variable conditional_var;


shared_ptr<vector<string>> list_image_file(const string& root)
{
    shared_ptr<vector<string>> result(new vector<string>);

    string folder = root;
    DIR* dp;
    struct dirent* _dirent;

    // DF Travel
    stack<string> _remainDirs;
    _remainDirs.push(folder);

    vector<string> accepted_ext {"jpg", "png"}; //  Accepted file extensions
    //vector<string> image_files;
    while (!_remainDirs.empty())
    {
        string current_path = _remainDirs.top();
        _remainDirs.pop();
        dp = opendir(current_path.c_str());
        _dirent = NULL;
        //unsigned counter = 0;
        while ((_dirent = readdir(dp)) != NULL)
        {
           //cout  << counter++ << ") " << _dirent->d_name << " " << ((_dirent->d_type == DT_DIR) ? "Folder" : "Others")  << endl;
           string dirname(_dirent->d_name);
           if (!(dirname.compare(".") == 0) && !(dirname.compare("..") == 0)) {
               //printf("%d) %s (%s)\n", counter++, _dirent->d_name, (_dirent->d_type == DT_DIR) ? "Folder" : "Others");
               // Check dir type
               if (_dirent->d_type == DT_DIR) {
                    string full_path = current_path + "/" + dirname;
                    _remainDirs.push(full_path);
                   //cout << full_path << endl;
               } // end if type

               if (_dirent->d_type == DT_REG) {
                   auto it = find_if(dirname.rbegin(), dirname.rend(), [](char c) { return c == '.'; });
                   string ext = string(it.base(), dirname.end());
                   bool is_accepted = false;
                   for (auto i = accepted_ext.begin(); i != accepted_ext.end(); ++i) {
                       is_accepted |= (*i).compare(ext) == 0;
                   }
                   if (is_accepted)
                   {
                       string file_path = current_path + "/" + dirname;
                       //cout << file_path << endl;
                       result->push_back(file_path);
                   }
               }

           } //  end if
        } // dir loop
        closedir(dp);
    } // dft
    return result;
}

int main(int argc, char *argv[])
{
    cout << "Number of threads is : " << thread::hardware_concurrency() << endl;
    auto files = list_image_file("/Users/bbphuc/Desktop/fddb");
    cout << "Number of files is :" << files->size() << endl;
    // Create thread_pool
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
    return 0;
}
